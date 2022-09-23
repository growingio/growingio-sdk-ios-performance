//  Created by Karl Stenerud on 2012-01-28.
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
//  GrowingCrashC.c
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


#include "GrowingCrashC.h"

#include "GrowingCrashCachedData.h"
#include "GrowingCrashReport.h"
#include "GrowingCrashReportFixer.h"
#include "GrowingCrashReportStore.h"
#include "GrowingCrashMonitor_CPPException.h"
#include "GrowingCrashMonitor_Deadlock.h"
#include "GrowingCrashMonitor_User.h"
#include "GrowingCrashFileUtils.h"
#include "GrowingCrashObjC.h"
#include "GrowingCrashString.h"
#include "GrowingCrashMonitor_System.h"
#include "GrowingCrashMonitor_Zombie.h"
#include "GrowingCrashMonitor_AppState.h"
#include "GrowingCrashMonitorContext.h"
#include "GrowingCrashSystemCapabilities.h"

//#define GrowingCrashLogger_LocalLevel TRACE
#include "GrowingCrashLogger.h"

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum
{
    GrowingCrashApplicationStateNone,
    GrowingCrashApplicationStateDidBecomeActive,
    GrowingCrashApplicationStateWillResignActiveActive,
    GrowingCrashApplicationStateDidEnterBackground,
    GrowingCrashApplicationStateWillEnterForeground,
    GrowingCrashApplicationStateWillTerminate
} GrowingCrashApplicationState;

// ============================================================================
#pragma mark - Globals -
// ============================================================================

/** True if GrowingCrash has been installed. */
static volatile bool g_installed = 0;

static bool g_shouldAddConsoleLogToReport = false;
static bool g_shouldPrintPreviousLog = false;
static char g_consoleLogPath[GROWINGCRASHFU_MAX_PATH_LENGTH];
static GrowingCrashMonitorType g_monitoring = GrowingCrashMonitorTypeProductionSafeMinimal;
static char g_lastCrashReportFilePath[GROWINGCRASHFU_MAX_PATH_LENGTH];
static GrowingCrashReportWrittenCallback g_reportWrittenCallback;
static GrowingCrashApplicationState g_lastApplicationState = GrowingCrashApplicationStateNone;

// ============================================================================
#pragma mark - Utility -
// ============================================================================

static void printPreviousLog(const char* filePath)
{
    char* data;
    int length;
    if(growingcrashfu_readEntireFile(filePath, &data, &length, 0))
    {
        printf("\nvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv Previous Log vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv\n\n");
        printf("%s\n", data);
        free(data);
        printf("^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^\n\n");
        fflush(stdout);
    }
}

static void notifyOfBeforeInstallationState(void)
{
    GrowingCrashLOG_DEBUG("Notifying of pre-installation state");
    switch (g_lastApplicationState)
    {
        case GrowingCrashApplicationStateDidBecomeActive:
            return growingcrash_notifyAppActive(true);
        case GrowingCrashApplicationStateWillResignActiveActive:
            return growingcrash_notifyAppActive(false);
        case GrowingCrashApplicationStateDidEnterBackground:
            return growingcrash_notifyAppInForeground(false);
        case GrowingCrashApplicationStateWillEnterForeground:
            return growingcrash_notifyAppInForeground(true);
        case GrowingCrashApplicationStateWillTerminate:
            return growingcrash_notifyAppTerminate();
        default:
            return;
    }
}

// ============================================================================
#pragma mark - Callbacks -
// ============================================================================

/** Called when a crash occurs.
 *
 * This function gets passed as a callback to a crash handler.
 */
static void onCrash(struct GrowingCrash_MonitorContext* monitorContext)
{
    if (monitorContext->currentSnapshotUserReported == false) {
        GrowingCrashLOG_DEBUG("Updating application state to note crash.");
        growingcrashstate_notifyAppCrash();
    }
    monitorContext->consoleLogPath = g_shouldAddConsoleLogToReport ? g_consoleLogPath : NULL;

    if(monitorContext->crashedDuringCrashHandling)
    {
        growingcrashreport_writeRecrashReport(monitorContext, g_lastCrashReportFilePath);
    }
    else
    {
        char crashReportFilePath[GROWINGCRASHFU_MAX_PATH_LENGTH];
        int64_t reportID = growingcrs_getNextCrashReport(crashReportFilePath);
        strncpy(g_lastCrashReportFilePath, crashReportFilePath, sizeof(g_lastCrashReportFilePath));
        growingcrashreport_writeStandardReport(monitorContext, crashReportFilePath);

        if(g_reportWrittenCallback)
        {
            g_reportWrittenCallback(reportID);
        }
    }
}


// ============================================================================
#pragma mark - API -
// ============================================================================

GrowingCrashMonitorType growingcrash_install(const char* appName, const char* const installPath)
{
    GrowingCrashLOG_DEBUG("Installing crash reporter.");

    if(g_installed)
    {
        GrowingCrashLOG_DEBUG("Crash reporter already installed.");
        return g_monitoring;
    }
    g_installed = 1;

    char path[GROWINGCRASHFU_MAX_PATH_LENGTH];
    snprintf(path, sizeof(path), "%s/Reports", installPath);
    growingcrashfu_makePath(path);
    growingcrs_initialize(appName, path);

    snprintf(path, sizeof(path), "%s/Data", installPath);
    growingcrashfu_makePath(path);
    snprintf(path, sizeof(path), "%s/Data/CrashState.json", installPath);
    growingcrashstate_initialize(path);

    snprintf(g_consoleLogPath, sizeof(g_consoleLogPath), "%s/Data/ConsoleLog.txt", installPath);
    if(g_shouldPrintPreviousLog)
    {
        printPreviousLog(g_consoleLogPath);
    }
    growingcrashlog_setLogFilename(g_consoleLogPath, true);
    
    growingccd_init(60);

    growingcrashcm_setEventCallback(onCrash);
    GrowingCrashMonitorType monitors = growingcrash_setMonitoring(g_monitoring);

    GrowingCrashLOG_DEBUG("Installation complete.");

    notifyOfBeforeInstallationState();

    return monitors;
}

