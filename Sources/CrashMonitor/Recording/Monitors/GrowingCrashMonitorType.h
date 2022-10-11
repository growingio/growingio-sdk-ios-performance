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
//  GrowingCrashMonitorType.h
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


#ifndef HDR_GrowingCrashMonitorType_h
#define HDR_GrowingCrashMonitorType_h

#ifdef __cplusplus
extern "C" {
#endif


/** Various aspects of the system that can be monitored:
 * - Mach kernel exception
 * - Fatal signal
 * - Uncaught C++ exception
 * - Uncaught Objective-C NSException
 * - Deadlock on the main thread
 * - User reported custom exception
 */
typedef enum
{
    /* Captures and reports Mach exceptions. */
    GrowingCrashMonitorTypeMachException      = 0x01,
    
    /* Captures and reports POSIX signals. */
    GrowingCrashMonitorTypeSignal             = 0x02,
    
    /* Captures and reports C++ exceptions.
     * Note: This will slightly slow down exception processing.
     */
    GrowingCrashMonitorTypeCPPException       = 0x04,
    
    /* Captures and reports NSExceptions. */
    GrowingCrashMonitorTypeNSException        = 0x08,
    
    /* Detects and reports a deadlock in the main thread. */
    GrowingCrashMonitorTypeMainThreadDeadlock = 0x10,
    
    /* Accepts and reports user-generated exceptions. */
    GrowingCrashMonitorTypeUserReported       = 0x20,
    
    /* Keeps track of and injects system information. */
    GrowingCrashMonitorTypeSystem             = 0x40,
    
    /* Keeps track of and injects application state. */
    GrowingCrashMonitorTypeApplicationState   = 0x80,
    
    /* Keeps track of zombies, and injects the last zombie NSException. */
    GrowingCrashMonitorTypeZombie             = 0x100,
} GrowingCrashMonitorType;

#define GrowingCrashMonitorTypeAll              \
(                                          \
    GrowingCrashMonitorTypeMachException      | \
    GrowingCrashMonitorTypeSignal             | \
    GrowingCrashMonitorTypeCPPException       | \
    GrowingCrashMonitorTypeNSException        | \
    GrowingCrashMonitorTypeMainThreadDeadlock | \
    GrowingCrashMonitorTypeUserReported       | \
    GrowingCrashMonitorTypeSystem             | \
    GrowingCrashMonitorTypeApplicationState   | \
    GrowingCrashMonitorTypeZombie               \
)

#define GrowingCrashMonitorTypeExperimental     \
(                                          \
    GrowingCrashMonitorTypeMainThreadDeadlock   \
)

#define GrowingCrashMonitorTypeDebuggerUnsafe   \
(                                          \
    GrowingCrashMonitorTypeMachException      | \
    GrowingCrashMonitorTypeSignal             | \
    GrowingCrashMonitorTypeCPPException       | \
    GrowingCrashMonitorTypeNSException          \
)

#define GrowingCrashMonitorTypeAsyncSafe        \
(                                          \
    GrowingCrashMonitorTypeMachException      | \
    GrowingCrashMonitorTypeSignal               \
)

#define GrowingCrashMonitorTypeOptional         \
(                                          \
    GrowingCrashMonitorTypeZombie               \
)
    
#define GrowingCrashMonitorTypeAsyncUnsafe (GrowingCrashMonitorTypeAll & (~GrowingCrashMonitorTypeAsyncSafe))

/** Monitors that are safe to enable in a debugger. */
#define GrowingCrashMonitorTypeDebuggerSafe (GrowingCrashMonitorTypeAll & (~GrowingCrashMonitorTypeDebuggerUnsafe))

/** Monitors that are safe to use in a production environment.
 * All other monitors should be considered experimental.
 */
#define GrowingCrashMonitorTypeProductionSafe (GrowingCrashMonitorTypeAll & (~GrowingCrashMonitorTypeExperimental))

/** Production safe monitors, minus the optional ones. */
#define GrowingCrashMonitorTypeProductionSafeMinimal (GrowingCrashMonitorTypeProductionSafe & (~GrowingCrashMonitorTypeOptional))

/** Monitors that are required for proper operation.
 * These add essential information to the reports, but do not trigger reporting.
 */
#define GrowingCrashMonitorTypeRequired (GrowingCrashMonitorTypeSystem | GrowingCrashMonitorTypeApplicationState)

/** Effectively disables automatica reporting. The only way to generate a report
 * in this mode is by manually calling growingcrash_reportUserException().
 */
#define GrowingCrashMonitorTypeManual (GrowingCrashMonitorTypeRequired | GrowingCrashMonitorTypeUserReported)

#define GrowingCrashMonitorTypeNone 0

const char* growingcrashmonitortype_name(GrowingCrashMonitorType monitorType);


#ifdef __cplusplus
}
#endif

#endif // HDR_GrowingCrashMonitorType_h
