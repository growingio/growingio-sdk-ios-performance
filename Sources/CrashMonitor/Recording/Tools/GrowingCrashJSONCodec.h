//  Created by Karl Stenerud on 2012-01-07.
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
//  GrowingCrashJSONCodec.h
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


/* Reads and writes JSON encoded data.
 */


#ifndef HDR_GrowingCrashJSONCodec_h
#define HDR_GrowingCrashJSONCodec_h

#ifdef __cplusplus
extern "C" {
#endif


#include <stdbool.h>
#include <stdint.h>

/* Tells the encoder to automatically determine the length of a field value.
 * Currently, this is done using strlen().
 */
#define GrowingCrashJSON_SIZE_AUTOMATIC -1

enum
{
    /** Encoding or decoding: Everything completed without error */
    GrowingCrashJSON_OK = 0,

    /** Encoding or decoding: Encountered an unexpected or invalid character */
    GrowingCrashJSON_ERROR_INVALID_CHARACTER = 1,

    /** Decoding: Source data was too long. */
    GrowingCrashJSON_ERROR_DATA_TOO_LONG = 2,

    /** Encoding: addJSONData could not handle the data.
     * This code is not used by the decoder, but is meant to be returned by
     * the addJSONData callback method if it couldn't handle the data.
     */
    GrowingCrashJSON_ERROR_CANNOT_ADD_DATA = 3,

    /** Decoding: Source data appears to be truncated. */
    GrowingCrashJSON_ERROR_INCOMPLETE = 4,

    /** Decoding: Parsing failed due to bad data structure/type/contents.
     * This code is not used by the decoder, but is meant to be returned
     * by the user callback methods if the decoded data is incorrect for
     * semantic or structural reasons.
     */
    GrowingCrashJSON_ERROR_INVALID_DATA = 5,
};

/** Get a description for an error code.
 *
 * @param error The error code.
 *
 * @return A string describing the error.
 */
const char* growingcrashjson_stringForError(const int error);


// ============================================================================
// Encode
// ============================================================================

/** Function pointer for adding more UTF-8 encoded JSON data.
 *
 * @param data The UTF-8 data to add.
 *
 * @param length The length of the data.
 *
 * @param userData user-specified contextual data.
 *
 * @return GrowingCrashJSON_OK if the data was handled.
 *         otherwise GrowingCrashJSON_ERROR_CANNOT_ADD_DATA.
 */
typedef int (*GrowingCrashJSONAddDataFunc)(const char* data, int length, void* userData);

typedef struct
{
    /** Function to call to add more encoded JSON data. */
    GrowingCrashJSONAddDataFunc addJSONData;

    /** User-specified data */
    void* userData;

    /** How many containers deep we are. */
    int containerLevel;

    /** Whether or not the current container is an object. */
    bool isObject[200];

    /** true if this is the first entry at the current container level. */
    bool containerFirstEntry;

    bool prettyPrint;

} GrowingCrashJSONEncodeContext;


/** Begin a new encoding process.
 *
 * @param context The encoding context.
 *
 * @param prettyPrint If true, insert whitespace to make the output pretty.
 *
 * @param addJSONData Function to handle adding data.
 *
 * @param userData User-specified data which gets passed to addJSONData.
 */
void growingcrashjson_beginEncode(GrowingCrashJSONEncodeContext* context,
                        bool prettyPrint,
                        GrowingCrashJSONAddDataFunc addJSONData,
                        void* userData);

/** End the encoding process, ending any remaining open containers.
 *
 * @return GrowingCrashJSON_OK if the process was successful.
 */
int growingcrashjson_endEncode(GrowingCrashJSONEncodeContext* context);

/** Add a boolean element.
 *
 * @param context The encoding context.
 *
 * @param name The element's name.
 *
 * @param value The element's value.
 *
 * @return GrowingCrashJSON_OK if the process was successful.
 */
int growingcrashjson_addBooleanElement(GrowingCrashJSONEncodeContext* context,
                             const char* name,
                             bool value);

/** Add an integer element.
 *
 * @param context The encoding context.
 *
 * @param name The element's name.
 *
 * @param value The element's value.
 *
 * @return GrowingCrashJSON_OK if the process was successful.
 */
int growingcrashjson_addIntegerElement(GrowingCrashJSONEncodeContext* context,
                             const char* name,
                             int64_t value);

/** Add an unsigned integer element.
 *
 * @param context The encoding context.
 *
 * @param name The element's name.
 *
 * @param value The element's value.
 *
 * @return GrowingCrashJSON_OK if the process was successful.
 */
int growingcrashjson_addUIntegerElement(GrowingCrashJSONEncodeContext* const context,
                              const char* const name,
                              uint64_t value);
    
/** Add a floating point element.
 *
 * @param context The encoding context.
 *
 * @param name The element's name.
 *
 * @param value The element's value.
 *
 * @return GrowingCrashJSON_OK if the process was successful.
 */
int growingcrashjson_addFloatingPointElement(GrowingCrashJSONEncodeContext* context,
                                   const char* name,
                                   double value);

/** Add a null element.
 *
 * @param context The encoding context.
 *
 * @param name The element's name.
 *
 * @return GrowingCrashJSON_OK if the process was successful.
 */
int growingcrashjson_addNullElement(GrowingCrashJSONEncodeContext* context,
                          const char* name);

/** Add a string element.
 *
 * @param context The encoding context.
 *
 * @param name The element's name.
 *
 * @param value The element's value.
 *
 * @param length the length of the string, or GrowingCrashJSON_SIZE_AUTOMATIC.
 *
 * @return GrowingCrashJSON_OK if the process was successful.
 */
int growingcrashjson_addStringElement(GrowingCrashJSONEncodeContext* context,
                            const char* name,
                            const char* value,
                            int length);

/** Start an incrementally-built string element.
 *
 * Use this for constructing very large strings.
 *
 * @param context The encoding context.
 *
 * @param name The element's name.
 *
 * @return GrowingCrashJSON_OK if the process was successful.
 */
int growingcrashjson_beginStringElement(GrowingCrashJSONEncodeContext* context,
                              const char* name);

/** Add a string fragment to an incrementally-built string element.
 *
 * @param context The encoding context.
 *
 * @param value The string fragment.
 *
 * @param length the length of the string fragment.
 *
 * @return GrowingCrashJSON_OK if the process was successful.
 */
int growingcrashjson_appendStringElement(GrowingCrashJSONEncodeContext* context,
                               const char* value,
                               int length);

/** End an incrementally-built string element.
 *
 * @param context The encoding context.
 *
 * @return GrowingCrashJSON_OK if the process was successful.
 */
int growingcrashjson_endStringElement(GrowingCrashJSONEncodeContext* context);

/** Add a string element. The element will be converted to string-coded hex.
 *
 * @param context The encoding context.
 *
 * @param name The element's name.
 *
 * @param value The element's value.
 *
 * @param length The length of the data.
 *
 * @return GrowingCrashJSON_OK if the process was successful.
 */
int growingcrashjson_addDataElement(GrowingCrashJSONEncodeContext* const context,
                          const char* name,
                          const char* value,
                          int length);

/** Start an incrementally-built data element. The element will be converted
 * to string-coded hex.
 *
 * Use this for constructing very large data elements.
 *
 * @param context The encoding context.
 *
 * @param name The element's name.
 *
 * @return GrowingCrashJSON_OK if the process was successful.
 */
int growingcrashjson_beginDataElement(GrowingCrashJSONEncodeContext* const context,
                            const char* const name);

/** Add a data fragment to an incrementally-built data element.
 *
 * @param context The encoding context.
 *
 * @param value The data fragment.
 *
 * @param length the length of the data fragment.
 *
 * @return GrowingCrashJSON_OK if the process was successful.
 */
int growingcrashjson_appendDataElement(GrowingCrashJSONEncodeContext* const context,
                             const char* const value,
                             int length);

/** End an incrementally-built data element.
 *
 * @param context The encoding context.
 *
 * @return GrowingCrashJSON_OK if the process was successful.
 */
int growingcrashjson_endDataElement(GrowingCrashJSONEncodeContext* const context);

/** Add a pre-formatted JSON element.
 *
 * @param encodeContext The encoding context.
 *
 * @param name The element's name.
 *
 * @param jsonData The element's value. MUST BE VALID JSON!
 *
 * @param jsonDataLength The length of the element.
 *
 * @param closeLastContainer If false, do not close the last container.
 *
 * @return GrowingCrashJSON_OK if the process was successful.
 */
int growingcrashjson_addJSONElement(GrowingCrashJSONEncodeContext* const encodeContext,
                          const char* restrict const name,
                          const char* restrict const jsonData,
                          const int jsonDataLength,
                          const bool closeLastContainer);

/** Begin a new object container.
 *
 * @param context The encoding context.
 *
 * @param name The object's name.
 *
 * @return GrowingCrashJSON_OK if the process was successful.
 */
int growingcrashjson_beginObject(GrowingCrashJSONEncodeContext* context,
                       const char* name);

/** Begin a new array container.
 *
 * @param context The encoding context.
 *
 * @param name The array's name.
 *
 * @return GrowingCrashJSON_OK if the process was successful.
 */
int growingcrashjson_beginArray(GrowingCrashJSONEncodeContext* context,
                      const char* name);

/** Begin a generic JSON element, adding any necessary JSON preamble text,
 *  including commas and names.
 *  Note: This does not add any object or array specifiers ('{', '[').
 *
 * @param context The JSON context.
 *
 * @param name The name of the next element (only needed if parent is a dictionary).
 */
int growingcrashjson_beginElement(GrowingCrashJSONEncodeContext* const context,
                        const char* const name);

/** Add JSON data manually.
 * This function just passes your data directly through, even if it's malforned.
 *
 * @param context The encoding context.
 *
 * @param data The data to write.
 *
 * @param length The length of the data.
 *
 * @return GrowingCrashJSON_OK if the process was successful.
 */
int growingcrashjson_addRawJSONData(GrowingCrashJSONEncodeContext* const context,
                          const char* const data,
                          const int length);

/** End the current container and return to the next higher level.
 *
 * @param context The encoding context.
 *
 * @return GrowingCrashJSON_OK if the process was successful.
 */
int growingcrashjson_endContainer(GrowingCrashJSONEncodeContext* context);

/** Decode and add JSON data from a file.
 *
 * @param context The encoding context.
 *
 * @param name The name to give the top element from the file.
 *
 * @param filename The file to read from.
 *
 * @param closeLastContainer If false, do not close the last container.
 */
int growingcrashjson_addJSONFromFile(GrowingCrashJSONEncodeContext* const context,
                           const char* restrict const name,
                           const char* restrict const filename,
                           const bool closeLastContainer);


// ============================================================================
// Decode
// ============================================================================


/**
 * Callbacks called during a JSON decode process.
 * All function pointers must point to valid functions.
 */
typedef struct GrowingCrashJSONDecodeCallbacks
{
    /** Called when a boolean element is decoded.
     *
     * @param name The element's name.
     *
     * @param value The element's value.
     *
     * @param userData Data that was specified when calling growingcrashjson_decode().
     *
     * @return GrowingCrashJSON_OK if decoding should continue.
     */
    int (*onBooleanElement)(const char* name,
                            bool value,
                            void* userData);

    /** Called when a floating point element is decoded.
     *
     * @param name The element's name.
     *
     * @param value The element's value.
     *
     * @param userData Data that was specified when calling growingcrashjson_decode().
     *
     * @return GrowingCrashJSON_OK if decoding should continue.
     */
    int (*onFloatingPointElement)(const char* name,
                                  double value,
                                  void* userData);

    /** Called when an integer element is decoded.
     *
     * @param name The element's name.
     *
     * @param value The element's value.
     *
     * @param userData Data that was specified when calling growingcrashjson_decode().
     *
     * @return GrowingCrashJSON_OK if decoding should continue.
     */
    int (*onIntegerElement)(const char* name,
                            int64_t value,
                            void* userData);

    /** Called when a null element is decoded.
     *
     * @param name The element's name.
     *
     * @param userData Data that was specified when calling growingcrashjson_decode().
     *
     * @return GrowingCrashJSON_OK if decoding should continue.
     */
    int (*onNullElement)(const char* name,
                         void* userData);

    /** Called when a string element is decoded.
     *
     * @param name The element's name.
     *
     * @param value The element's value.
     *
     * @param userData Data that was specified when calling growingcrashjson_decode().
     *
     * @return GrowingCrashJSON_OK if decoding should continue.
     */
    int (*onStringElement)(const char* name,
                           const char* value,
                           void* userData);

    /** Called when a new object is encountered.
     *
     * @param name The object's name.
     *
     * @param userData Data that was specified when calling growingcrashjson_decode().
     *
     * @return GrowingCrashJSON_OK if decoding should continue.
     */
    int (*onBeginObject)(const char* name,
                         void* userData);

    /** Called when a new array is encountered.
     *
     * @param name The array's name.
     *
     * @param userData Data that was specified when calling growingcrashjson_decode().
     *
     * @return GrowingCrashJSON_OK if decoding should continue.
     */
    int (*onBeginArray)(const char* name,
                        void* userData);

    /** Called when leaving the current container and returning to the next
     * higher level container.
     *
     * @param userData Data that was specified when calling growingcrashjson_decode().
     *
     * @return GrowingCrashJSON_OK if decoding should continue.
     */
    int (*onEndContainer)(void* userData);

    /** Called when the end of the input data is reached.
     *
     * @param userData Data that was specified when calling growingcrashjson_decode().
     *
     * @return GrowingCrashJSON_OK if decoding should continue.
     */
    int (*onEndData)(void* userData);

} GrowingCrashJSONDecodeCallbacks;


/** Read a JSON encoded file from the specified FD.
 *
 * @param data UTF-8 encoded JSON data.
 *
 * @param length Length of the data.
 *
 * @param stringBuffer A buffer to use for decoding strings.
 *                     Note: 1/4 of this buffer will be used for dictionary name decoding.
 *
 * @param stringBufferLength The length of the string buffer.
 *
 * @param callbacks The callbacks to call while decoding.
 *
 * @param userData Any data you would like passed to the callbacks.
 *
 * @oaram errorOffset If not null, will contain the offset into the data
 *                    where the error (if any) occurred.
 *
 * @return GrowingCrashJSON_OK if succesful. An error code otherwise.
 */
int growingcrashjson_decode(const char* data,
                  int length,
                  char* stringBuffer,
                  int stringBufferLength,
                  GrowingCrashJSONDecodeCallbacks* callbacks,
                  void* userData,
                  int* errorOffset);


#ifdef __cplusplus
}
#endif

#endif // HDR_GrowingCrashJSONCodec_h
