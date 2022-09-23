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
//  GrowingCrashMachineContext.h
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


#ifndef HDR_GrowingCrashMachineContext_h
#define HDR_GrowingCrashMachineContext_h

#ifdef __cplusplus
extern "C" {
#endif

#include "GrowingCrashThread.h"
#include <stdbool.h>
#include <mach/mach.h>

/** Suspend the runtime environment.
 */
void growingcrashmc_suspendEnvironment(thread_act_array_t *suspendedThreads, mach_msg_type_number_t *numSuspendedThreads);

/** Resume the runtime environment.
 */
void growingcrashmc_resumeEnvironment(thread_act_array_t threads, mach_msg_type_number_t numThreads);

/** Create a new machine context on the stack.
 * This macro creates a storage object on the stack, as well as a pointer of type
 * struct GrowingCrashMachineContext* in the current scope, which points to the storage object.
 *
 * Example usage: GROWINGCRASHMC_NEW_CONTEXT(a_context);
 * This creates a new pointer at the current scope that behaves as if:
 *     struct GrowingCrashMachineContext* a_context = some_storage_location;
 *
 * @param NAME The C identifier to give the pointer.
 */
#define GROWINGCRASHMC_NEW_CONTEXT(NAME) \
    char growingcrashmc_##NAME##_storage[growingcrashmc_contextSize()]; \
    struct GrowingCrashMachineContext* NAME = (struct GrowingCrashMachineContext*)growingcrashmc_##NAME##_storage

struct GrowingCrashMachineContext;

/** Get the internal size of a machine context.
 */
int growingcrashmc_contextSize(void);

/** Fill in a machine context from a thread.
 *
 * @param thread The thread to get information from.
 * @param destinationContext The context to fill.
 * @param isCrashedContext Used to indicate that this is the thread that crashed,
 *
 * @return true if successful.
 */
bool growingcrashmc_getContextForThread(GrowingCrashThread thread, struct GrowingCrashMachineContext* destinationContext, bool isCrashedContext);

/** Fill in a machine context from a signal handler.
 * A signal handler context is always assumed to be a crashed context.
 *
 * @param signalUserContext The signal context to get information from.
 * @param destinationContext The context to fill.
 *
 * @return true if successful.
 */
bool growingcrashmc_getContextForSignal(void* signalUserContext, struct GrowingCrashMachineContext* destinationContext);

/** Get the thread associated with a machine context.
 *
 * @param context The machine context.
 *
 * @return The associated thread.
 */
GrowingCrashThread growingcrashmc_getThreadFromContext(const struct GrowingCrashMachineContext* const context);

/** Get the number of threads stored in a machine context.
 *
 * @param context The machine context.
 *
 * @return The number of threads.
 */
int growingcrashmc_getThreadCount(const struct GrowingCrashMachineContext* const context);

/** Get a thread from a machine context.
 *
 * @param context The machine context.
 * @param index The index of the thread to retrieve.
 *
 * @return The thread.
 */
GrowingCrashThread growingcrashmc_getThreadAtIndex(const struct GrowingCrashMachineContext* const context, int index);

/** Get the index of a thread.
 *
 * @param context The machine context.
 * @param thread The thread.
 *
 * @return The thread's index, or -1 if it couldn't be determined.
 */
int growingcrashmc_indexOfThread(const struct GrowingCrashMachineContext* const context, GrowingCrashThread thread);

/** Check if this is a crashed context.
 */
bool growingcrashmc_isCrashedContext(const struct GrowingCrashMachineContext* const context);

/** Check if this context can have stored CPU state.
 */
bool growingcrashmc_canHaveCPUState(const struct GrowingCrashMachineContext* const context);

/** Check if this context has valid exception registers.
 */
bool growingcrashmc_hasValidExceptionRegisters(const struct GrowingCrashMachineContext* const context);

/** Add a thread to the reserved threads list.
 *
 * @param thread The thread to add to the list.
 */
void growingcrashmc_addReservedThread(GrowingCrashThread thread);


#ifdef __cplusplus
}
#endif

#endif // HDR_GrowingCrashMachineContext_h
