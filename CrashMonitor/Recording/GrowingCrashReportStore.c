//  Created by Karl Stenerud on 2012-02-05.
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
//  GrowingCrashReportStore.c
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

#include "GrowingCrashReportStore.h"
#include "GrowingCrashLogger.h"
#include "GrowingCrashFileUtils.h"

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>


static int g_maxReportCount = 5;
// Have to use max 32-bit atomics because of MIPS.
static _Atomic(uint32_t) g_nextUniqueIDLow;
static int64_t g_nextUniqueIDHigh;
static const char* g_appName;
static const char* g_reportsPath;
static pthread_mutex_t g_mutex = PTHREAD_MUTEX_INITIALIZER;

static int compareInt64(const void* a, const void* b)
{
    int64_t diff = *(int64_t*)a - *(int64_t*)b;
    if(diff < 0)
    {
        return -1;
    }
    else if(diff > 0)
    {
        return 1;
    }
    return 0;
}

static inline int64_t getNextUniqueID()
{
    return g_nextUniqueIDHigh + g_nextUniqueIDLow++;
}

static void getCrashReportPathByID(int64_t id, char* pathBuffer)
{
    snprintf(pathBuffer, GROWINGCRS_MAX_PATH_LENGTH, "%s/%s-report-%016llx.json", g_reportsPath, g_appName, id);
    
}

static int64_t getReportIDFromFilename(const char* filename)
{
    char scanFormat[100];
    sprintf(scanFormat, "%s-report-%%" PRIx64 ".json", g_appName);
    
    int64_t reportID = 0;
    sscanf(filename, scanFormat, &reportID);
    return reportID;
}

static int getReportCount()
{
    int count = 0;
    DIR* dir = opendir(g_reportsPath);
    if(dir == NULL)
    {
        GrowingCrashLOG_ERROR("Could not open directory %s", g_reportsPath);
        goto done;
    }
    struct dirent* ent;
    while((ent = readdir(dir)) != NULL)
    {
        if(getReportIDFromFilename(ent->d_name) > 0)
        {
            count++;
        }
    }

done:
    if(dir != NULL)
    {
        closedir(dir);
    }
    return count;
}

static int getReportIDs(int64_t* reportIDs, int count)
{
    int index = 0;
    DIR* dir = opendir(g_reportsPath);
    if(dir == NULL)
    {
        GrowingCrashLOG_ERROR("Could not open directory %s", g_reportsPath);
        goto done;
    }

    struct dirent* ent;
    while((ent = readdir(dir)) != NULL && index < count)
    {
        int64_t reportID = getReportIDFromFilename(ent->d_name);
        if(reportID > 0)
        {
            reportIDs[index++] = reportID;
        }
    }

    qsort(reportIDs, (unsigned)count, sizeof(reportIDs[0]), compareInt64);

done:
    if(dir != NULL)
    {
        closedir(dir);
    }
    return index;
}

static void pruneReports()
{
    int reportCount = getReportCount();
    if(reportCount > g_maxReportCount)
    {
        int64_t reportIDs[reportCount];
        reportCount = getReportIDs(reportIDs, reportCount);
        
        for(int i = 0; i < reportCount - g_maxReportCount; i++)
        {
            growingcrs_deleteReportWithID(reportIDs[i]);
        }
    }
}

static void initializeIDs()
{
    time_t rawTime;
    time(&rawTime);
    struct tm time;
    gmtime_r(&rawTime, &time);
    int64_t baseID = (int64_t)time.tm_sec
                   + (int64_t)time.tm_min * 61
                   + (int64_t)time.tm_hour * 61 * 60
                   + (int64_t)time.tm_yday * 61 * 60 * 24
                   + (int64_t)time.tm_year * 61 * 60 * 24 * 366;
    baseID <<= 23;

    g_nextUniqueIDHigh = baseID & ~(int64_t)0xffffffff;
    g_nextUniqueIDLow = (uint32_t)(baseID & 0xffffffff);
}


// Public API

void growingcrs_initialize(const char* appName, const char* reportsPath)
{
    pthread_mutex_lock(&g_mutex);
    g_appName = strdup(appName);
    g_reportsPath = strdup(reportsPath);
    growingcrashfu_makePath(reportsPath);
    pruneReports();
    initializeIDs();
    pthread_mutex_unlock(&g_mutex);
}

int64_t growingcrs_getNextCrashReport(char* crashReportPathBuffer)
{
    int64_t nextID = getNextUniqueID();
    if(crashReportPathBuffer)
    {
        getCrashReportPathByID(nextID, crashReportPathBuffer);
    }
    return nextID;
}

int growingcrs_getReportCount()
{
    pthread_mutex_lock(&g_mutex);
    int count = getReportCount();
    pthread_mutex_unlock(&g_mutex);
    return count;
}

int growingcrs_getReportIDs(int64_t* reportIDs, int count)
{
    pthread_mutex_lock(&g_mutex);
    count = getReportIDs(reportIDs, count);
    pthread_mutex_unlock(&g_mutex);
    return count;
}

char* growingcrs_readReport(int64_t reportID)
{
    pthread_mutex_lock(&g_mutex);
    char path[GROWINGCRS_MAX_PATH_LENGTH];
    getCrashReportPathByID(reportID, path);
    char* result;
    growingcrashfu_readEntireFile(path, &result, NULL, 2000000);
    pthread_mutex_unlock(&g_mutex);
    return result;
}

int64_t growingcrs_addUserReport(const char* report, int reportLength)
{
    pthread_mutex_lock(&g_mutex);
    int64_t currentID = getNextUniqueID();
    char crashReportPath[GROWINGCRS_MAX_PATH_LENGTH];
    getCrashReportPathByID(currentID, crashReportPath);

    int fd = open(crashReportPath, O_WRONLY | O_CREAT, 0644);
    if(fd < 0)
    {
        GrowingCrashLOG_ERROR("Could not open file %s: %s", crashReportPath, strerror(errno));
        goto done;
    }

    int bytesWritten = (int)write(fd, report, (unsigned)reportLength);
    if(bytesWritten < 0)
    {
        GrowingCrashLOG_ERROR("Could not write to file %s: %s", crashReportPath, strerror(errno));
        goto done;
    }
    else if(bytesWritten < reportLength)
    {
        GrowingCrashLOG_ERROR("Expected to write %d bytes to file %s, but only wrote %d", crashReportPath, reportLength, bytesWritten);
    }

done:
    if(fd >= 0)
    {
        close(fd);
    }
    pthread_mutex_unlock(&g_mutex);

    return currentID;
}

void growingcrs_deleteAllReports()
{
    pthread_mutex_lock(&g_mutex);
    growingcrashfu_deleteContentsOfPath(g_reportsPath);
    pthread_mutex_unlock(&g_mutex);
}

void growingcrs_deleteReportWithID(int64_t reportID)
{
    char path[GROWINGCRS_MAX_PATH_LENGTH];
    getCrashReportPathByID(reportID, path);
    growingcrashfu_removeFile(path, true);
}

void growingcrs_setMaxReportCount(int maxReportCount)
{
    g_maxReportCount = maxReportCount;
}
