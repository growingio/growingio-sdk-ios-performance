//  Created by Karl Stenerud on 2012-02-05.
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
//  GrowingCrashReportStore.h
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

#ifndef HDR_GrowingCrashReportStore_h
#define HDR_GrowingCrashReportStore_h

#ifdef __cplusplus
extern "C" {
#endif


#include <stdint.h>

#define GROWINGCRS_MAX_PATH_LENGTH 500

/** Initialize the report store.
 *
 * @param appName The application's name.
 * @param reportsPath Full path to directory where the reports are to be stored (path will be created if needed).
 */
void growingcrs_initialize(const char* appName, const char* reportsPath);

/** Get the next crash report to be generated.
 * Max length for paths is GROWINGCRS_MAX_PATH_LENGTH
 *
 * @param crashReportPathBuffer Buffer to store the crash report path.
 *
 * @return the report ID of the next report.
 */
int64_t growingcrs_getNextCrashReport(char* crashReportPathBuffer);

/** Get the number of reports on disk.
 */
int growingcrs_getReportCount(void);

/** Get a list of IDs for all reports on disk.
 *
 * @param reportIDs An array big enough to hold all report IDs.
 * @param count How many reports the array can hold.
 *
 * @return The number of report IDs that were placed in the array.
 */
int growingcrs_getReportIDs(int64_t* reportIDs, int count);

/** Read a report.
 *
 * @param reportID The report's ID.
 *
 * @return The NULL terminated report, or NULL if not found.
 *         MEMORY MANAGEMENT WARNING: User is responsible for calling free() on the returned value.
 */
char* growingcrs_readReport(int64_t reportID);

/** Add a custom report to the store.
 *
 * @param report The report's contents (must be JSON encoded).
 * @param reportLength The length of the report in bytes.
 *
 * @return the new report's ID.
 */
int64_t growingcrs_addUserReport(const char* report, int reportLength);

/** Delete all reports on disk.
 */
void growingcrs_deleteAllReports(void);

/** Delete report.
 *
 * @param reportID An ID of report to delete.
 */
void growingcrs_deleteReportWithID(int64_t reportID);

/** Set the maximum number of reports allowed on disk before old ones get deleted.
 *
 * @param maxReportCount The maximum number of reports.
 */
    void growingcrs_setMaxReportCount(int maxReportCount);

#ifdef __cplusplus
}
#endif

#endif // HDR_GrowingCrashReportStore_h
