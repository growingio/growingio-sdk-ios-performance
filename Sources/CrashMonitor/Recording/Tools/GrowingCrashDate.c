// Copyright 2016 Karl Stenerud.
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
//  GrowingCrashDate.c
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

#include "GrowingCrashDate.h"
#include <stdio.h>
#include <time.h>

void growingcrashdate_utcStringFromTimestamp(time_t timestamp, char* buffer21Chars)
{
    struct tm result = {0};
    gmtime_r(&timestamp, &result);
    snprintf(buffer21Chars, 21, "%04d-%02d-%02dT%02d:%02d:%02dZ",
             result.tm_year + 1900,
             result.tm_mon + 1,
             result.tm_mday,
             result.tm_hour,
             result.tm_min,
             result.tm_sec);
}

void growingcrashdate_utcStringFromMicroseconds(int64_t microseconds, char* buffer28Chars)
{
    struct tm result = {0};
    time_t curtime = (time_t)(microseconds / 1000000);
    long micros = (long)(microseconds % 1000000);

    gmtime_r(&curtime, &result);
    snprintf(buffer28Chars, 28, "%04d-%02d-%02dT%02d:%02d:%02d.%06ldZ",
             result.tm_year + 1900,
             result.tm_mon + 1,
             result.tm_mday,
             result.tm_hour,
             result.tm_min,
             result.tm_sec,
             micros);
}
