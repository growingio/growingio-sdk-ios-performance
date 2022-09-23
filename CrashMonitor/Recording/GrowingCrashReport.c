//  Created by Karl Stenerud on 2012-01-28.
//
//  Copyright (c) 2012 Karl Stenerud. All rights reserved.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall remain in place
// in this source code.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//
//  GrowingCrashReport.c
//  GrowingAnalytics
//
//  Created by YoloMao on 2022/9/19.
//  Copyright (C) 2022 Beijing Yishu Technology Co., Ltd.
//
//  Licensed under the Apache License, Version 2.0 (the "License");
//  you may not use this file except in compliance with the License.
//  You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
//  Unless required by applicable law or agreed to in writing, software
//  distributed under the License is distributed on an "AS IS" BASIS,
//  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//  See the License for the specific language governing permissions and
//  limitations under the License.


#include "GrowingCrashReport.h"

#include "GrowingCrashReportFields.h"
#include "GrowingCrashReportWriter.h"
#include "GrowingCrashDynamicLinker.h"
#include "GrowingCrashFileUtils.h"
#include "GrowingCrashJSONCodec.h"
#include "GrowingCrashCPU.h"
#include "GrowingCrashMemory.h"
#include "GrowingCrashMach.h"
#include "GrowingCrashThread.h"
#include "GrowingCrashObjC.h"
#include "GrowingCrashSignalInfo.h"
#include "GrowingCrashMonitor_Zombie.h"
#include "GrowingCrashString.h"
#include "GrowingCrashReportVersion.h"
#include "GrowingCrashStackCursor_Backtrace.h"
#include "GrowingCrashStackCursor_MachineContext.h"
#include "GrowingCrashSystemCapabilities.h"
#include "GrowingCrashCachedData.h"

//#define GrowingCrashLogger_LocalLevel TRACE
#include "GrowingCrashLogger.h"

#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>

// ============================================================================
#pragma mark - Constants -
// ============================================================================

/** Default number of objects, subobjects, and ivars to record from a memory loc */
#define kDefaultMemorySearchDepth 15

/** How far to search the stack (in pointer sized jumps) for notable data. */
#define kStackNotableSearchBackDistance 20
#define kStackNotableSearchForwardDistance 10

/** How much of the stack to dump (in pointer sized jumps). */
#define kStackContentsPushedDistance 20
#define kStackContentsPoppedDistance 10
#define kStackContentsTotalDistance (kStackContentsPushedDistance + kStackContentsPoppedDistance)

/** The minimum length for a valid string. */
#define kMinStringLength 4


// ============================================================================
#pragma mark - JSON Encoding -
// ============================================================================

#define getJsonContext(REPORT_WRITER) ((GrowingCrashJSONEncodeContext*)((REPORT_WRITER)->context))

/** Used for writing hex string values. */
static const char g_hexNybbles[] =
{
    '0', '1', '2', '3', '4', '5', '6', '7',
    '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'
};

// ============================================================================
#pragma mark - Runtime Config -
// ============================================================================

typedef struct
{
    /** If YES, introspect memory contents during a crash.
     * Any Objective-C objects or C strings near the stack pointer or referenced by
     * cpu registers or exceptions will be recorded in the crash report, along with
     * their contents.
     */
    bool enabled;
    
    /** List of classes that should never be introspected.
     * Whenever a class in this list is encountered, only the class name will be recorded.
     */
    const char** restrictedClasses;
    int restrictedClassesCount;
} GrowingCrash_IntrospectionRules;

static const char* g_userInfoJSON;
static GrowingCrash_IntrospectionRules g_introspectionRules;
static GrowingCrashReportWriteCallback g_userSectionWriteCallback;


#pragma mark Callbacks

static void addBooleanElement(const GrowingCrashReportWriter* const writer, const char* const key, const bool value)
{
    growingcrashjson_addBooleanElement(getJsonContext(writer), key, value);
}

static void addFloatingPointElement(const GrowingCrashReportWriter* const writer, const char* const key, const double value)
{
    growingcrashjson_addFloatingPointElement(getJsonContext(writer), key, value);
}

static void addIntegerElement(const GrowingCrashReportWriter* const writer, const char* const key, const int64_t value)
{
    growingcrashjson_addIntegerElement(getJsonContext(writer), key, value);
}

static void addUIntegerElement(const GrowingCrashReportWriter* const writer, const char* const key, const uint64_t value)
{
    growingcrashjson_addUIntegerElement(getJsonContext(writer), key, value);
}

static void addStringElement(const GrowingCrashReportWriter* const writer, const char* const key, const char* const value)
{
    growingcrashjson_addStringElement(getJsonContext(writer), key, value, GrowingCrashJSON_SIZE_AUTOMATIC);
}

static void addTextFileElement(const GrowingCrashReportWriter* const writer, const char* const key, const char* const filePath)
{
    const int fd = open(filePath, O_RDONLY);
    if(fd < 0)
    {
        GrowingCrashLOG_ERROR("Could not open file %s: %s", filePath, strerror(errno));
        return;
    }

    if(growingcrashjson_beginStringElement(getJsonContext(writer), key) != GrowingCrashJSON_OK)
    {
        GrowingCrashLOG_ERROR("Could not start string element");
        goto done;
    }

    char buffer[512];
    int bytesRead;
    for(bytesRead = (int)read(fd, buffer, sizeof(buffer));
        bytesRead > 0;
        bytesRead = (int)read(fd, buffer, sizeof(buffer)))
    {
        if(growingcrashjson_appendStringElement(getJsonContext(writer), buffer, bytesRead) != GrowingCrashJSON_OK)
        {
            GrowingCrashLOG_ERROR("Could not append string element");
            goto done;
        }
    }

done:
    growingcrashjson_endStringElement(getJsonContext(writer));
    close(fd);
}

static void addDataElement(const GrowingCrashReportWriter* const writer,
                           const char* const key,
                           const char* const value,
                           const int length)
{
    growingcrashjson_addDataElement(getJsonContext(writer), key, value, length);
}

static void beginDataElement(const GrowingCrashReportWriter* const writer, const char* const key)
{
    growingcrashjson_beginDataElement(getJsonContext(writer), key);
}

static void appendDataElement(const GrowingCrashReportWriter* const writer, const char* const value, const int length)
{
    growingcrashjson_appendDataElement(getJsonContext(writer), value, length);
}

static void endDataElement(const GrowingCrashReportWriter* const writer)
{
    growingcrashjson_endDataElement(getJsonContext(writer));
}

static void addUUIDElement(const GrowingCrashReportWriter* const writer, const char* const key, const unsigned char* const value)
{
    if(value == NULL)
    {
        growingcrashjson_addNullElement(getJsonContext(writer), key);
    }
    else
    {
        char uuidBuffer[37];
        const unsigned char* src = value;
        char* dst = uuidBuffer;
        for(int i = 0; i < 4; i++)
        {
            *dst++ = g_hexNybbles[(*src>>4)&15];
            *dst++ = g_hexNybbles[(*src++)&15];
        }
        *dst++ = '-';
        for(int i = 0; i < 2; i++)
        {
            *dst++ = g_hexNybbles[(*src>>4)&15];
            *dst++ = g_hexNybbles[(*src++)&15];
        }
        *dst++ = '-';
        for(int i = 0; i < 2; i++)
        {
            *dst++ = g_hexNybbles[(*src>>4)&15];
            *dst++ = g_hexNybbles[(*src++)&15];
        }
        *dst++ = '-';
        for(int i = 0; i < 2; i++)
        {
            *dst++ = g_hexNybbles[(*src>>4)&15];
            *dst++ = g_hexNybbles[(*src++)&15];
        }
        *dst++ = '-';
        for(int i = 0; i < 6; i++)
        {
            *dst++ = g_hexNybbles[(*src>>4)&15];
            *dst++ = g_hexNybbles[(*src++)&15];
        }

        growingcrashjson_addStringElement(getJsonContext(writer), key, uuidBuffer, (int)(dst - uuidBuffer));
    }
}

