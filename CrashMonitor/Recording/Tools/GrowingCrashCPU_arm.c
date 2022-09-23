//  Created by Karl Stenerud on 2012-01-29.
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
//  GrowingCrashCPU_arm.c
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

#if defined (__arm__)


#include "GrowingCrashCPU.h"
#include "GrowingCrashCPU_Apple.h"
#include "GrowingCrashMachineContext.h"
#include "GrowingCrashMachineContext_Apple.h"
#include <stdlib.h>

//#define GrowingCrashLogger_LocalLevel TRACE
#include "GrowingCrashLogger.h"


static const char* g_registerNames[] =
{
    "r0", "r1", "r2", "r3", "r4", "r5", "r6", "r7",
    "r8", "r9", "r10", "r11", "ip",
    "sp", "lr", "pc", "cpsr"
};
static const int g_registerNamesCount =
sizeof(g_registerNames) / sizeof(*g_registerNames);


static const char* g_exceptionRegisterNames[] =
{
    "exception", "fsr", "far"
};
static const int g_exceptionRegisterNamesCount =
sizeof(g_exceptionRegisterNames) / sizeof(*g_exceptionRegisterNames);


uintptr_t growingcrashcpu_framePointer(const GrowingCrashMachineContext* const context)
{
    return context->machineContext.__ss.__r[7];
}

uintptr_t growingcrashcpu_stackPointer(const GrowingCrashMachineContext* const context)
{
    return context->machineContext.__ss.__sp;
}

uintptr_t growingcrashcpu_instructionAddress(const GrowingCrashMachineContext* const context)
{
    return context->machineContext.__ss.__pc;
}

uintptr_t growingcrashcpu_linkRegister(const GrowingCrashMachineContext* const context)
{
    return context->machineContext.__ss.__lr;
}

void growingcrashcpu_getState(GrowingCrashMachineContext* context)
{
    thread_t thread = context->thisThread;
    STRUCT_MCONTEXT_L* const machineContext = &context->machineContext;
    
    growingcrashcpu_i_fillState(thread, (thread_state_t)&machineContext->__ss, ARM_THREAD_STATE, ARM_THREAD_STATE_COUNT);
    growingcrashcpu_i_fillState(thread, (thread_state_t)&machineContext->__es, ARM_EXCEPTION_STATE, ARM_EXCEPTION_STATE_COUNT);
}

int growingcrashcpu_numRegisters(void)
{
    return g_registerNamesCount;
}

const char* growingcrashcpu_registerName(const int regNumber)
{
    if(regNumber < growingcrashcpu_numRegisters())
    {
        return g_registerNames[regNumber];
    }
    return NULL;
}

uint64_t growingcrashcpu_registerValue(const GrowingCrashMachineContext* const context, const int regNumber)
{
    if(regNumber <= 12)
    {
        return context->machineContext.__ss.__r[regNumber];
    }

    switch(regNumber)
    {
        case 13: return context->machineContext.__ss.__sp;
        case 14: return context->machineContext.__ss.__lr;
        case 15: return context->machineContext.__ss.__pc;
        case 16: return context->machineContext.__ss.__cpsr;
    }

    GrowingCrashLOG_ERROR("Invalid register number: %d", regNumber);
    return 0;
}

int growingcrashcpu_numExceptionRegisters(void)
{
    return g_exceptionRegisterNamesCount;
}

const char* growingcrashcpu_exceptionRegisterName(const int regNumber)
{
    if(regNumber < growingcrashcpu_numExceptionRegisters())
    {
        return g_exceptionRegisterNames[regNumber];
    }
    GrowingCrashLOG_ERROR("Invalid register number: %d", regNumber);
    return NULL;
}

uint64_t growingcrashcpu_exceptionRegisterValue(const GrowingCrashMachineContext* const context, const int regNumber)
{
    switch(regNumber)
    {
        case 0:
            return context->machineContext.__es.__exception;
        case 1:
            return context->machineContext.__es.__fsr;
        case 2:
            return context->machineContext.__es.__far;
    }

    GrowingCrashLOG_ERROR("Invalid register number: %d", regNumber);
    return 0;
}

uintptr_t growingcrashcpu_faultAddress(const GrowingCrashMachineContext* const context)
{
    return context->machineContext.__es.__far;
}

int growingcrashcpu_stackGrowDirection(void)
{
    return -1;
}

uintptr_t growingcrashcpu_normaliseInstructionPointer(uintptr_t ip)
{
    return ip;
}

#endif
