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
//  GrowingCrashStackCursor_SelfThread.c
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


#include "GrowingCrashStackCursor_SelfThread.h"
#include "GrowingCrashStackCursor_Backtrace.h"
#include <execinfo.h>

//#define GrowingCrashLogger_LocalLevel TRACE
#include "GrowingCrashLogger.h"

#define MAX_BACKTRACE_LENGTH (GROWINGCRASHSC_CONTEXT_SIZE - sizeof(GrowingCrashStackCursor_Backtrace_Context) / sizeof(void*) - 1)

typedef struct
{
    GrowingCrashStackCursor_Backtrace_Context SelfThreadContextSpacer;
    uintptr_t backtrace[0];
} SelfThreadContext;

void growingcrashsc_initSelfThread(GrowingCrashStackCursor *cursor, int skipEntries)
{
    SelfThreadContext* context = (SelfThreadContext*)cursor->context;
    int backtraceLength = backtrace((void**)context->backtrace, MAX_BACKTRACE_LENGTH);
    growingcrashsc_initWithBacktrace(cursor, context->backtrace, backtraceLength, skipEntries + 1);
}