static void addJSONElement(const GrowingCrashReportWriter* const writer,
                           const char* const key,
                           const char* const jsonElement,
                           bool closeLastContainer)
{
    int jsonResult = growingcrashjson_addJSONElement(getJsonContext(writer),
                                           key,
                                           jsonElement,
                                           (int)strlen(jsonElement),
                                           closeLastContainer);
    if(jsonResult != GrowingCrashJSON_OK)
    {
        char errorBuff[100];
        snprintf(errorBuff,
                 sizeof(errorBuff),
                 "Invalid JSON data: %s",
                 growingcrashjson_stringForError(jsonResult));
        growingcrashjson_beginObject(getJsonContext(writer), key);
        growingcrashjson_addStringElement(getJsonContext(writer),
                                GrowingCrashField_Error,
                                errorBuff,
                                GrowingCrashJSON_SIZE_AUTOMATIC);
        growingcrashjson_addStringElement(getJsonContext(writer),
                                GrowingCrashField_JSONData,
                                jsonElement,
                                GrowingCrashJSON_SIZE_AUTOMATIC);
        growingcrashjson_endContainer(getJsonContext(writer));
    }
}

static void addJSONElementFromFile(const GrowingCrashReportWriter* const writer,
                                   const char* const key,
                                   const char* const filePath,
                                   bool closeLastContainer)
{
    growingcrashjson_addJSONFromFile(getJsonContext(writer), key, filePath, closeLastContainer);
}

static void beginObject(const GrowingCrashReportWriter* const writer, const char* const key)
{
    growingcrashjson_beginObject(getJsonContext(writer), key);
}

static void beginArray(const GrowingCrashReportWriter* const writer, const char* const key)
{
    growingcrashjson_beginArray(getJsonContext(writer), key);
}

static void endContainer(const GrowingCrashReportWriter* const writer)
{
    growingcrashjson_endContainer(getJsonContext(writer));
}


static void addTextLinesFromFile(const GrowingCrashReportWriter* const writer, const char* const key, const char* const filePath)
{
    char readBuffer[1024];
    GrowingCrashBufferedReader reader;
    if(!growingcrashfu_openBufferedReader(&reader, filePath, readBuffer, sizeof(readBuffer)))
    {
        return;
    }
    char buffer[1024];
    beginArray(writer, key);
    {
        for(;;)
        {
            int length = sizeof(buffer);
            growingcrashfu_readBufferedReaderUntilChar(&reader, '\n', buffer, &length);
            if(length <= 0)
            {
                break;
            }
            buffer[length - 1] = '\0';
            growingcrashjson_addStringElement(getJsonContext(writer), NULL, buffer, GrowingCrashJSON_SIZE_AUTOMATIC);
        }
    }
    endContainer(writer);
    growingcrashfu_closeBufferedReader(&reader);
}

static int addJSONData(const char* restrict const data, const int length, void* restrict userData)
{
    GrowingCrashBufferedWriter* writer = (GrowingCrashBufferedWriter*)userData;
    const bool success = growingcrashfu_writeBufferedWriter(writer, data, length);
    return success ? GrowingCrashJSON_OK : GrowingCrashJSON_ERROR_CANNOT_ADD_DATA;
}


// ============================================================================
#pragma mark - Utility -
// ============================================================================

/** Check if a memory address points to a valid null terminated UTF-8 string.
 *
 * @param address The address to check.
 *
 * @return true if the address points to a string.
 */
static bool isValidString(const void* const address)
{
    if((void*)address == NULL)
    {
        return false;
    }

    char buffer[500];
    if((uintptr_t)address+sizeof(buffer) < (uintptr_t)address)
    {
        // Wrapped around the address range.
        return false;
    }
    if(!growingcrashmem_copySafely(address, buffer, sizeof(buffer)))
    {
        return false;
    }
    return growingcrashstring_isNullTerminatedUTF8String(buffer, kMinStringLength, sizeof(buffer));
}

/** Get the backtrace for the specified machine context.
 *
 * This function will choose how to fetch the backtrace based on the crash and
 * machine context. It may store the backtrace in backtraceBuffer unless it can
 * be fetched directly from memory. Do not count on backtraceBuffer containing
 * anything. Always use the return value.
 *
 * @param crash The crash handler context.
 *
 * @param machineContext The machine context.
 *
 * @param cursor The stack cursor to fill.
 *
 * @return True if the cursor was filled.
 */
static bool getStackCursor(const GrowingCrash_MonitorContext* const crash,
                           const struct GrowingCrashMachineContext* const machineContext,
                           GrowingCrashStackCursor *cursor)
{
    if(growingcrashmc_getThreadFromContext(machineContext) == growingcrashmc_getThreadFromContext(crash->offendingMachineContext))
    {
        *cursor = *((GrowingCrashStackCursor*)crash->stackCursor);
        return true;
    }

    growingcrashsc_initWithMachineContext(cursor, GROWINGCRASHSC_STACK_OVERFLOW_THRESHOLD, machineContext);
    return true;
}


// ============================================================================
#pragma mark - Report Writing -
// ============================================================================

/** Write the contents of a memory location.
 * Also writes meta information about the data.
 *
 * @param writer The writer.
 *
 * @param key The object key, if needed.
 *
 * @param address The memory address.
 *
 * @param limit How many more subreferenced objects to write, if any.
 */
static void writeMemoryContents(const GrowingCrashReportWriter* const writer,
                                const char* const key,
                                const uintptr_t address,
                                int* limit);

/** Write a string to the report.
 * This will only print the first child of the array.
 *
 * @param writer The writer.
 *
 * @param key The object key, if needed.
 *
 * @param objectAddress The object's address.
 *
 * @param limit How many more subreferenced objects to write, if any.
 */
static void writeNSStringContents(const GrowingCrashReportWriter* const writer,
                                  const char* const key,
                                  const uintptr_t objectAddress,
                                  __unused int* limit)
{
    const void* object = (const void*)objectAddress;
    char buffer[200];
    if(growingcrashobjc_copyStringContents(object, buffer, sizeof(buffer)))
    {
        writer->addStringElement(writer, key, buffer);
    }
}

/** Write a URL to the report.
 * This will only print the first child of the array.
 *
 * @param writer The writer.
 *
 * @param key The object key, if needed.
 *
 * @param objectAddress The object's address.
 *
 * @param limit How many more subreferenced objects to write, if any.
 */
static void writeURLContents(const GrowingCrashReportWriter* const writer,
                             const char* const key,
                             const uintptr_t objectAddress,
                             __unused int* limit)
{
    const void* object = (const void*)objectAddress;
    char buffer[200];
    if(growingcrashobjc_copyStringContents(object, buffer, sizeof(buffer)))
    {
        writer->addStringElement(writer, key, buffer);
    }
}

/** Write a date to the report.
 * This will only print the first child of the array.
 *
 * @param writer The writer.
 *
 * @param key The object key, if needed.
 *
 * @param objectAddress The object's address.
 *
 * @param limit How many more subreferenced objects to write, if any.
 */
static void writeDateContents(const GrowingCrashReportWriter* const writer,
                              const char* const key,
                              const uintptr_t objectAddress,
                              __unused int* limit)
{
    const void* object = (const void*)objectAddress;
    writer->addFloatingPointElement(writer, key, growingcrashobjc_dateContents(object));
}

/** Write a number to the report.
 * This will only print the first child of the array.
 *
 * @param writer The writer.
 *
 * @param key The object key, if needed.
 *
 * @param objectAddress The object's address.
 *
 * @param limit How many more subreferenced objects to write, if any.
 */
static void writeNumberContents(const GrowingCrashReportWriter* const writer,
                                const char* const key,
                                const uintptr_t objectAddress,
                                __unused int* limit)
{
    const void* object = (const void*)objectAddress;
    writer->addFloatingPointElement(writer, key, growingcrashobjc_numberAsFloat(object));
}

/** Write an array to the report.
 * This will only print the first child of the array.
 *
 * @param writer The writer.
 *
 * @param key The object key, if needed.
 *
 * @param objectAddress The object's address.
 *
 * @param limit How many more subreferenced objects to write, if any.
 */
static void writeArrayContents(const GrowingCrashReportWriter* const writer,
                               const char* const key,
                               const uintptr_t objectAddress,
                               int* limit)
{
    const void* object = (const void*)objectAddress;
    uintptr_t firstObject;
    if(growingcrashobjc_arrayContents(object, &firstObject, 1) == 1)
    {
        writeMemoryContents(writer, key, firstObject, limit);
    }
}

