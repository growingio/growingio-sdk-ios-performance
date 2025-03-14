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
//  GrowingCrashReportWriter.h
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


/* Pointers to functions for writing to a crash report. All JSON types are
 * supported.
 */


#ifndef HDR_GrowingCrashReportWriter_h
#define HDR_GrowingCrashReportWriter_h

#ifdef __cplusplus
extern "C" {
#endif


#include <stdbool.h>
#include <stdint.h>


/**
 * Encapsulates report writing functionality.
 */
typedef struct GrowingCrashReportWriter
{
    /** Add a boolean element to the report.
     *
     * @param writer This writer.
     *
     * @param name The name to give this element.
     *
     * @param value The value to add.
     */
    void (*addBooleanElement)(const struct GrowingCrashReportWriter* writer,
                              const char* name,
                              bool value);

    /** Add a floating point element to the report.
     *
     * @param writer This writer.
     *
     * @param name The name to give this element.
     *
     * @param value The value to add.
     */
    void (*addFloatingPointElement)(const struct GrowingCrashReportWriter* writer,
                                    const char* name,
                                    double value);

    /** Add an integer element to the report.
     *
     * @param writer This writer.
     *
     * @param name The name to give this element.
     *
     * @param value The value to add.
     */
    void (*addIntegerElement)(const struct GrowingCrashReportWriter* writer,
                              const char* name,
                              int64_t value);

    /** Add an unsigned integer element to the report.
     *
     * @param writer This writer.
     *
     * @param name The name to give this element.
     *
     * @param value The value to add.
     */
    void (*addUIntegerElement)(const struct GrowingCrashReportWriter* writer,
                               const char* name,
                               uint64_t value);

    /** Add a string element to the report.
     *
     * @param writer This writer.
     *
     * @param name The name to give this element.
     *
     * @param value The value to add.
     */
    void (*addStringElement)(const struct GrowingCrashReportWriter* writer,
                             const char* name,
                             const char* value);

    /** Add a string element from a text file to the report.
     *
     * @param writer This writer.
     *
     * @param name The name to give this element.
     *
     * @param filePath The path to the file containing the value to add.
     */
    void (*addTextFileElement)(const struct GrowingCrashReportWriter* writer,
                               const char* name,
                               const char* filePath);

    /** Add an array of string elements representing lines from a text file to the report.
     *
     * @param writer This writer.
     *
     * @param name The name to give this element.
     *
     * @param filePath The path to the file containing the value to add.
     */
    void (*addTextFileLinesElement)(const struct GrowingCrashReportWriter* writer,
                                    const char* name,
                                    const char* filePath);

    /** Add a JSON element from a text file to the report.
     *
     * @param writer This writer.
     *
     * @param name The name to give this element.
     *
     * @param filePath The path to the file containing the value to add.
     *
     * @param closeLastContainer If false, do not close the last container.
     */
    void (*addJSONFileElement)(const struct GrowingCrashReportWriter* writer,
                               const char* name,
                               const char* filePath,
                               const bool closeLastContainer);
    
    /** Add a hex encoded data element to the report.
     *
     * @param writer This writer.
     *
     * @param name The name to give this element.
     *
     * @param value A pointer to the binary data.
     *
     * @paramn length The length of the data.
     */
    void (*addDataElement)(const struct GrowingCrashReportWriter* writer,
                           const char* name,
                           const char* value,
                           const int length);

    /** Begin writing a hex encoded data element to the report.
     *
     * @param writer This writer.
     *
     * @param name The name to give this element.
     */
    void (*beginDataElement)(const struct GrowingCrashReportWriter* writer,
                             const char* name);

    /** Append hex encoded data to the current data element in the report.
     *
     * @param writer This writer.
     *
     * @param value A pointer to the binary data.
     *
     * @paramn length The length of the data.
     */
    void (*appendDataElement)(const struct GrowingCrashReportWriter* writer,
                              const char* value,
                              const int length);

    /** Complete writing a hex encoded data element to the report.
     *
     * @param writer This writer.
     */
    void (*endDataElement)(const struct GrowingCrashReportWriter* writer);

    /** Add a UUID element to the report.
     *
     * @param writer This writer.
     *
     * @param name The name to give this element.
     *
     * @param value A pointer to the binary UUID data.
     */
    void (*addUUIDElement)(const struct GrowingCrashReportWriter* writer,
                           const char* name,
                           const unsigned char* value);

    /** Add a preformatted JSON element to the report.
     *
     * @param writer This writer.
     *
     * @param name The name to give this element.
     *
     * @param value A pointer to the JSON data.
     */
    void (*addJSONElement)(const struct GrowingCrashReportWriter* writer,
                           const char* name,
                           const char* jsonElement,
                           bool closeLastContainer);

    /** Begin a new object container.
     *
     * @param writer This writer.
     *
     * @param name The name to give this element.
     */
    void (*beginObject)(const struct GrowingCrashReportWriter* writer,
                        const char* name);

    /** Begin a new array container.
     *
     * @param writer This writer.
     *
     * @param name The name to give this element.
     */
    void (*beginArray)(const struct GrowingCrashReportWriter* writer,
                       const char* name);

    /** Leave the current container, returning to the next higher level
     *  container.
     *
     * @param writer This writer.
     */
    void (*endContainer)(const struct GrowingCrashReportWriter* writer);

    /** Internal contextual data for the writer */
    void* context;

} GrowingCrashReportWriter;

typedef void (*GrowingCrashReportWriteCallback)(const GrowingCrashReportWriter* writer);


#ifdef __cplusplus
}
#endif

#endif // HDR_GrowingCrashReportWriter_h

