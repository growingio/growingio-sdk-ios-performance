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
//  GrowingCrashReport.h
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


/* Writes a crash report to disk.
 */


#ifndef HDR_GrowingCrashReport_h
#define HDR_GrowingCrashReport_h

#ifdef __cplusplus
extern "C" {
#endif

#import "GrowingCrashReportWriter.h"
#import "GrowingCrashMonitorContext.h"

#include <stdbool.h>


// ============================================================================
#pragma mark - Configuration -
// ============================================================================
    
/** Set custom user information to be stored in the report.
 *
 * @param userInfoJSON The user information, in JSON format.
 */
void growingcrashreport_setUserInfoJSON(const char* const userInfoJSON);

/** Configure whether to introspect any interesting memory locations.
 *  This can find things like strings or Objective-C classes.
 *
 * @param shouldIntrospectMemory If true, introspect memory.
 */
void growingcrashreport_setIntrospectMemory(bool shouldIntrospectMemory);

/** Specify which objective-c classes should not be introspected.
 *
 * @param doNotIntrospectClasses Array of class names.
 * @param length Length of the array.
 */
void growingcrashreport_setDoNotIntrospectClasses(const char** doNotIntrospectClasses, int length);

/** Set the function to call when writing the user section of the report.
 *  This allows the user to add more fields to the user section at the time of the crash.
 *  Note: Only async-safe functions are allowed in the callback.
 *
 * @param userSectionWriteCallback The user section write callback.
 */
void growingcrashreport_setUserSectionWriteCallback(const GrowingCrashReportWriteCallback userSectionWriteCallback);


// ============================================================================
#pragma mark - Main API -
// ============================================================================
    
/** Write a standard crash report to a file.
 *
 * @param monitorContext Contextual information about the crash and environment.
 *                       The caller must fill this out before passing it in.
 *
 * @param path The file to write to.
 */
void growingcrashreport_writeStandardReport(const struct GrowingCrash_MonitorContext* const monitorContext,
                                       const char* path);

/** Write a minimal crash report to a file.
 *
 * @param monitorContext Contextual information about the crash and environment.
 *                       The caller must fill this out before passing it in.
 *
 * @param path The file to write to.
 */
void growingcrashreport_writeRecrashReport(const struct GrowingCrash_MonitorContext* const monitorContext,
                                      const char* path);


#ifdef __cplusplus
}
#endif

#endif // HDR_GrowingCrashReport_h