/** Write out ivar information about an unknown object.
 *
 * @param writer The writer.
 *
 * @param key The object key, if needed.
 *
 * @param objectAddress The object's address.
 *
 * @param limit How many more subreferenced objects to write, if any.
 */
static void writeUnknownObjectContents(const GrowingCrashReportWriter* const writer,
                                       const char* const key,
                                       const uintptr_t objectAddress,
                                       int* limit)
{
    (*limit)--;
    const void* object = (const void*)objectAddress;
    GrowingCrashObjCIvar ivars[10];
    int8_t s8;
    int16_t s16;
    int sInt;
    int32_t s32;
    int64_t s64;
    uint8_t u8;
    uint16_t u16;
    unsigned int uInt;
    uint32_t u32;
    uint64_t u64;
    float f32;
    double f64;
    bool b;
    void* pointer;
    
    
    writer->beginObject(writer, key);
    {
        if(growingcrashobjc_isTaggedPointer(object))
        {
            writer->addIntegerElement(writer, "tagged_payload", (int64_t)growingcrashobjc_taggedPointerPayload(object));
        }
        else
        {
            const void* class = growingcrashobjc_isaPointer(object);
            int ivarCount = growingcrashobjc_ivarList(class, ivars, sizeof(ivars)/sizeof(*ivars));
            *limit -= ivarCount;
            for(int i = 0; i < ivarCount; i++)
            {
                GrowingCrashObjCIvar* ivar = &ivars[i];
                switch(ivar->type[0])
                {
                    case 'c':
                        growingcrashobjc_ivarValue(object, ivar->index, &s8);
                        writer->addIntegerElement(writer, ivar->name, s8);
                        break;
                    case 'i':
                        growingcrashobjc_ivarValue(object, ivar->index, &sInt);
                        writer->addIntegerElement(writer, ivar->name, sInt);
                        break;
                    case 's':
                        growingcrashobjc_ivarValue(object, ivar->index, &s16);
                        writer->addIntegerElement(writer, ivar->name, s16);
                        break;
                    case 'l':
                        growingcrashobjc_ivarValue(object, ivar->index, &s32);
                        writer->addIntegerElement(writer, ivar->name, s32);
                        break;
                    case 'q':
                        growingcrashobjc_ivarValue(object, ivar->index, &s64);
                        writer->addIntegerElement(writer, ivar->name, s64);
                        break;
                    case 'C':
                        growingcrashobjc_ivarValue(object, ivar->index, &u8);
                        writer->addUIntegerElement(writer, ivar->name, u8);
                        break;
                    case 'I':
                        growingcrashobjc_ivarValue(object, ivar->index, &uInt);
                        writer->addUIntegerElement(writer, ivar->name, uInt);
                        break;
                    case 'S':
                        growingcrashobjc_ivarValue(object, ivar->index, &u16);
                        writer->addUIntegerElement(writer, ivar->name, u16);
                        break;
                    case 'L':
                        growingcrashobjc_ivarValue(object, ivar->index, &u32);
                        writer->addUIntegerElement(writer, ivar->name, u32);
                        break;
                    case 'Q':
                        growingcrashobjc_ivarValue(object, ivar->index, &u64);
                        writer->addUIntegerElement(writer, ivar->name, u64);
                        break;
                    case 'f':
                        growingcrashobjc_ivarValue(object, ivar->index, &f32);
                        writer->addFloatingPointElement(writer, ivar->name, f32);
                        break;
                    case 'd':
                        growingcrashobjc_ivarValue(object, ivar->index, &f64);
                        writer->addFloatingPointElement(writer, ivar->name, f64);
                        break;
                    case 'B':
                        growingcrashobjc_ivarValue(object, ivar->index, &b);
                        writer->addBooleanElement(writer, ivar->name, b);
                        break;
                    case '*':
                    case '@':
                    case '#':
                    case ':':
                        growingcrashobjc_ivarValue(object, ivar->index, &pointer);
                        writeMemoryContents(writer, ivar->name, (uintptr_t)pointer, limit);
                        break;
                    default:
                        GrowingCrashLOG_DEBUG("%s: Unknown ivar type [%s]", ivar->name, ivar->type);
                }
            }
        }
    }
    writer->endContainer(writer);
}

static bool isRestrictedClass(const char* name)
{
    if(g_introspectionRules.restrictedClasses != NULL)
    {
        for(int i = 0; i < g_introspectionRules.restrictedClassesCount; i++)
        {
            if(strcmp(name, g_introspectionRules.restrictedClasses[i]) == 0)
            {
                return true;
            }
        }
    }
    return false;
}

static void writeZombieIfPresent(const GrowingCrashReportWriter* const writer,
                                 const char* const key,
                                 const uintptr_t address)
{
#if GROWINGCRASH_HAS_OBJC
    const void* object = (const void*)address;
    const char* zombieClassName = growingcrashzombie_className(object);
    if(zombieClassName != NULL)
    {
        writer->addStringElement(writer, key, zombieClassName);
    }
#endif
}

static bool writeObjCObject(const GrowingCrashReportWriter* const writer,
                            const uintptr_t address,
                            int* limit)
{
#if GROWINGCRASH_HAS_OBJC
    const void* object = (const void*)address;
    switch(growingcrashobjc_objectType(object))
    {
        case GrowingCrashObjCTypeClass:
            writer->addStringElement(writer, GrowingCrashField_Type, GrowingCrashMemType_Class);
            writer->addStringElement(writer, GrowingCrashField_Class, growingcrashobjc_className(object));
            return true;
        case GrowingCrashObjCTypeObject:
        {
            writer->addStringElement(writer, GrowingCrashField_Type, GrowingCrashMemType_Object);
            const char* className = growingcrashobjc_objectClassName(object);
            writer->addStringElement(writer, GrowingCrashField_Class, className);
            if(!isRestrictedClass(className))
            {
                switch(growingcrashobjc_objectClassType(object))
                {
                    case GrowingCrashObjCClassTypeString:
                        writeNSStringContents(writer, GrowingCrashField_Value, address, limit);
                        return true;
                    case GrowingCrashObjCClassTypeURL:
                        writeURLContents(writer, GrowingCrashField_Value, address, limit);
                        return true;
                    case GrowingCrashObjCClassTypeDate:
                        writeDateContents(writer, GrowingCrashField_Value, address, limit);
                        return true;
                    case GrowingCrashObjCClassTypeArray:
                        if(*limit > 0)
                        {
                            writeArrayContents(writer, GrowingCrashField_FirstObject, address, limit);
                        }
                        return true;
                    case GrowingCrashObjCClassTypeNumber:
                        writeNumberContents(writer, GrowingCrashField_Value, address, limit);
                        return true;
                    case GrowingCrashObjCClassTypeDictionary:
                    case GrowingCrashObjCClassTypeException:
                        // TODO: Implement these.
                        if(*limit > 0)
                        {
                            writeUnknownObjectContents(writer, GrowingCrashField_Ivars, address, limit);
                        }
                        return true;
                    case GrowingCrashObjCClassTypeUnknown:
                        if(*limit > 0)
                        {
                            writeUnknownObjectContents(writer, GrowingCrashField_Ivars, address, limit);
                        }
                        return true;
                }
            }
            break;
        }
        case GrowingCrashObjCTypeBlock:
            writer->addStringElement(writer, GrowingCrashField_Type, GrowingCrashMemType_Block);
            const char* className = growingcrashobjc_objectClassName(object);
            writer->addStringElement(writer, GrowingCrashField_Class, className);
            return true;
        case GrowingCrashObjCTypeUnknown:
            break;
    }
#endif

    return false;
}

/** Write the contents of a memory location.
 * Also writes meta information about the data.
 *
 * @param writer The writer.
 *
 * @param key The object key, if needed.
 *
 * @param address The memory address.
 *
 * @param limit How many more subreferenced objects to write, if any.
 */
