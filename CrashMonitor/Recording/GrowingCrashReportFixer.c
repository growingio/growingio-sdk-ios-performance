//  Created by Karl Stenerud on 2016-11-07.
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
//  GrowingCrashReportFixer.c
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

#include "GrowingCrashReportFields.h"
#include "GrowingCrashSystemCapabilities.h"
#include "GrowingCrashJSONCodec.h"
#include "GrowingCrashDemangle_CPP.h"
#if GROWINGCRASH_HAS_SWIFT
#include "GrowingCrashDemangle_Swift.h"
#endif
#include "GrowingCrashDate.h"
#include "GrowingCrashLogger.h"

#include <stdlib.h>
#include <string.h>

#define MAX_DEPTH 100
#define MAX_NAME_LENGTH 100
#define REPORT_VERSION_COMPONENTS_COUNT 3

static char* datePaths[][MAX_DEPTH] =
{
    {"", GrowingCrashField_Report, GrowingCrashField_Timestamp},
    {"", GrowingCrashField_RecrashReport, GrowingCrashField_Report, GrowingCrashField_Timestamp},
};
static int datePathsCount = sizeof(datePaths) / sizeof(*datePaths);

static char* demanglePaths[][MAX_DEPTH] =
{
    {"", GrowingCrashField_Crash, GrowingCrashField_Threads, "", GrowingCrashField_Backtrace, GrowingCrashField_Contents, "", GrowingCrashField_SymbolName},
    {"", GrowingCrashField_RecrashReport, GrowingCrashField_Crash, GrowingCrashField_Threads, "", GrowingCrashField_Backtrace, GrowingCrashField_Contents, "", GrowingCrashField_SymbolName},
    {"", GrowingCrashField_Crash, GrowingCrashField_Error, GrowingCrashField_CPPException, GrowingCrashField_Name},
    {"", GrowingCrashField_RecrashReport, GrowingCrashField_Crash, GrowingCrashField_Error, GrowingCrashField_CPPException, GrowingCrashField_Name},
};
static int demanglePathsCount = sizeof(demanglePaths) / sizeof(*demanglePaths);

static char* versionPaths[][MAX_DEPTH] =
{
    {"", GrowingCrashField_Report, GrowingCrashField_Version},
    {"", GrowingCrashField_RecrashReport, GrowingCrashField_Report, GrowingCrashField_Version},
};
static int versionPathsCount = sizeof(versionPaths) / sizeof(*versionPaths);

typedef struct
{
    GrowingCrashJSONEncodeContext* encodeContext;
    int reportVersionComponents[REPORT_VERSION_COMPONENTS_COUNT];
    char objectPath[MAX_DEPTH][MAX_NAME_LENGTH];
    int currentDepth;
    char* outputPtr;
    int outputBytesLeft;
} FixupContext;

static bool increaseDepth(FixupContext* context, const char* name)
{
    if(context->currentDepth >= MAX_DEPTH)
    {
        return false;
    }
    if(name == NULL)
    {
        *context->objectPath[context->currentDepth] = '\0';
    }
    else
    {
        strncpy(context->objectPath[context->currentDepth], name, sizeof(context->objectPath[context->currentDepth]));
    }
    context->currentDepth++;
    return true;
}

static bool decreaseDepth(FixupContext* context)
{
    if(context->currentDepth <= 0)
    {
        return false;
    }
    context->currentDepth--;
    return true;
}

static bool matchesPath(FixupContext* context, char** path, const char* finalName)
{
    if(finalName == NULL)
    {
        finalName = "";
    }

    for(int i = 0;i < context->currentDepth; i++)
    {
        if(strncmp(context->objectPath[i], path[i], MAX_NAME_LENGTH) != 0)
        {
            return false;
        }
    }
    if(strncmp(finalName, path[context->currentDepth], MAX_NAME_LENGTH) != 0)
    {
        return false;
    }
    return true;
}

static bool matchesAPath(FixupContext* context, const char* name, char* paths[][MAX_DEPTH], int pathsCount)
{
    for(int i = 0; i < pathsCount; i++)
    {
        if(matchesPath(context, paths[i], name))
        {
            return true;
        }
    }
    return false;
}

static bool matchesMinVersion(FixupContext* context, int major, int minor, int patch)
{
    // Works only for report version 3.1.0 and above. See GrowingCrashReportVersion.h
    bool result = false;
    int *parts = context->reportVersionComponents;
    result = result || (parts[0] > major);
    result = result || (parts[0] == major && parts[1] > minor);
    result = result || (parts[0] == major && parts[1] == minor && parts[2] >= patch);
    return result;
}

static bool shouldDemangle(FixupContext* context, const char* name)
{
    return matchesAPath(context, name, demanglePaths, demanglePathsCount);
}

static bool shouldFixDate(FixupContext* context, const char* name)
{
    return matchesAPath(context, name, datePaths, datePathsCount);
}

static bool shouldSaveVersion(FixupContext* context, const char* name)
{
    return matchesAPath(context, name, versionPaths, versionPathsCount);
}

static int onBooleanElement(const char* const name,
                            const bool value,
                            void* const userData)
{
    FixupContext* context = (FixupContext*)userData;
    return growingcrashjson_addBooleanElement(context->encodeContext, name, value);
}

static int onFloatingPointElement(const char* const name,
                                  const double value,
                                  void* const userData)
{
    FixupContext* context = (FixupContext*)userData;
    return growingcrashjson_addFloatingPointElement(context->encodeContext, name, value);
}

