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
//  GrowingCrashCPU_x86_64.c
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


#if defined (__x86_64__)


#include "GrowingCrashCPU.h"
#include "GrowingCrashCPU_Apple.h"
#include "GrowingCrashMachineContext.h"
#include "GrowingCrashMachineContext_Apple.h"

#include <stdlib.h>

//#define GrowingCrashLogger_LocalLevel TRACE
#include "GrowingCrashLogger.h"


static const char* g_registerNames[] =
{
    "rax", "rbx", "rcx", "rdx",
    "rdi", "rsi",
    "rbp", "rsp",
    "r8", "r9", "r10", "r11", "r12", "r13", "r14", "r15",
    "rip", "rflags",
    "cs", "fs", "gs"
};
static const int g_registerNamesCount =
sizeof(g_registerNames) / sizeof(*g_registerNames);


static const char* g_exceptionRegisterNames[] =
{
    "trapno", "err", "faultvaddr"
};
static const int g_exceptionRegisterNamesCount =
sizeof(g_exceptionRegisterNames) / sizeof(*g_exceptionRegisterNames);


uintptr_t growingcrashcpu_framePointer(const GrowingCrashMachineContext* const context)
{
    return context->machineContext.__ss.__rbp;
}

uintptr_t growingcrashcpu_stackPointer(const GrowingCrashMachineContext* const context)
{
    return context->machineContext.__ss.__rsp;
}

uintptr_t growingcrashcpu_instructionAddress(const GrowingCrashMachineContext* const context)
{
    return context->machineContext.__ss.__rip;
}

uintptr_t growingcrashcpu_linkRegister(__unused const GrowingCrashMachineContext* const context)
{
    return 0;
}

void growingcrashcpu_getState(GrowingCrashMachineContext* context)
{
    thread_t thread = context->thisThread;
    STRUCT_MCONTEXT_L* const machineContext = &context->machineContext;
    
    growingcrashcpu_i_fillState(thread, (thread_state_t)&machineContext->__ss, x86_THREAD_STATE64, x86_THREAD_STATE64_COUNT);
    growingcrashcpu_i_fillState(thread, (thread_state_t)&machineContext->__es, x86_EXCEPTION_STATE64, x86_EXCEPTION_STATE64_COUNT);
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
    switch(regNumber)
    {
        case 0:
            return context->machineContext.__ss.__rax;
        case 1:
            return context->machineContext.__ss.__rbx;
        case 2:
            return context->machineContext.__ss.__rcx;
        case 3:
            return context->machineContext.__ss.__rdx;
        case 4:
            return context->machineContext.__ss.__rdi;
        case 5:
            return context->machineContext.__ss.__rsi;
        case 6:
            return context->machineContext.__ss.__rbp;
        case 7:
            return context->machineContext.__ss.__rsp;
        case 8:
            return context->machineContext.__ss.__r8;
        case 9:
            return context->machineContext.__ss.__r9;
        case 10:
            return context->machineContext.__ss.__r10;
        case 11:
            return context->machineContext.__ss.__r11;
        case 12:
            return context->machineContext.__ss.__r12;
        case 13:
            return context->machineContext.__ss.__r13;
        case 14:
            return context->machineContext.__ss.__r14;
        case 15:
            return context->machineContext.__ss.__r15;
        case 16:
            return context->machineContext.__ss.__rip;
        case 17:
            return context->machineContext.__ss.__rflags;
        case 18:
            return context->machineContext.__ss.__cs;
        case 19:
            return context->machineContext.__ss.__fs;
        case 20:
            return context->machineContext.__ss.__gs;
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
            return context->machineContext.__es.__trapno;
        case 1:
            return context->machineContext.__es.__err;
        case 2:
            return context->machineContext.__es.__faultvaddr;
    }

    GrowingCrashLOG_ERROR("Invalid register number: %d", regNumber);
    return 0;
}

uintptr_t growingcrashcpu_faultAddress(const GrowingCrashMachineContext* const context)
{
    return context->machineContext.__es.__faultvaddr;
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
