//  Created by Karl Stenerud on 2016-12-02.
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
//  GrowingCrashMachineContext.c
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

#include "GrowingCrashMachineContext_Apple.h"
#include "GrowingCrashMachineContext.h"
#include "GrowingCrashSystemCapabilities.h"
#include "GrowingCrashCPU.h"
#include "GrowingCrashCPU_Apple.h"
#include "GrowingCrashStackCursor_MachineContext.h"

#include <mach/mach.h>

//#define GrowingCrashLogger_LocalLevel TRACE
#include "GrowingCrashLogger.h"

#ifdef __arm64__
#include <sys/_types/_ucontext64.h>
    #define UC_MCONTEXT uc_mcontext64

typedef ucontext64_t SignalUserContext;
#else
    #define UC_MCONTEXT uc_mcontext
    typedef ucontext_t SignalUserContext;
#endif

static GrowingCrashThread g_reservedThreads[10];
static int g_reservedThreadsMaxIndex = sizeof(g_reservedThreads) / sizeof(g_reservedThreads[0]) - 1;
static int g_reservedThreadsCount = 0;


static inline bool isStackOverflow(const GrowingCrashMachineContext* const context)
{
    GrowingCrashStackCursor stackCursor;
    growingcrashsc_initWithMachineContext(&stackCursor, GROWINGCRASHSC_STACK_OVERFLOW_THRESHOLD, context);
    while(stackCursor.advanceCursor(&stackCursor))
    {
    }
    return stackCursor.state.hasGivenUp;
}

static inline bool getThreadList(GrowingCrashMachineContext* context)
{
    const task_t thisTask = mach_task_self();
    GrowingCrashLOG_DEBUG("Getting thread list");
    kern_return_t kr;
    thread_act_array_t threads;
    mach_msg_type_number_t actualThreadCount;

    if((kr = task_threads(thisTask, &threads, &actualThreadCount)) != KERN_SUCCESS)
    {
        GrowingCrashLOG_ERROR("task_threads: %s", mach_error_string(kr));
        return false;
    }
    GrowingCrashLOG_TRACE("Got %d threads", context->threadCount);
    int threadCount = (int)actualThreadCount;
    int maxThreadCount = sizeof(context->allThreads) / sizeof(context->allThreads[0]);
    if(threadCount > maxThreadCount)
    {
        GrowingCrashLOG_ERROR("Thread count %d is higher than maximum of %d", threadCount, maxThreadCount);
        threadCount = maxThreadCount;
    }
    for(int i = 0; i < threadCount; i++)
    {
        context->allThreads[i] = threads[i];
    }
    context->threadCount = threadCount;

    for(mach_msg_type_number_t i = 0; i < actualThreadCount; i++)
    {
        mach_port_deallocate(thisTask, context->allThreads[i]);
    }
    vm_deallocate(thisTask, (vm_address_t)threads, sizeof(thread_t) * actualThreadCount);

    return true;
}

int growingcrashmc_contextSize()
{
    return sizeof(GrowingCrashMachineContext);
}

GrowingCrashThread growingcrashmc_getThreadFromContext(const GrowingCrashMachineContext* const context)
{
    return context->thisThread;
}

bool growingcrashmc_getContextForThread(GrowingCrashThread thread, GrowingCrashMachineContext* destinationContext, bool isCrashedContext)
{
    GrowingCrashLOG_DEBUG("Fill thread 0x%x context into %p. is crashed = %d", thread, destinationContext, isCrashedContext);
    memset(destinationContext, 0, sizeof(*destinationContext));
    destinationContext->thisThread = (thread_t)thread;
    destinationContext->isCurrentThread = thread == growingcrashthread_self();
    destinationContext->isCrashedContext = isCrashedContext;
    destinationContext->isSignalContext = false;
    if(growingcrashmc_canHaveCPUState(destinationContext))
    {
        growingcrashcpu_getState(destinationContext);
    }
    if(growingcrashmc_isCrashedContext(destinationContext))
    {
        destinationContext->isStackOverflow = isStackOverflow(destinationContext);
        getThreadList(destinationContext);
    }
    GrowingCrashLOG_TRACE("Context retrieved.");
    return true;
}

bool growingcrashmc_getContextForSignal(void* signalUserContext, GrowingCrashMachineContext* destinationContext)
{
    GrowingCrashLOG_DEBUG("Get context from signal user context and put into %p.", destinationContext);
    _STRUCT_MCONTEXT* sourceContext = ((SignalUserContext*)signalUserContext)->UC_MCONTEXT;
    memcpy(&destinationContext->machineContext, sourceContext, sizeof(destinationContext->machineContext));
    destinationContext->thisThread = (thread_t)growingcrashthread_self();
    destinationContext->isCrashedContext = true;
    destinationContext->isSignalContext = true;
    destinationContext->isStackOverflow = isStackOverflow(destinationContext);
    getThreadList(destinationContext);
    GrowingCrashLOG_TRACE("Context retrieved.");
    return true;
}