static void writeMemoryContents(const GrowingCrashReportWriter* const writer,
                                const char* const key,
                                const uintptr_t address,
                                int* limit)
{
    (*limit)--;
    const void* object = (const void*)address;
    writer->beginObject(writer, key);
    {
        writer->addUIntegerElement(writer, GrowingCrashField_Address, address);
        writeZombieIfPresent(writer, GrowingCrashField_LastDeallocObject, address);
        if(!writeObjCObject(writer, address, limit))
        {
            if(object == NULL)
            {
                writer->addStringElement(writer, GrowingCrashField_Type, GrowingCrashMemType_NullPointer);
            }
            else if(isValidString(object))
            {
                writer->addStringElement(writer, GrowingCrashField_Type, GrowingCrashMemType_String);
                writer->addStringElement(writer, GrowingCrashField_Value, (const char*)object);
            }
            else
            {
                writer->addStringElement(writer, GrowingCrashField_Type, GrowingCrashMemType_Unknown);
            }
        }
    }
    writer->endContainer(writer);
}

static bool isValidPointer(const uintptr_t address)
{
    if(address == (uintptr_t)NULL)
    {
        return false;
    }

#if GROWINGCRASH_HAS_OBJC
    if(growingcrashobjc_isTaggedPointer((const void*)address))
    {
        if(!growingcrashobjc_isValidTaggedPointer((const void*)address))
        {
            return false;
        }
    }
#endif

    return true;
}

static bool isNotableAddress(const uintptr_t address)
{
    if(!isValidPointer(address))
    {
        return false;
    }
    
    const void* object = (const void*)address;

#if GROWINGCRASH_HAS_OBJC
    if(growingcrashzombie_className(object) != NULL)
    {
        return true;
    }

    if(growingcrashobjc_objectType(object) != GrowingCrashObjCTypeUnknown)
    {
        return true;
    }
#endif

    if(isValidString(object))
    {
        return true;
    }

    return false;
}

/** Write the contents of a memory location only if it contains notable data.
 * Also writes meta information about the data.
 *
 * @param writer The writer.
 *
 * @param key The object key, if needed.
 *
 * @param address The memory address.
 */
static void writeMemoryContentsIfNotable(const GrowingCrashReportWriter* const writer,
                                         const char* const key,
                                         const uintptr_t address)
{
    if(isNotableAddress(address))
    {
        int limit = kDefaultMemorySearchDepth;
        writeMemoryContents(writer, key, address, &limit);
    }
}

/** Look for a hex value in a string and try to write whatever it references.
 *
 * @param writer The writer.
 *
 * @param key The object key, if needed.
 *
 * @param string The string to search.
 */
static void writeAddressReferencedByString(const GrowingCrashReportWriter* const writer,
                                           const char* const key,
                                           const char* string)
{
    uint64_t address = 0;
    if(string == NULL || !growingcrashstring_extractHexValue(string, (int)strlen(string), &address))
    {
        return;
    }
    
    int limit = kDefaultMemorySearchDepth;
    writeMemoryContents(writer, key, (uintptr_t)address, &limit);
}

#pragma mark Backtrace

/** Write a backtrace to the report.
 *
 * @param writer The writer to write the backtrace to.
 *
 * @param key The object key, if needed.
 *
 * @param stackCursor The stack cursor to read from.
 */
static void writeBacktrace(const GrowingCrashReportWriter* const writer,
                           const char* const key,
                           GrowingCrashStackCursor* stackCursor)
{
    writer->beginObject(writer, key);
    {
        writer->beginArray(writer, GrowingCrashField_Contents);
        {
            while(stackCursor->advanceCursor(stackCursor))
            {
                writer->beginObject(writer, NULL);
                {
                    if(stackCursor->symbolicate(stackCursor))
                    {
                        if(stackCursor->stackEntry.imageName != NULL)
                        {
                            writer->addStringElement(writer, GrowingCrashField_ObjectName, growingcrashfu_lastPathEntry(stackCursor->stackEntry.imageName));
                        }
                        writer->addUIntegerElement(writer, GrowingCrashField_ObjectAddr, stackCursor->stackEntry.imageAddress);
                        if(stackCursor->stackEntry.symbolName != NULL)
                        {
                            writer->addStringElement(writer, GrowingCrashField_SymbolName, stackCursor->stackEntry.symbolName);
                        }
                        writer->addUIntegerElement(writer, GrowingCrashField_SymbolAddr, stackCursor->stackEntry.symbolAddress);
                    }
                    writer->addUIntegerElement(writer, GrowingCrashField_InstructionAddr, stackCursor->stackEntry.address);
                }
                writer->endContainer(writer);
            }
        }
        writer->endContainer(writer);
        writer->addIntegerElement(writer, GrowingCrashField_Skipped, 0);
    }
    writer->endContainer(writer);
}
                              

#pragma mark Stack

/** Write a dump of the stack contents to the report.
 *
 * @param writer The writer.
 *
 * @param key The object key, if needed.
 *
 * @param machineContext The context to retrieve the stack from.
 *
 * @param isStackOverflow If true, the stack has overflowed.
 */
static void writeStackContents(const GrowingCrashReportWriter* const writer,
                               const char* const key,
                               const struct GrowingCrashMachineContext* const machineContext,
                               const bool isStackOverflow)
{
    uintptr_t sp = growingcrashcpu_stackPointer(machineContext);
    if((void*)sp == NULL)
    {
        return;
    }

    uintptr_t lowAddress = sp + (uintptr_t)(kStackContentsPushedDistance * (int)sizeof(sp) * growingcrashcpu_stackGrowDirection() * -1);
    uintptr_t highAddress = sp + (uintptr_t)(kStackContentsPoppedDistance * (int)sizeof(sp) * growingcrashcpu_stackGrowDirection());
    if(highAddress < lowAddress)
    {
        uintptr_t tmp = lowAddress;
        lowAddress = highAddress;
        highAddress = tmp;
    }
    writer->beginObject(writer, key);
    {
        writer->addStringElement(writer, GrowingCrashField_GrowDirection, growingcrashcpu_stackGrowDirection() > 0 ? "+" : "-");
        writer->addUIntegerElement(writer, GrowingCrashField_DumpStart, lowAddress);
        writer->addUIntegerElement(writer, GrowingCrashField_DumpEnd, highAddress);
        writer->addUIntegerElement(writer, GrowingCrashField_StackPtr, sp);
        writer->addBooleanElement(writer, GrowingCrashField_Overflow, isStackOverflow);
        uint8_t stackBuffer[kStackContentsTotalDistance * sizeof(sp)];
        int copyLength = (int)(highAddress - lowAddress);
        if(growingcrashmem_copySafely((void*)lowAddress, stackBuffer, copyLength))
        {
            writer->addDataElement(writer, GrowingCrashField_Contents, (void*)stackBuffer, copyLength);
        }
        else
        {
            writer->addStringElement(writer, GrowingCrashField_Error, "Stack contents not accessible");
        }
    }
    writer->endContainer(writer);
}

/** Write any notable addresses near the stack pointer (above and below).
 *
 * @param writer The writer.
 *
 * @param machineContext The context to retrieve the stack from.
 *
 * @param backDistance The distance towards the beginning of the stack to check.
 *
 * @param forwardDistance The distance past the end of the stack to check.
 */
static void writeNotableStackContents(const GrowingCrashReportWriter* const writer,
                                      const struct GrowingCrashMachineContext* const machineContext,
                                      const int backDistance,
                                      const int forwardDistance)
{
    uintptr_t sp = growingcrashcpu_stackPointer(machineContext);
    if((void*)sp == NULL)
    {
        return;
    }

    uintptr_t lowAddress = sp + (uintptr_t)(backDistance * (int)sizeof(sp) * growingcrashcpu_stackGrowDirection() * -1);
    uintptr_t highAddress = sp + (uintptr_t)(forwardDistance * (int)sizeof(sp) * growingcrashcpu_stackGrowDirection());
    if(highAddress < lowAddress)
    {
        uintptr_t tmp = lowAddress;
        lowAddress = highAddress;
        highAddress = tmp;
    }
    uintptr_t contentsAsPointer;
    char nameBuffer[40];
    for(uintptr_t address = lowAddress; address < highAddress; address += sizeof(address))
    {
        if(growingcrashmem_copySafely((void*)address, &contentsAsPointer, sizeof(contentsAsPointer)))
        {
            sprintf(nameBuffer, "stack@%p", (void*)address);
            writeMemoryContentsIfNotable(writer, nameBuffer, contentsAsPointer);
        }
    }
}


#pragma mark Registers

/** Write the contents of all regular registers to the report.
 *
 * @param writer The writer.
 *
 * @param key The object key, if needed.
 *
 * @param machineContext The context to retrieve the registers from.
 */
