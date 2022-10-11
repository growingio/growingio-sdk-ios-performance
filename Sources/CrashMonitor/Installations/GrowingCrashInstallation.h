//  Created by Karl Stenerud on 2013-02-10.
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
//  GrowingCrashInstallation.h
//  GrowingAnalytics
//
//  Created by YoloMao on 2022/9/23.
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


#import <Foundation/Foundation.h>
#import "GrowingCrashReportFilter.h"
#import "GrowingCrashReportWriter.h"


/**
 * Crash system installation which handles backend-specific details.
 *
 * Only one installation can be installed at a time.
 *
 * This is an abstract class.
 */
@interface GrowingCrashInstallation : NSObject

/** C Function to call during a crash report to give the callee an opportunity to
 * add to the report. NULL = ignore.
 *
 * WARNING: Only call async-safe functions from this function! DO NOT call
 * Objective-C methods!!!
 */
@property(atomic,readwrite,assign) GrowingCrashReportWriteCallback onCrash;

/** Install this installation. Call this instead of -[GrowingCrash install] to install
 * with everything needed for your particular backend.
 */
- (void) install;

/** Convenience method to call -[GrowingCrash sendAllReportsWithCompletion:].
 * This method will set the GrowingCrash sink and then send all outstanding reports.
 *
 * Note: Pay special attention to GrowingCrash's "deleteBehaviorAfterSendAll" property.
 *
 * @param onCompletion Called when sending is complete (nil = ignore).
 */
- (void) sendAllReportsWithCompletion:(GrowingCrashReportFilterCompletion) onCompletion;

/** Add a filter that gets executed before all normal filters.
 * Prepended filters will be executed in the order in which they were added.
 *
 * @param filter the filter to prepend.
 */
- (void) addPreFilter:(id<GrowingCrashReportFilter>) filter;

@end
