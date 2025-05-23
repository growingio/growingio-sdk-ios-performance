//  Copyright (c) 2016 Karl Stenerud. All rights reserved.
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
//  GrowingCrashStackCursor_MachineContext.c
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


#include "GrowingCrashStackCursor_MachineContext.h"

#include "GrowingCrashCPU.h"
#include "GrowingCrashMemory.h"

#include <stdlib.h>

#define GrowingCrashLogger_LocalLevel TRACE
#include "GrowingCrashLogger.h"


/** Represents an entry in a frame list.
 * This is modeled after the various i386/x64 frame walkers in the xnu source,
 * and seems to work fine in ARM as well. I haven't included the args pointer
 * since it's not needed in this context.
 */

/*
 
 comment by @SecondDog
 
 try to explain why it is work in arm64:
 
 as we know ,arm stack layout is like this:
 so the pre FP should be *(current FP - 32)
 
 -------------  <---------- current FP
 | PC         |
 -------------  <---------- current FP - 8
 | LR         |
 -------------  <---------- current FP - 16
 | SP         |
 -------------  <---------- current FP - 24
 | FP(pre)    |
 -------------  <---------- current FP - 32
 
 the struct FrameEntry is defined like this
 typedef struct FrameEntry
 {
 struct FrameEntry* previous; <----- this pointer is 8 byte in arm64
 uintptr_t return_address;  <------ this value is also 8 byte
 } FrameEntry; <----- 16byte total
 
 
 but the arm64 call stack is like this.
 -------------
 | LR(x30)     |
 -------------
 | pre FP(x29) |
 -------------  <-----Current FP
 
 so copy 16 byte data to the FrameEntry is just fit in arm64,so it works fine in arm64
 
 */

typedef struct FrameEntry
{
    /** The previous frame in the list. */
    struct FrameEntry* previous;
    
    /** The instruction address. */
    uintptr_t return_address;
} FrameEntry;


typedef struct
{
    const struct GrowingCrashMachineContext* machineContext;
    int maxStackDepth;
    FrameEntry currentFrame;
    uintptr_t instructionAddress;
    uintptr_t linkRegister;
    bool isPastFramePointer;
} MachineContextCursor;

static bool advanceCursor(GrowingCrashStackCursor *cursor)
{
    MachineContextCursor* context = (MachineContextCursor*)cursor->context;
    uintptr_t nextAddress = 0;
    
    if(cursor->state.currentDepth >= context->maxStackDepth)
    {
        cursor->state.hasGivenUp = true;
        return false;
    }
    
    if(context->instructionAddress == 0 && cursor->state.currentDepth == 0)
    {
        context->instructionAddress = growingcrashcpu_instructionAddress(context->machineContext);
        nextAddress = context->instructionAddress;
        goto successfulExit;
    }
    
    if(context->linkRegister == 0 && !context->isPastFramePointer)
    {
        // Link register, if available, is the second address in the trace.
        context->linkRegister = growingcrashcpu_linkRegister(context->machineContext);
        if(context->linkRegister != 0)
        {
            nextAddress = context->linkRegister;
            goto successfulExit;
        }
    }

    if(context->currentFrame.previous == NULL)
    {
        if(context->isPastFramePointer)
        {
            return false;
        }
        context->currentFrame.previous = (struct FrameEntry*)growingcrashcpu_framePointer(context->machineContext);
        context->isPastFramePointer = true;
    }

    if(!growingcrashmem_copySafely(context->currentFrame.previous, &context->currentFrame, sizeof(context->currentFrame)))
    {
        return false;
    }
    if(context->currentFrame.previous == 0 || context->currentFrame.return_address == 0)
    {
        return false;
    }

    nextAddress = context->currentFrame.return_address;
    
successfulExit:
    cursor->stackEntry.address = growingcrashcpu_normaliseInstructionPointer(nextAddress);
    cursor->state.currentDepth++;
    return true;
}

static void resetCursor(GrowingCrashStackCursor* cursor)
{
    growingcrashsc_resetCursor(cursor);
    MachineContextCursor* context = (MachineContextCursor*)cursor->context;
    context->currentFrame.previous = 0;
    context->currentFrame.return_address = 0;
    context->instructionAddress = 0;
    context->linkRegister = 0;
    context->isPastFramePointer = 0;
}

void growingcrashsc_initWithMachineContext(GrowingCrashStackCursor *cursor, int maxStackDepth, const struct GrowingCrashMachineContext* machineContext)
{
    growingcrashsc_initCursor(cursor, resetCursor, advanceCursor);
    MachineContextCursor* context = (MachineContextCursor*)cursor->context;
    context->machineContext = machineContext;
    context->maxStackDepth = maxStackDepth;
    context->instructionAddress = cursor->stackEntry.address;
}