static void writeBasicRegisters(const GrowingCrashReportWriter* const writer,
                                const char* const key,
                                const struct GrowingCrashMachineContext* const machineContext)
{
    char registerNameBuff[30];
    const char* registerName;
    writer->beginObject(writer, key);
    {
        const int numRegisters = growingcrashcpu_numRegisters();
        for(int reg = 0; reg < numRegisters; reg++)
        {
            registerName = growingcrashcpu_registerName(reg);
            if(registerName == NULL)
            {
                snprintf(registerNameBuff, sizeof(registerNameBuff), "r%d", reg);
                registerName = registerNameBuff;
            }
            writer->addUIntegerElement(writer, registerName,
                                       growingcrashcpu_registerValue(machineContext, reg));
        }
    }
    writer->endContainer(writer);
}

/** Write the contents of all exception registers to the report.
 *
 * @param writer The writer.
 *
 * @param key The object key, if needed.
 *
 * @param machineContext The context to retrieve the registers from.
 */
static void writeExceptionRegisters(const GrowingCrashReportWriter* const writer,
                                    const char* const key,
                                    const struct GrowingCrashMachineContext* const machineContext)
{
    char registerNameBuff[30];
    const char* registerName;
    writer->beginObject(writer, key);
    {
        const int numRegisters = growingcrashcpu_numExceptionRegisters();
        for(int reg = 0; reg < numRegisters; reg++)
        {
            registerName = growingcrashcpu_exceptionRegisterName(reg);
            if(registerName == NULL)
            {
                snprintf(registerNameBuff, sizeof(registerNameBuff), "r%d", reg);
                registerName = registerNameBuff;
            }
            writer->addUIntegerElement(writer,registerName,
                                       growingcrashcpu_exceptionRegisterValue(machineContext, reg));
        }
    }
    writer->endContainer(writer);
}

/** Write all applicable registers.
 *
 * @param writer The writer.
 *
 * @param key The object key, if needed.
 *
 * @param machineContext The context to retrieve the registers from.
 */
static void writeRegisters(const GrowingCrashReportWriter* const writer,
                           const char* const key,
                           const struct GrowingCrashMachineContext* const machineContext)
{
    writer->beginObject(writer, key);
    {
        writeBasicRegisters(writer, GrowingCrashField_Basic, machineContext);
        if(growingcrashmc_hasValidExceptionRegisters(machineContext))
        {
            writeExceptionRegisters(writer, GrowingCrashField_Exception, machineContext);
        }
    }
    writer->endContainer(writer);
}

/** Write any notable addresses contained in the CPU registers.
 *
 * @param writer The writer.
 *
 * @param machineContext The context to retrieve the registers from.
 */
static void writeNotableRegisters(const GrowingCrashReportWriter* const writer,
                                  const struct GrowingCrashMachineContext* const machineContext)
{
    char registerNameBuff[30];
    const char* registerName;
    const int numRegisters = growingcrashcpu_numRegisters();
    for(int reg = 0; reg < numRegisters; reg++)
    {
        registerName = growingcrashcpu_registerName(reg);
        if(registerName == NULL)
        {
            snprintf(registerNameBuff, sizeof(registerNameBuff), "r%d", reg);
            registerName = registerNameBuff;
        }
        writeMemoryContentsIfNotable(writer,
                                     registerName,
                                     (uintptr_t)growingcrashcpu_registerValue(machineContext, reg));
    }
}

#pragma mark Thread-specific

/** Write any notable addresses in the stack or registers to the report.
 *
 * @param writer The writer.
 *
 * @param key The object key, if needed.
 *
 * @param machineContext The context to retrieve the registers from.
 */
static void writeNotableAddresses(const GrowingCrashReportWriter* const writer,
                                  const char* const key,
                                  const struct GrowingCrashMachineContext* const machineContext)
{
    writer->beginObject(writer, key);
    {
        writeNotableRegisters(writer, machineContext);
        writeNotableStackContents(writer,
                                  machineContext,
                                  kStackNotableSearchBackDistance,
                                  kStackNotableSearchForwardDistance);
    }
    writer->endContainer(writer);
}

/** Write information about a thread to the report.
 *
 * @param writer The writer.
 *
 * @param key The object key, if needed.
 *
 * @param crash The crash handler context.
 *
 * @param machineContext The context whose thread to write about.
 *
 * @param shouldWriteNotableAddresses If true, write any notable addresses found.
 */
static void writeThread(const GrowingCrashReportWriter* const writer,
                        const char* const key,
                        const GrowingCrash_MonitorContext* const crash,
                        const struct GrowingCrashMachineContext* const machineContext,
                        const int threadIndex,
                        const bool shouldWriteNotableAddresses)
{
    bool isCrashedThread = growingcrashmc_isCrashedContext(machineContext);
    GrowingCrashThread thread = growingcrashmc_getThreadFromContext(machineContext);
    GrowingCrashLOG_DEBUG("Writing thread %x (index %d). is crashed: %d", thread, threadIndex, isCrashedThread);

    GrowingCrashStackCursor stackCursor;
    bool hasBacktrace = getStackCursor(crash, machineContext, &stackCursor);

    writer->beginObject(writer, key);
    {
        if(hasBacktrace)
        {
            writeBacktrace(writer, GrowingCrashField_Backtrace, &stackCursor);
        }
        if(growingcrashmc_canHaveCPUState(machineContext))
        {
            writeRegisters(writer, GrowingCrashField_Registers, machineContext);
        }
        writer->addIntegerElement(writer, GrowingCrashField_Index, threadIndex);
        const char* name = growingccd_getThreadName(thread);
        if(name != NULL)
        {
            writer->addStringElement(writer, GrowingCrashField_Name, name);
        }
        name = growingccd_getQueueName(thread);
        if(name != NULL)
        {
            writer->addStringElement(writer, GrowingCrashField_DispatchQueue, name);
        }
        writer->addBooleanElement(writer, GrowingCrashField_Crashed, isCrashedThread);
        writer->addBooleanElement(writer, GrowingCrashField_CurrentThread, thread == growingcrashthread_self());
        if(isCrashedThread)
        {
            writeStackContents(writer, GrowingCrashField_Stack, machineContext, stackCursor.state.hasGivenUp);
            if(shouldWriteNotableAddresses)
            {
                writeNotableAddresses(writer, GrowingCrashField_NotableAddresses, machineContext);
            }
        }
    }
    writer->endContainer(writer);
}

/** Write information about all threads to the report.
 *
 * @param writer The writer.
 *
 * @param key The object key, if needed.
 *
 * @param crash The crash handler context.
 */
static void writeAllThreads(const GrowingCrashReportWriter* const writer,
                            const char* const key,
                            const GrowingCrash_MonitorContext* const crash,
                            bool writeNotableAddresses)
{
    const struct GrowingCrashMachineContext* const context = crash->offendingMachineContext;
    GrowingCrashThread offendingThread = growingcrashmc_getThreadFromContext(context);
    int threadCount = growingcrashmc_getThreadCount(context);
    GROWINGCRASHMC_NEW_CONTEXT(machineContext);

    // Fetch info for all threads.
    writer->beginArray(writer, key);
    {
        GrowingCrashLOG_DEBUG("Writing %d threads.", threadCount);
        for(int i = 0; i < threadCount; i++)
        {
            GrowingCrashThread thread = growingcrashmc_getThreadAtIndex(context, i);
            if(thread == offendingThread)
            {
                writeThread(writer, NULL, crash, context, i, writeNotableAddresses);
            }
            else
            {
                growingcrashmc_getContextForThread(thread, machineContext, false);
                writeThread(writer, NULL, crash, machineContext, i, writeNotableAddresses);
            }
        }
    }
    writer->endContainer(writer);
}

#pragma mark Global Report Data

/** Write information about a binary image to the report.
 *
 * @param writer The writer.
 *
 * @param key The object key, if needed.
 *
 * @param index Which image to write about.
 */
