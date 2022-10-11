//  Created by Karl Stenerud on 2012-12-09.
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
//  GrowingCrashMonitor_Deadlock.m
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

#import "GrowingCrashMonitor_Deadlock.h"
#import "GrowingCrashMonitorContext.h"
#import "GrowingCrashID.h"
#import "GrowingCrashThread.h"
#import "GrowingCrashStackCursor_MachineContext.h"
#import <Foundation/Foundation.h>

//#define GrowingCrashLogger_LocalLevel TRACE
#import "GrowingCrashLogger.h"


#define kIdleInterval 5.0f


@class GrowingCrashDeadlockMonitor;

// ============================================================================
#pragma mark - Globals -
// ============================================================================

static volatile bool g_isEnabled = false;

static GrowingCrash_MonitorContext g_monitorContext;

/** Thread which monitors other threads. */
static GrowingCrashDeadlockMonitor* g_monitor;

static GrowingCrashThread g_mainQueueThread;

/** Interval between watchdog pulses. */
static NSTimeInterval g_watchdogInterval = 0;


// ============================================================================
#pragma mark - X -
// ============================================================================

@interface GrowingCrashDeadlockMonitor: NSObject

@property(nonatomic, readwrite, retain) NSThread* monitorThread;
@property(atomic, readwrite, assign) BOOL awaitingResponse;

@end

@implementation GrowingCrashDeadlockMonitor

@synthesize monitorThread = _monitorThread;
@synthesize awaitingResponse = _awaitingResponse;

- (id) init
{
    if((self = [super init]))
    {
        // target (self) is retained until selector (runMonitor) exits.
        self.monitorThread = [[NSThread alloc] initWithTarget:self selector:@selector(runMonitor) object:nil];
        self.monitorThread.name = @"GrowingCrash Deadlock Detection Thread";
        [self.monitorThread start];
    }
    return self;
}

- (void) cancel
{
    [self.monitorThread cancel];
}

- (void) watchdogPulse
{
    __block id blockSelf = self;
    self.awaitingResponse = YES;
    dispatch_async(dispatch_get_main_queue(), ^
                   {
                       [blockSelf watchdogAnswer];
                   });
}

- (void) watchdogAnswer
{
    self.awaitingResponse = NO;
}

- (void) handleDeadlock
{
    thread_act_array_t threads = NULL;
    mach_msg_type_number_t numThreads = 0;
    growingcrashmc_suspendEnvironment(&threads, &numThreads);
    growingcrashcm_notifyFatalExceptionCaptured(false);

    GROWINGCRASHMC_NEW_CONTEXT(machineContext);
    growingcrashmc_getContextForThread(g_mainQueueThread, machineContext, false);
    GrowingCrashStackCursor stackCursor;
    growingcrashsc_initWithMachineContext(&stackCursor, GROWINGCRASHSC_MAX_STACK_DEPTH, machineContext);
    char eventID[37];
    growingcrashid_generate(eventID);

    GrowingCrashLOG_DEBUG(@"Filling out context.");
    GrowingCrash_MonitorContext* crashContext = &g_monitorContext;
    memset(crashContext, 0, sizeof(*crashContext));
    crashContext->crashType = GrowingCrashMonitorTypeMainThreadDeadlock;
    crashContext->eventID = eventID;
    crashContext->registersAreValid = false;
    crashContext->offendingMachineContext = machineContext;
    crashContext->stackCursor = &stackCursor;
    
    growingcrashcm_handleException(crashContext);
    growingcrashmc_resumeEnvironment(threads, numThreads);

    GrowingCrashLOG_DEBUG(@"Calling abort()");
    abort();
}

- (void) runMonitor
{
    BOOL cancelled = NO;
    do
    {
        // Only do a watchdog check if the watchdog interval is > 0.
        // If the interval is <= 0, just idle until the user changes it.
        @autoreleasepool {
            NSTimeInterval sleepInterval = g_watchdogInterval;
            BOOL runWatchdogCheck = sleepInterval > 0;
            if(!runWatchdogCheck)
            {
                sleepInterval = kIdleInterval;
            }
            [NSThread sleepForTimeInterval:sleepInterval];
            cancelled = self.monitorThread.isCancelled;
            if(!cancelled && runWatchdogCheck)
            {
                if(self.awaitingResponse)
                {
                    [self handleDeadlock];
                }
                else
                {
                    [self watchdogPulse];
                }
            }
        }
    } while (!cancelled);
}

@end

// ============================================================================
#pragma mark - API -
// ============================================================================

static void initialize()
{
    static bool isInitialized = false;
    if(!isInitialized)
    {
        isInitialized = true;
        dispatch_async(dispatch_get_main_queue(), ^{g_mainQueueThread = growingcrashthread_self();});
    }
}

static void setEnabled(bool isEnabled)
{
    if(isEnabled != g_isEnabled)
    {
        g_isEnabled = isEnabled;
        if(isEnabled)
        {
            GrowingCrashLOG_DEBUG(@"Creating new deadlock monitor.");
            initialize();
            g_monitor = [[GrowingCrashDeadlockMonitor alloc] init];
        }
        else
        {
            GrowingCrashLOG_DEBUG(@"Stopping deadlock monitor.");
            [g_monitor cancel];
            g_monitor = nil;
        }
    }
}

static bool isEnabled()
{
    return g_isEnabled;
}

GrowingCrashMonitorAPI* growingcrashcm_deadlock_getAPI()
{
    static GrowingCrashMonitorAPI api =
    {
        .setEnabled = setEnabled,
        .isEnabled = isEnabled
    };
    return &api;
}

void growingcrashcm_setDeadlockHandlerWatchdogInterval(double value)
{
    g_watchdogInterval = value;
}