static int onIntegerElement(const char* const name,
                            const int64_t value,
                            void* const userData)
{
    FixupContext* context = (FixupContext*)userData;
    int result = GrowingCrashJSON_OK;
    if(shouldFixDate(context, name))
    {
        char buffer[28];

        if(matchesMinVersion(context, 3, 3, 0))
        {
            growingcrashdate_utcStringFromMicroseconds(value, buffer);
        }
        else
        {
            growingcrashdate_utcStringFromTimestamp((time_t)value, buffer);
        }

        result = growingcrashjson_addStringElement(context->encodeContext, name, buffer, (int)strlen(buffer));
    }
    else
    {
        result = growingcrashjson_addIntegerElement(context->encodeContext, name, value);
    }
    return result;
}

static int onNullElement(const char* const name,
                         void* const userData)
{
    FixupContext* context = (FixupContext*)userData;
    return growingcrashjson_addNullElement(context->encodeContext, name);
}

static int onStringElement(const char* const name,
                           const char* const value,
                           void* const userData)
{
    FixupContext* context = (FixupContext*)userData;
    const char* stringValue = value;
    char* demangled = NULL;
    if(shouldDemangle(context, name))
    {
        demangled = growingcrashdm_demangleCPP(value);
#if GROWINGCRASH_HAS_SWIFT
        if(demangled == NULL)
        {
            demangled = growingcrashdm_demangleSwift(value);
        }
#endif
        if(demangled != NULL)
        {
            stringValue = demangled;
        }
    }
    int result = growingcrashjson_addStringElement(context->encodeContext, name, stringValue, (int)strlen(stringValue));
    if(demangled != NULL)
    {
        free(demangled);
    }
    if(shouldSaveVersion(context, name))
    {
        memset(context->reportVersionComponents, 0, sizeof(context->reportVersionComponents));
        int versionPartsIndex = 0;
        char* mutableValue = strdup(value);
        char* versionPart = strtok(mutableValue, ".");
        while(versionPart != NULL && versionPartsIndex < REPORT_VERSION_COMPONENTS_COUNT)
        {
            context->reportVersionComponents[versionPartsIndex++] = atoi(versionPart);
            versionPart = strtok(NULL, ".");
        }
        free(mutableValue);
    }
    return result;
}

static int onBeginObject(const char* const name,
                         void* const userData)
{
    FixupContext* context = (FixupContext*)userData;
    int result = growingcrashjson_beginObject(context->encodeContext, name);
    if(!increaseDepth(context, name))
    {
        return GrowingCrashJSON_ERROR_DATA_TOO_LONG;
    }
    return result;
}

static int onBeginArray(const char* const name,
                        void* const userData)
{
    FixupContext* context = (FixupContext*)userData;
    int result = growingcrashjson_beginArray(context->encodeContext, name);
    if(!increaseDepth(context, name))
    {
        return GrowingCrashJSON_ERROR_DATA_TOO_LONG;
    }
    return result;
}

static int onEndContainer(void* const userData)
{
    FixupContext* context = (FixupContext*)userData;
    int result = growingcrashjson_endContainer(context->encodeContext);
    if(!decreaseDepth(context))
    {
        // Do something;
    }
    return result;
}

static int onEndData(__unused void* const userData)
{
    FixupContext* context = (FixupContext*)userData;
    return growingcrashjson_endEncode(context->encodeContext);
}

static int addJSONData(const char* data, int length, void* userData)
{
    FixupContext* context = (FixupContext*)userData;
    if(length > context->outputBytesLeft)
    {
        return GrowingCrashJSON_ERROR_DATA_TOO_LONG;
    }
    memcpy(context->outputPtr, data, length);
    context->outputPtr += length;
    context->outputBytesLeft -= length;
    
    return GrowingCrashJSON_OK;
}

char* growingcrf_fixupCrashReport(const char* crashReport)
{
    if(crashReport == NULL)
    {
        return NULL;
    }

    GrowingCrashJSONDecodeCallbacks callbacks =
    {
        .onBeginArray = onBeginArray,
        .onBeginObject = onBeginObject,
        .onBooleanElement = onBooleanElement,
        .onEndContainer = onEndContainer,
        .onEndData = onEndData,
        .onFloatingPointElement = onFloatingPointElement,
        .onIntegerElement = onIntegerElement,
        .onNullElement = onNullElement,
        .onStringElement = onStringElement,
    };
    int stringBufferLength = 10000;
    char* stringBuffer = malloc((unsigned)stringBufferLength);
    int crashReportLength = (int)strlen(crashReport);
    int fixedReportLength = (int)(crashReportLength * 1.5);
    char* fixedReport = malloc((unsigned)fixedReportLength);
    GrowingCrashJSONEncodeContext encodeContext;
    FixupContext fixupContext =
    {
        .encodeContext = &encodeContext,
        .reportVersionComponents = {0},
        .currentDepth = 0,
        .outputPtr = fixedReport,
        .outputBytesLeft = fixedReportLength,
    };
    
    growingcrashjson_beginEncode(&encodeContext, true, addJSONData, &fixupContext);
    
    int errorOffset = 0;
    int result = growingcrashjson_decode(crashReport, (int)strlen(crashReport), stringBuffer, stringBufferLength, &callbacks, &fixupContext, &errorOffset);
    *fixupContext.outputPtr = '\0';
    free(stringBuffer);
    if(result != GrowingCrashJSON_OK)
    {
        GrowingCrashLOG_ERROR("Could not decode report: %s", growingcrashjson_stringForError(result));
        free(fixedReport);
        return NULL;
    }
    return fixedReport;
}