static void writeBinaryImage(const GrowingCrashReportWriter* const writer,
                             const char* const key,
                             const int index)
{
    GrowingCrashBinaryImage image = {0};
    if(!growingcrashdl_getBinaryImage(index, &image))
    {
        return;
    }

    writer->beginObject(writer, key);
    {
        writer->addUIntegerElement(writer, GrowingCrashField_ImageAddress, image.address);
        writer->addUIntegerElement(writer, GrowingCrashField_ImageVmAddress, image.vmAddress);
        writer->addUIntegerElement(writer, GrowingCrashField_ImageSize, image.size);
        writer->addStringElement(writer, GrowingCrashField_Name, image.name);
        writer->addUUIDElement(writer, GrowingCrashField_UUID, image.uuid);
        writer->addIntegerElement(writer, GrowingCrashField_CPUType, image.cpuType);
        writer->addIntegerElement(writer, GrowingCrashField_CPUSubType, image.cpuSubType);
        writer->addUIntegerElement(writer, GrowingCrashField_ImageMajorVersion, image.majorVersion);
        writer->addUIntegerElement(writer, GrowingCrashField_ImageMinorVersion, image.minorVersion);
        writer->addUIntegerElement(writer, GrowingCrashField_ImageRevisionVersion, image.revisionVersion);
        if(image.crashInfoMessage != NULL)
        {
            writer->addStringElement(writer, GrowingCrashField_ImageCrashInfoMessage, image.crashInfoMessage);
        }
        if(image.crashInfoMessage2 != NULL)
        {
            writer->addStringElement(writer, GrowingCrashField_ImageCrashInfoMessage2, image.crashInfoMessage2);
        }
    }
    writer->endContainer(writer);
}

/** Write information about all images to the report.
 *
 * @param writer The writer.
 *
 * @param key The object key, if needed.
 */
static void writeBinaryImages(const GrowingCrashReportWriter* const writer, const char* const key)
{
    const int imageCount = growingcrashdl_imageCount();

    writer->beginArray(writer, key);
    {
        for(int iImg = 0; iImg < imageCount; iImg++)
        {
            writeBinaryImage(writer, NULL, iImg);
        }
    }
    writer->endContainer(writer);
}

/** Write information about system memory to the report.
 *
 * @param writer The writer.
 *
 * @param key The object key, if needed.
 */
static void writeMemoryInfo(const GrowingCrashReportWriter* const writer,
                            const char* const key,
                            const GrowingCrash_MonitorContext* const monitorContext)
{
    writer->beginObject(writer, key);
    {
        writer->addUIntegerElement(writer, GrowingCrashField_Size, monitorContext->System.memorySize);
        writer->addUIntegerElement(writer, GrowingCrashField_Usable, monitorContext->System.usableMemory);
        writer->addUIntegerElement(writer, GrowingCrashField_Free, monitorContext->System.freeMemory);
    }
    writer->endContainer(writer);
}

/** Write information about the error leading to the crash to the report.
 *
 * @param writer The writer.
 *
 * @param key The object key, if needed.
 *
 * @param crash The crash handler context.
 */
static void writeError(const GrowingCrashReportWriter* const writer,
                       const char* const key,
                       const GrowingCrash_MonitorContext* const crash)
{
    writer->beginObject(writer, key);
    {
#if GROWINGCRASH_HOST_APPLE
        writer->beginObject(writer, GrowingCrashField_Mach);
        {
            const char* machExceptionName = growingcrashmach_exceptionName(crash->mach.type);
            const char* machCodeName = crash->mach.code == 0 ? NULL : growingcrashmach_kernelReturnCodeName(crash->mach.code);
            writer->addUIntegerElement(writer, GrowingCrashField_Exception, (unsigned)crash->mach.type);
            if(machExceptionName != NULL)
            {
                writer->addStringElement(writer, GrowingCrashField_ExceptionName, machExceptionName);
            }
            writer->addUIntegerElement(writer, GrowingCrashField_Code, (unsigned)crash->mach.code);
            if(machCodeName != NULL)
            {
                writer->addStringElement(writer, GrowingCrashField_CodeName, machCodeName);
            }
            writer->addUIntegerElement(writer, GrowingCrashField_Subcode, (size_t)crash->mach.subcode);
        }
        writer->endContainer(writer);
#endif
        writer->beginObject(writer, GrowingCrashField_Signal);
        {
            const char* sigName = growingcrashsignal_signalName(crash->signal.signum);
            const char* sigCodeName = growingcrashsignal_signalCodeName(crash->signal.signum, crash->signal.sigcode);
            writer->addUIntegerElement(writer, GrowingCrashField_Signal, (unsigned)crash->signal.signum);
            if(sigName != NULL)
            {
                writer->addStringElement(writer, GrowingCrashField_Name, sigName);
            }
            writer->addUIntegerElement(writer, GrowingCrashField_Code, (unsigned)crash->signal.sigcode);
            if(sigCodeName != NULL)
            {
                writer->addStringElement(writer, GrowingCrashField_CodeName, sigCodeName);
            }
        }
        writer->endContainer(writer);

        writer->addUIntegerElement(writer, GrowingCrashField_Address, crash->faultAddress);
        if(crash->crashReason != NULL)
        {
            writer->addStringElement(writer, GrowingCrashField_Reason, crash->crashReason);
        }

        // Gather specific info.
        switch(crash->crashType)
        {
            case GrowingCrashMonitorTypeMainThreadDeadlock:
                writer->addStringElement(writer, GrowingCrashField_Type, GrowingCrashExcType_Deadlock);
                break;
                
            case GrowingCrashMonitorTypeMachException:
                writer->addStringElement(writer, GrowingCrashField_Type, GrowingCrashExcType_Mach);
                break;

            case GrowingCrashMonitorTypeCPPException:
            {
                writer->addStringElement(writer, GrowingCrashField_Type, GrowingCrashExcType_CPPException);
                writer->beginObject(writer, GrowingCrashField_CPPException);
                {
                    writer->addStringElement(writer, GrowingCrashField_Name, crash->CPPException.name);
                }
                writer->endContainer(writer);
                break;
            }
            case GrowingCrashMonitorTypeNSException:
            {
                writer->addStringElement(writer, GrowingCrashField_Type, GrowingCrashExcType_NSException);
                writer->beginObject(writer, GrowingCrashField_NSException);
                {
                    writer->addStringElement(writer, GrowingCrashField_Name, crash->NSException.name);
                    writer->addStringElement(writer, GrowingCrashField_UserInfo, crash->NSException.userInfo);
                    writeAddressReferencedByString(writer, GrowingCrashField_ReferencedObject, crash->crashReason);
                }
                writer->endContainer(writer);
                break;
            }
            case GrowingCrashMonitorTypeSignal:
                writer->addStringElement(writer, GrowingCrashField_Type, GrowingCrashExcType_Signal);
                break;

            case GrowingCrashMonitorTypeUserReported:
            {
                writer->addStringElement(writer, GrowingCrashField_Type, GrowingCrashExcType_User);
                writer->beginObject(writer, GrowingCrashField_UserReported);
                {
                    writer->addStringElement(writer, GrowingCrashField_Name, crash->userException.name);
                    if(crash->userException.language != NULL)
                    {
                        writer->addStringElement(writer, GrowingCrashField_Language, crash->userException.language);
                    }
                    if(crash->userException.lineOfCode != NULL)
                    {
                        writer->addStringElement(writer, GrowingCrashField_LineOfCode, crash->userException.lineOfCode);
                    }
                    if(crash->userException.customStackTrace != NULL)
                    {
                        writer->addJSONElement(writer, GrowingCrashField_Backtrace, crash->userException.customStackTrace, true);
                    }
                }
                writer->endContainer(writer);
                break;
            }
            case GrowingCrashMonitorTypeSystem:
            case GrowingCrashMonitorTypeApplicationState:
            case GrowingCrashMonitorTypeZombie:
                GrowingCrashLOG_ERROR("Crash monitor type 0x%x shouldn't be able to cause events!", crash->crashType);
                break;
        }
    }
    writer->endContainer(writer);
}

/** Write information about app runtime, etc to the report.
 *
 * @param writer The writer.
 *
 * @param key The object key, if needed.
 *
 * @param monitorContext The event monitor context.
 */
