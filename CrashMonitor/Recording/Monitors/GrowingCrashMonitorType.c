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
//  GrowingCrashMonitorType.c
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


#include "GrowingCrashMonitorType.h"

#include <stdlib.h>


static const struct
{
    const GrowingCrashMonitorType type;
    const char* const name;
} g_monitorTypes[] =
{
#define MONITORTYPE(NAME) {NAME, #NAME}
    MONITORTYPE(GrowingCrashMonitorTypeMachException),
    MONITORTYPE(GrowingCrashMonitorTypeSignal),
    MONITORTYPE(GrowingCrashMonitorTypeCPPException),
    MONITORTYPE(GrowingCrashMonitorTypeNSException),
    MONITORTYPE(GrowingCrashMonitorTypeMainThreadDeadlock),
    MONITORTYPE(GrowingCrashMonitorTypeUserReported),
    MONITORTYPE(GrowingCrashMonitorTypeSystem),
    MONITORTYPE(GrowingCrashMonitorTypeApplicationState),
    MONITORTYPE(GrowingCrashMonitorTypeZombie),
};
static const int g_monitorTypesCount = sizeof(g_monitorTypes) / sizeof(*g_monitorTypes);


const char* growingcrashmonitortype_name(const GrowingCrashMonitorType monitorType)
{
    for(int i = 0; i < g_monitorTypesCount; i++)
    {
        if(g_monitorTypes[i].type == monitorType)
        {
            return g_monitorTypes[i].name;
        }
    }
    return NULL;
}