void growingcrashmc_addReservedThread(GrowingCrashThread thread)
{
    int nextIndex = g_reservedThreadsCount;
    if(nextIndex > g_reservedThreadsMaxIndex)
    {
        GrowingCrashLOG_ERROR("Too many reserved threads (%d). Max is %d", nextIndex, g_reservedThreadsMaxIndex);
        return;
    }
    g_reservedThreads[g_reservedThreadsCount++] = thread;
}

#if GROWINGCRASH_HAS_THREADS_API
static inline bool isThreadInList(thread_t thread, GrowingCrashThread* list, int listCount)
{
    for(int i = 0; i < listCount; i++)
    {
        if(list[i] == (GrowingCrashThread)thread)
        {
            return true;
        }
    }
    return false;
}
#endif

void growingcrashmc_suspendEnvironment(__unused thread_act_array_t *suspendedThreads, __unused mach_msg_type_number_t *numSuspendedThreads)
{
#if GROWINGCRASH_HAS_THREADS_API
    GrowingCrashLOG_DEBUG("Suspending environment.");
    kern_return_t kr;
    const task_t thisTask = mach_task_self();
    const thread_t thisThread = (thread_t)growingcrashthread_self();
    
    if((kr = task_threads(thisTask, suspendedThreads, numSuspendedThreads)) != KERN_SUCCESS)
    {
        GrowingCrashLOG_ERROR("task_threads: %s", mach_error_string(kr));
        return;
    }
    
    for(mach_msg_type_number_t i = 0; i < *numSuspendedThreads; i++)
    {
        thread_t thread = (*suspendedThreads)[i];
        if(thread != thisThread && !isThreadInList(thread, g_reservedThreads, g_reservedThreadsCount))
        {
            if((kr = thread_suspend(thread)) != KERN_SUCCESS)
            {
                // Record the error and keep going.
                GrowingCrashLOG_ERROR("thread_suspend (%08x): %s", thread, mach_error_string(kr));
            }
        }
    }
    
    GrowingCrashLOG_DEBUG("Suspend complete.");
#endif
}

void growingcrashmc_resumeEnvironment(__unused thread_act_array_t threads, __unused mach_msg_type_number_t numThreads)
{
#if GROWINGCRASH_HAS_THREADS_API
    GrowingCrashLOG_DEBUG("Resuming environment.");
    kern_return_t kr;
    const task_t thisTask = mach_task_self();
    const thread_t thisThread = (thread_t)growingcrashthread_self();
    
    if(threads == NULL || numThreads == 0)
    {
        GrowingCrashLOG_ERROR("we should call growingcrashmc_suspendEnvironment() first");
        return;
    }
    
    for(mach_msg_type_number_t i = 0; i < numThreads; i++)
    {
        thread_t thread = threads[i];
        if(thread != thisThread && !isThreadInList(thread, g_reservedThreads, g_reservedThreadsCount))
        {
            if((kr = thread_resume(thread)) != KERN_SUCCESS)
            {
                // Record the error and keep going.
                GrowingCrashLOG_ERROR("thread_resume (%08x): %s", thread, mach_error_string(kr));
            }
        }
    }
    
    for(mach_msg_type_number_t i = 0; i < numThreads; i++)
    {
        mach_port_deallocate(thisTask, threads[i]);
    }
    vm_deallocate(thisTask, (vm_address_t)threads, sizeof(thread_t) * numThreads);
    
    GrowingCrashLOG_DEBUG("Resume complete.");
#endif
}

int growingcrashmc_getThreadCount(const GrowingCrashMachineContext* const context)
{
    return context->threadCount;
}

GrowingCrashThread growingcrashmc_getThreadAtIndex(const GrowingCrashMachineContext* const context, int index)
{
    return context->allThreads[index];
    
}

int growingcrashmc_indexOfThread(const GrowingCrashMachineContext* const context, GrowingCrashThread thread)
{
    GrowingCrashLOG_TRACE("check thread vs %d threads", context->threadCount);
    for(int i = 0; i < (int)context->threadCount; i++)
    {
        GrowingCrashLOG_TRACE("%d: %x vs %x", i, thread, context->allThreads[i]);
        if(context->allThreads[i] == thread)
        {
            return i;
        }
    }
    return -1;
}

bool growingcrashmc_isCrashedContext(const GrowingCrashMachineContext* const context)
{
    return context->isCrashedContext;
}

static inline bool isContextForCurrentThread(const GrowingCrashMachineContext* const context)
{
    return context->isCurrentThread;
}

static inline bool isSignalContext(const GrowingCrashMachineContext* const context)
{
    return context->isSignalContext;
}

bool growingcrashmc_canHaveCPUState(const GrowingCrashMachineContext* const context)
{
    return !isContextForCurrentThread(context) || isSignalContext(context);
}

bool growingcrashmc_hasValidExceptionRegisters(const GrowingCrashMachineContext* const context)
{
    return growingcrashmc_canHaveCPUState(context) && growingcrashmc_isCrashedContext(context);
}