static void writeAppStats(const GrowingCrashReportWriter* const writer,
                          const char* const key,
                          const GrowingCrash_MonitorContext* const monitorContext)
{
    writer->beginObject(writer, key);
    {
        writer->addBooleanElement(writer, GrowingCrashField_AppActive, monitorContext->AppState.applicationIsActive);
        writer->addBooleanElement(writer, GrowingCrashField_AppInFG, monitorContext->AppState.applicationIsInForeground);

        writer->addIntegerElement(writer, GrowingCrashField_LaunchesSinceCrash, monitorContext->AppState.launchesSinceLastCrash);
        writer->addIntegerElement(writer, GrowingCrashField_SessionsSinceCrash, monitorContext->AppState.sessionsSinceLastCrash);
        writer->addFloatingPointElement(writer, GrowingCrashField_ActiveTimeSinceCrash, monitorContext->AppState.activeDurationSinceLastCrash);
        writer->addFloatingPointElement(writer, GrowingCrashField_BGTimeSinceCrash, monitorContext->AppState.backgroundDurationSinceLastCrash);

        writer->addIntegerElement(writer, GrowingCrashField_SessionsSinceLaunch, monitorContext->AppState.sessionsSinceLaunch);
        writer->addFloatingPointElement(writer, GrowingCrashField_ActiveTimeSinceLaunch, monitorContext->AppState.activeDurationSinceLaunch);
        writer->addFloatingPointElement(writer, GrowingCrashField_BGTimeSinceLaunch, monitorContext->AppState.backgroundDurationSinceLaunch);
    }
    writer->endContainer(writer);
}

/** Write information about this process.
 *
 * @param writer The writer.
 *
 * @param key The object key, if needed.
 */
static void writeProcessState(const GrowingCrashReportWriter* const writer,
                              const char* const key,
                              const GrowingCrash_MonitorContext* const monitorContext)
{
    writer->beginObject(writer, key);
    {
        if(monitorContext->ZombieException.address != 0)
        {
            writer->beginObject(writer, GrowingCrashField_LastDeallocedNSException);
            {
                writer->addUIntegerElement(writer, GrowingCrashField_Address, monitorContext->ZombieException.address);
                writer->addStringElement(writer, GrowingCrashField_Name, monitorContext->ZombieException.name);
                writer->addStringElement(writer, GrowingCrashField_Reason, monitorContext->ZombieException.reason);
                writeAddressReferencedByString(writer, GrowingCrashField_ReferencedObject, monitorContext->ZombieException.reason);
            }
            writer->endContainer(writer);
        }
    }
    writer->endContainer(writer);
}

/** Write basic report information.
 *
 * @param writer The writer.
 *
 * @param key The object key, if needed.
 *
 * @param type The report type.
 *
 * @param reportID The report ID.
 */
static void writeReportInfo(const GrowingCrashReportWriter* const writer,
                            const char* const key,
                            const char* const type,
                            const char* const reportID,
                            const char* const processName)
{
    writer->beginObject(writer, key);
    {
        struct timeval tp;
        gettimeofday(&tp, NULL);
        int64_t microseconds = ((int64_t)tp.tv_sec) * 1000000 + tp.tv_usec;
        
        writer->addStringElement(writer, GrowingCrashField_Version, GROWINGCRASH_REPORT_VERSION);
        writer->addStringElement(writer, GrowingCrashField_ID, reportID);
        writer->addStringElement(writer, GrowingCrashField_ProcessName, processName);
        writer->addIntegerElement(writer, GrowingCrashField_Timestamp, microseconds);
        writer->addStringElement(writer, GrowingCrashField_Type, type);
    }
    writer->endContainer(writer);
}

static void writeRecrash(const GrowingCrashReportWriter* const writer,
                         const char* const key,
                         const char* crashReportPath)
{
    writer->addJSONFileElement(writer, key, crashReportPath, true);
}


#pragma mark Setup

/** Prepare a report writer for use.
 *
 * @oaram writer The writer to prepare.
 *
 * @param context JSON writer contextual information.
 */
static void prepareReportWriter(GrowingCrashReportWriter* const writer, GrowingCrashJSONEncodeContext* const context)
{
    writer->addBooleanElement = addBooleanElement;
    writer->addFloatingPointElement = addFloatingPointElement;
    writer->addIntegerElement = addIntegerElement;
    writer->addUIntegerElement = addUIntegerElement;
    writer->addStringElement = addStringElement;
    writer->addTextFileElement = addTextFileElement;
    writer->addTextFileLinesElement = addTextLinesFromFile;
    writer->addJSONFileElement = addJSONElementFromFile;
    writer->addDataElement = addDataElement;
    writer->beginDataElement = beginDataElement;
    writer->appendDataElement = appendDataElement;
    writer->endDataElement = endDataElement;
    writer->addUUIDElement = addUUIDElement;
    writer->addJSONElement = addJSONElement;
    writer->beginObject = beginObject;
    writer->beginArray = beginArray;
    writer->endContainer = endContainer;
    writer->context = context;
}


// ============================================================================
#pragma mark - Main API -
// ============================================================================

void growingcrashreport_writeRecrashReport(const GrowingCrash_MonitorContext* const monitorContext, const char* const path)
{
    char writeBuffer[1024];
    GrowingCrashBufferedWriter bufferedWriter;
    static char tempPath[GROWINGCRASHFU_MAX_PATH_LENGTH];
    strncpy(tempPath, path, sizeof(tempPath) - 10);
    strncpy(tempPath + strlen(tempPath) - 5, ".old", 5);
    GrowingCrashLOG_INFO("Writing recrash report to %s", path);

    if(rename(path, tempPath) < 0)
    {
        GrowingCrashLOG_ERROR("Could not rename %s to %s: %s", path, tempPath, strerror(errno));
    }
    if(!growingcrashfu_openBufferedWriter(&bufferedWriter, path, writeBuffer, sizeof(writeBuffer)))
    {
        return;
    }

    growingccd_freeze();

    GrowingCrashJSONEncodeContext jsonContext;
    jsonContext.userData = &bufferedWriter;
    GrowingCrashReportWriter concreteWriter;
    GrowingCrashReportWriter* writer = &concreteWriter;
    prepareReportWriter(writer, &jsonContext);

    growingcrashjson_beginEncode(getJsonContext(writer), true, addJSONData, &bufferedWriter);

    writer->beginObject(writer, GrowingCrashField_Report);
    {
        writeRecrash(writer, GrowingCrashField_RecrashReport, tempPath);
        growingcrashfu_flushBufferedWriter(&bufferedWriter);
        if(remove(tempPath) < 0)
        {
            GrowingCrashLOG_ERROR("Could not remove %s: %s", tempPath, strerror(errno));
        }
        writeReportInfo(writer,
                        GrowingCrashField_Report,
                        GrowingCrashReportType_Minimal,
                        monitorContext->eventID,
                        monitorContext->System.processName);
        growingcrashfu_flushBufferedWriter(&bufferedWriter);

        writer->beginObject(writer, GrowingCrashField_Crash);
        {
            writeError(writer, GrowingCrashField_Error, monitorContext);
            growingcrashfu_flushBufferedWriter(&bufferedWriter);
            int threadIndex = growingcrashmc_indexOfThread(monitorContext->offendingMachineContext,
                                                 growingcrashmc_getThreadFromContext(monitorContext->offendingMachineContext));
            writeThread(writer,
                        GrowingCrashField_CrashedThread,
                        monitorContext,
                        monitorContext->offendingMachineContext,
                        threadIndex,
                        false);
            growingcrashfu_flushBufferedWriter(&bufferedWriter);
        }
        writer->endContainer(writer);
    }
    writer->endContainer(writer);

    growingcrashjson_endEncode(getJsonContext(writer));
    growingcrashfu_closeBufferedWriter(&bufferedWriter);
    growingccd_unfreeze();
}