GrowingCrashMonitorType growingcrash_setMonitoring(GrowingCrashMonitorType monitors)
{
    g_monitoring = monitors;
    
    if(g_installed)
    {
        growingcrashcm_setActiveMonitors(monitors);
        return growingcrashcm_getActiveMonitors();
    }
    // Return what we will be monitoring in future.
    return g_monitoring;
}

void growingcrash_setUserInfoJSON(const char* const userInfoJSON)
{
    growingcrashreport_setUserInfoJSON(userInfoJSON);
}

void growingcrash_setDeadlockWatchdogInterval(double deadlockWatchdogInterval)
{
#if GROWINGCRASH_HAS_OBJC
    growingcrashcm_setDeadlockHandlerWatchdogInterval(deadlockWatchdogInterval);
#endif
}

void growingcrash_setSearchQueueNames(bool searchQueueNames)
{
    growingccd_setSearchQueueNames(searchQueueNames);
}

void growingcrash_setIntrospectMemory(bool introspectMemory)
{
    growingcrashreport_setIntrospectMemory(introspectMemory);
}

void growingcrash_setDoNotIntrospectClasses(const char** doNotIntrospectClasses, int length)
{
    growingcrashreport_setDoNotIntrospectClasses(doNotIntrospectClasses, length);
}

void growingcrash_setCrashNotifyCallback(const GrowingCrashReportWriteCallback onCrashNotify)
{
    growingcrashreport_setUserSectionWriteCallback(onCrashNotify);
}

void growingcrash_setReportWrittenCallback(const GrowingCrashReportWrittenCallback onReportWrittenNotify)
{
    g_reportWrittenCallback = onReportWrittenNotify;
}

void growingcrash_setAddConsoleLogToReport(bool shouldAddConsoleLogToReport)
{
    g_shouldAddConsoleLogToReport = shouldAddConsoleLogToReport;
}

void growingcrash_setPrintPreviousLog(bool shouldPrintPreviousLog)
{
    g_shouldPrintPreviousLog = shouldPrintPreviousLog;
}

void growingcrash_setMaxReportCount(int maxReportCount)
{
    growingcrs_setMaxReportCount(maxReportCount);
}

void growingcrash_reportUserException(const char* name,
                                 const char* reason,
                                 const char* language,
                                 const char* lineOfCode,
                                 const char* stackTrace,
                                 bool logAllThreads,
                                 bool terminateProgram)
{
    growingcrashcm_reportUserException(name,
                             reason,
                             language,
                             lineOfCode,
                             stackTrace,
                             logAllThreads,
                             terminateProgram);
    if(g_shouldAddConsoleLogToReport)
    {
        growingcrashlog_clearLogFile();
    }
}

void enableSwapCxaThrow(void)
{
    growingcrashcm_enableSwapCxaThrow();
}

void growingcrash_notifyObjCLoad(void)
{
    growingcrashstate_notifyObjCLoad();
}

void growingcrash_notifyAppActive(bool isActive)
{
    if (g_installed)
    {
        growingcrashstate_notifyAppActive(isActive);
    }
    g_lastApplicationState = isActive
        ? GrowingCrashApplicationStateDidBecomeActive
        : GrowingCrashApplicationStateWillResignActiveActive;
}

void growingcrash_notifyAppInForeground(bool isInForeground)
{
    if (g_installed)
    {
        growingcrashstate_notifyAppInForeground(isInForeground);
    }
    g_lastApplicationState = isInForeground
        ? GrowingCrashApplicationStateWillEnterForeground
        : GrowingCrashApplicationStateDidEnterBackground;
}

void growingcrash_notifyAppTerminate(void)
{
    if (g_installed)
    {
        growingcrashstate_notifyAppTerminate();
    }
    g_lastApplicationState = GrowingCrashApplicationStateWillTerminate;
}

void growingcrash_notifyAppCrash(void)
{
    growingcrashstate_notifyAppCrash();
}

int growingcrash_getReportCount()
{
    return growingcrs_getReportCount();
}

int growingcrash_getReportIDs(int64_t* reportIDs, int count)
{
    return growingcrs_getReportIDs(reportIDs, count);
}

char* growingcrash_readReport(int64_t reportID)
{
    if(reportID <= 0)
    {
        GrowingCrashLOG_ERROR("Report ID was %" PRIx64, reportID);
        return NULL;
    }

    char* rawReport = growingcrs_readReport(reportID);
    if(rawReport == NULL)
    {
        GrowingCrashLOG_ERROR("Failed to load report ID %" PRIx64, reportID);
        return NULL;
    }

    char* fixedReport = growingcrf_fixupCrashReport(rawReport);
    if(fixedReport == NULL)
    {
        GrowingCrashLOG_ERROR("Failed to fixup report ID %" PRIx64, reportID);
    }

    free(rawReport);
    return fixedReport;
}

int64_t growingcrash_addUserReport(const char* report, int reportLength)
{
    return growingcrs_addUserReport(report, reportLength);
}

void growingcrash_deleteAllReports()
{
    growingcrs_deleteAllReports();
}

void growingcrash_deleteReportWithID(int64_t reportID)
{
    growingcrs_deleteReportWithID(reportID);
}
