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
//  GrowingCrashStackCursor_Backtrace.h
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


#ifndef GrowingCrashStackCursor_Backtrace_h
#define GrowingCrashStackCursor_Backtrace_h

#ifdef __cplusplus
extern "C" {
#endif
    
    
#include "GrowingCrashStackCursor.h"

/** Exposed for other internal systems to use.
 */
typedef struct
{
    int skippedEntries;
    int backtraceLength;
    const uintptr_t* backtrace;
} GrowingCrashStackCursor_Backtrace_Context;
    

/** Initialize a stack cursor for an existing backtrace (array of addresses).
 *
 * @param cursor The stack cursor to initialize.
 *
 * @param backtrace The existing backtrace to walk.
 *
 * @param backtraceLength The length of the backtrace.
 *
 * @param skipEntries The number of stack entries to skip.
 */
void growingcrashsc_initWithBacktrace(GrowingCrashStackCursor *cursor, const uintptr_t* backtrace, int backtraceLength, int skipEntries);
    
    
#ifdef __cplusplus
}
#endif

#endif // GrowingCrashStackCursor_Backtrace_h