static void writeSystemInfo(const GrowingCrashReportWriter* const writer,
                            const char* const key,
                            const GrowingCrash_MonitorContext* const monitorContext)
{
    writer->beginObject(writer, key);
    {
        writer->addStringElement(writer, GrowingCrashField_SystemName, monitorContext->System.systemName);
        writer->addStringElement(writer, GrowingCrashField_SystemVersion, monitorContext->System.systemVersion);
        writer->addStringElement(writer, GrowingCrashField_Machine, monitorContext->System.machine);
        writer->addStringElement(writer, GrowingCrashField_Model, monitorContext->System.model);
        writer->addStringElement(writer, GrowingCrashField_KernelVersion, monitorContext->System.kernelVersion);
        writer->addStringElement(writer, GrowingCrashField_OSVersion, monitorContext->System.osVersion);
        writer->addBooleanElement(writer, GrowingCrashField_Jailbroken, monitorContext->System.isJailbroken);
        writer->addStringElement(writer, GrowingCrashField_BootTime, monitorContext->System.bootTime);
        writer->addStringElement(writer, GrowingCrashField_AppStartTime, monitorContext->System.appStartTime);
        writer->addStringElement(writer, GrowingCrashField_ExecutablePath, monitorContext->System.executablePath);
        writer->addStringElement(writer, GrowingCrashField_Executable, monitorContext->System.executableName);
        writer->addStringElement(writer, GrowingCrashField_BundleID, monitorContext->System.bundleID);
        writer->addStringElement(writer, GrowingCrashField_BundleName, monitorContext->System.bundleName);
        writer->addStringElement(writer, GrowingCrashField_BundleVersion, monitorContext->System.bundleVersion);
        writer->addStringElement(writer, GrowingCrashField_BundleShortVersion, monitorContext->System.bundleShortVersion);
        writer->addStringElement(writer, GrowingCrashField_AppUUID, monitorContext->System.appID);
        writer->addStringElement(writer, GrowingCrashField_CPUArch, monitorContext->System.cpuArchitecture);
        writer->addIntegerElement(writer, GrowingCrashField_CPUType, monitorContext->System.cpuType);
        writer->addIntegerElement(writer, GrowingCrashField_CPUSubType, monitorContext->System.cpuSubType);
        writer->addIntegerElement(writer, GrowingCrashField_BinaryCPUType, monitorContext->System.binaryCPUType);
        writer->addIntegerElement(writer, GrowingCrashField_BinaryCPUSubType, monitorContext->System.binaryCPUSubType);
        writer->addStringElement(writer, GrowingCrashField_TimeZone, monitorContext->System.timezone);
        writer->addStringElement(writer, GrowingCrashField_ProcessName, monitorContext->System.processName);
        writer->addIntegerElement(writer, GrowingCrashField_ProcessID, monitorContext->System.processID);
        writer->addIntegerElement(writer, GrowingCrashField_ParentProcessID, monitorContext->System.parentProcessID);
        writer->addStringElement(writer, GrowingCrashField_DeviceAppHash, monitorContext->System.deviceAppHash);
        writer->addStringElement(writer, GrowingCrashField_BuildType, monitorContext->System.buildType);
        writer->addIntegerElement(writer, GrowingCrashField_Storage, (int64_t)monitorContext->System.storageSize);

        writeMemoryInfo(writer, GrowingCrashField_Memory, monitorContext);
        writeAppStats(writer, GrowingCrashField_AppStats, monitorContext);
    }
    writer->endContainer(writer);

}

static void writeDebugInfo(const GrowingCrashReportWriter* const writer,
                            const char* const key,
                            const GrowingCrash_MonitorContext* const monitorContext)
{
    writer->beginObject(writer, key);
    {
        if(monitorContext->consoleLogPath != NULL)
        {
            addTextLinesFromFile(writer, GrowingCrashField_ConsoleLog, monitorContext->consoleLogPath);
        }
    }
    writer->endContainer(writer);
    
}

void growingcrashreport_writeStandardReport(const GrowingCrash_MonitorContext* const monitorContext, const char* const path)
{
    GrowingCrashLOG_INFO("Writing crash report to %s", path);
    char writeBuffer[1024];
    GrowingCrashBufferedWriter bufferedWriter;

    if(!growingcrashfu_openBufferedWriter(&bufferedWriter, path, writeBuffer, sizeof(writeBuffer)))
    {
        return;
    }

    growingccd_freeze();
    
    GrowingCrashJSONEncodeContext jsonContext;
    jsonContext.userData = &bufferedWriter;
    GrowingCrashReportWriter concreteWriter;
    GrowingCrashReportWriter* writer = &concreteWriter;
    prepareReportWriter(writer, &jsonContext);

    growingcrashjson_beginEncode(getJsonContext(writer), true, addJSONData, &bufferedWriter);

    writer->beginObject(writer, GrowingCrashField_Report);
    {
        writeReportInfo(writer,
                        GrowingCrashField_Report,
                        GrowingCrashReportType_Standard,
                        monitorContext->eventID,
                        monitorContext->System.processName);
        growingcrashfu_flushBufferedWriter(&bufferedWriter);

        writeBinaryImages(writer, GrowingCrashField_BinaryImages);
        growingcrashfu_flushBufferedWriter(&bufferedWriter);

        writeProcessState(writer, GrowingCrashField_ProcessState, monitorContext);
        growingcrashfu_flushBufferedWriter(&bufferedWriter);

        writeSystemInfo(writer, GrowingCrashField_System, monitorContext);
        growingcrashfu_flushBufferedWriter(&bufferedWriter);

        writer->beginObject(writer, GrowingCrashField_Crash);
        {
            writeError(writer, GrowingCrashField_Error, monitorContext);
            growingcrashfu_flushBufferedWriter(&bufferedWriter);
            writeAllThreads(writer,
                            GrowingCrashField_Threads,
                            monitorContext,
                            g_introspectionRules.enabled);
            growingcrashfu_flushBufferedWriter(&bufferedWriter);
        }
        writer->endContainer(writer);

        if(g_userInfoJSON != NULL)
        {
            addJSONElement(writer, GrowingCrashField_User, g_userInfoJSON, false);
            growingcrashfu_flushBufferedWriter(&bufferedWriter);
        }
        else
        {
            writer->beginObject(writer, GrowingCrashField_User);
        }
        if(g_userSectionWriteCallback != NULL)
        {
            growingcrashfu_flushBufferedWriter(&bufferedWriter);
            if (monitorContext->currentSnapshotUserReported == false) {
                g_userSectionWriteCallback(writer);
            }
        }
        writer->endContainer(writer);
        growingcrashfu_flushBufferedWriter(&bufferedWriter);

        writeDebugInfo(writer, GrowingCrashField_Debug, monitorContext);
    }
    writer->endContainer(writer);
    
    growingcrashjson_endEncode(getJsonContext(writer));
    growingcrashfu_closeBufferedWriter(&bufferedWriter);
    growingccd_unfreeze();
}



void growingcrashreport_setUserInfoJSON(const char* const userInfoJSON)
{
    static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
    GrowingCrashLOG_TRACE("set userInfoJSON to %p", userInfoJSON);

    pthread_mutex_lock(&mutex);
    if(g_userInfoJSON != NULL)
    {
        free((void*)g_userInfoJSON);
    }
    if(userInfoJSON == NULL)
    {
        g_userInfoJSON = NULL;
    }
    else
    {
        g_userInfoJSON = strdup(userInfoJSON);
    }
    pthread_mutex_unlock(&mutex);
}

void growingcrashreport_setIntrospectMemory(bool shouldIntrospectMemory)
{
    g_introspectionRules.enabled = shouldIntrospectMemory;
}

void growingcrashreport_setDoNotIntrospectClasses(const char** doNotIntrospectClasses, int length)
{
    const char** oldClasses = g_introspectionRules.restrictedClasses;
    int oldClassesLength = g_introspectionRules.restrictedClassesCount;
    const char** newClasses = NULL;
    int newClassesLength = 0;
    
    if(doNotIntrospectClasses != NULL && length > 0)
    {
        newClassesLength = length;
        newClasses = malloc(sizeof(*newClasses) * (unsigned)newClassesLength);
        if(newClasses == NULL)
        {
            GrowingCrashLOG_ERROR("Could not allocate memory");
            return;
        }
        
        for(int i = 0; i < newClassesLength; i++)
        {
            newClasses[i] = strdup(doNotIntrospectClasses[i]);
        }
    }
    
    g_introspectionRules.restrictedClasses = newClasses;
    g_introspectionRules.restrictedClassesCount = newClassesLength;
    
    if(oldClasses != NULL)
    {
        for(int i = 0; i < oldClassesLength; i++)
        {
            free((void*)oldClasses[i]);
        }
        free(oldClasses);
    }
}

void growingcrashreport_setUserSectionWriteCallback(const GrowingCrashReportWriteCallback userSectionWriteCallback)
{
    GrowingCrashLOG_TRACE("Set userSectionWriteCallback to %p", userSectionWriteCallback);
    g_userSectionWriteCallback = userSectionWriteCallback;
}
