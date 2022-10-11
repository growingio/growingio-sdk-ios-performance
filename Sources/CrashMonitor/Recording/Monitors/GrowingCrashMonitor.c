//  Created by Karl Stenerud on 2012-02-12.
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
//  GrowingCrashMonitor.c
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


#include "GrowingCrashMonitor.h"
#include "GrowingCrashMonitorContext.h"
#include "GrowingCrashMonitorType.h"

#include "GrowingCrashMonitor_Deadlock.h"
#include "GrowingCrashMonitor_MachException.h"
#include "GrowingCrashMonitor_CPPException.h"
#include "GrowingCrashMonitor_NSException.h"
#include "GrowingCrashMonitor_Signal.h"
#include "GrowingCrashMonitor_System.h"
#include "GrowingCrashMonitor_User.h"
#include "GrowingCrashMonitor_AppState.h"
#include "GrowingCrashMonitor_Zombie.h"
#include "GrowingCrashDebug.h"
#include "GrowingCrashThread.h"
#include "GrowingCrashSystemCapabilities.h"

#include <memory.h>

//#define GrowingCrashLogger_LocalLevel TRACE
#include "GrowingCrashLogger.h"


// ============================================================================
#pragma mark - Globals -
// ============================================================================

typedef struct
{
    GrowingCrashMonitorType monitorType;
    GrowingCrashMonitorAPI* (*getAPI)(void);
} Monitor;

static Monitor g_monitors[] =
{
#if GROWINGCRASH_HAS_MACH
    {
        .monitorType = GrowingCrashMonitorTypeMachException,
        .getAPI = growingcrashcm_machexception_getAPI,
    },
#endif
#if GROWINGCRASH_HAS_SIGNAL
    {
        .monitorType = GrowingCrashMonitorTypeSignal,
        .getAPI = growingcrashcm_signal_getAPI,
    },
#endif
#if GROWINGCRASH_HAS_OBJC
    {
        .monitorType = GrowingCrashMonitorTypeNSException,
        .getAPI = growingcrashcm_nsexception_getAPI,
    },
    {
        .monitorType = GrowingCrashMonitorTypeMainThreadDeadlock,
        .getAPI = growingcrashcm_deadlock_getAPI,
    },
    {
        .monitorType = GrowingCrashMonitorTypeZombie,
        .getAPI = growingcrashcm_zombie_getAPI,
    },
#endif
    {
        .monitorType = GrowingCrashMonitorTypeCPPException,
        .getAPI = growingcrashcm_cppexception_getAPI,
    },
    {
        .monitorType = GrowingCrashMonitorTypeUserReported,
        .getAPI = growingcrashcm_user_getAPI,
    },
    {
        .monitorType = GrowingCrashMonitorTypeSystem,
        .getAPI = growingcrashcm_system_getAPI,
    },
    {
        .monitorType = GrowingCrashMonitorTypeApplicationState,
        .getAPI = growingcrashcm_appstate_getAPI,
    },
};
static int g_monitorsCount = sizeof(g_monitors) / sizeof(*g_monitors);

static GrowingCrashMonitorType g_activeMonitors = GrowingCrashMonitorTypeNone;

static bool g_handlingFatalException = false;
static bool g_crashedDuringExceptionHandling = false;
static bool g_requiresAsyncSafety = false;

static void (*g_onExceptionEvent)(struct GrowingCrash_MonitorContext* monitorContext);

// ============================================================================
#pragma mark - API -
// ============================================================================

static inline GrowingCrashMonitorAPI* getAPI(Monitor* monitor)
{
    if(monitor != NULL && monitor->getAPI != NULL)
    {
        return monitor->getAPI();
    }
    return NULL;
}

static inline void setMonitorEnabled(Monitor* monitor, bool isEnabled)
{
    GrowingCrashMonitorAPI* api = getAPI(monitor);
    if(api != NULL && api->setEnabled != NULL)
    {
        api->setEnabled(isEnabled);
    }
}

static inline bool isMonitorEnabled(Monitor* monitor)
{
    GrowingCrashMonitorAPI* api = getAPI(monitor);
    if(api != NULL && api->isEnabled != NULL)
    {
        return api->isEnabled();
    }
    return false;
}

static inline void addContextualInfoToEvent(Monitor* monitor, struct GrowingCrash_MonitorContext* eventContext)
{
    GrowingCrashMonitorAPI* api = getAPI(monitor);
    if(api != NULL && api->addContextualInfoToEvent != NULL)
    {
        api->addContextualInfoToEvent(eventContext);
    }
}

void growingcrashcm_setEventCallback(void (*onEvent)(struct GrowingCrash_MonitorContext* monitorContext))
{
    g_onExceptionEvent = onEvent;
}

void growingcrashcm_setActiveMonitors(GrowingCrashMonitorType monitorTypes)
{
    if(growingcrashdebug_isBeingTraced() && (monitorTypes & GrowingCrashMonitorTypeDebuggerUnsafe))
    {
        static bool hasWarned = false;
        if(!hasWarned)
        {
            hasWarned = true;
            GrowingCrashLOGBASIC_WARN("    ************************ Crash Handler Notice ************************");
            GrowingCrashLOGBASIC_WARN("    *     App is running in a debugger. Masking out unsafe monitors.     *");
            GrowingCrashLOGBASIC_WARN("    * This means that most crashes WILL NOT BE RECORDED while debugging! *");
            GrowingCrashLOGBASIC_WARN("    **********************************************************************");
        }
        monitorTypes &= GrowingCrashMonitorTypeDebuggerSafe;
    }
    if(g_requiresAsyncSafety && (monitorTypes & GrowingCrashMonitorTypeAsyncUnsafe))
    {
        GrowingCrashLOG_DEBUG("Async-safe environment detected. Masking out unsafe monitors.");
        monitorTypes &= GrowingCrashMonitorTypeAsyncSafe;
    }

    GrowingCrashLOG_DEBUG("Changing active monitors from 0x%x tp 0x%x.", g_activeMonitors, monitorTypes);

    GrowingCrashMonitorType activeMonitors = GrowingCrashMonitorTypeNone;
    for(int i = 0; i < g_monitorsCount; i++)
    {
        Monitor* monitor = &g_monitors[i];
        bool isEnabled = monitor->monitorType & monitorTypes;
        setMonitorEnabled(monitor, isEnabled);
        if(isMonitorEnabled(monitor))
        {
            activeMonitors |= monitor->monitorType;
        }
        else
        {
            activeMonitors &= ~monitor->monitorType;
        }
    }

    GrowingCrashLOG_DEBUG("Active monitors are now 0x%x.", activeMonitors);
    g_activeMonitors = activeMonitors;
}

GrowingCrashMonitorType growingcrashcm_getActiveMonitors()
{
    return g_activeMonitors;
}


// ============================================================================
#pragma mark - Private API -
// ============================================================================

bool growingcrashcm_notifyFatalExceptionCaptured(bool isAsyncSafeEnvironment)
{
    g_requiresAsyncSafety |= isAsyncSafeEnvironment; // Don't let it be unset.
    if(g_handlingFatalException)
    {
        g_crashedDuringExceptionHandling = true;
    }
    g_handlingFatalException = true;
    if(g_crashedDuringExceptionHandling)
    {
        GrowingCrashLOG_INFO("Detected crash in the crash reporter. Uninstalling GrowingCrash.");
        growingcrashcm_setActiveMonitors(GrowingCrashMonitorTypeNone);
    }
    return g_crashedDuringExceptionHandling;
}

void growingcrashcm_handleException(struct GrowingCrash_MonitorContext* context)
{
    context->requiresAsyncSafety = g_requiresAsyncSafety;
    if(g_crashedDuringExceptionHandling)
    {
        context->crashedDuringCrashHandling = true;
    }
    for(int i = 0; i < g_monitorsCount; i++)
    {
        Monitor* monitor = &g_monitors[i];
        if(isMonitorEnabled(monitor))
        {
            addContextualInfoToEvent(monitor, context);
        }
    }

    g_onExceptionEvent(context);

    if (context->currentSnapshotUserReported) {
        g_handlingFatalException = false;
    } else {
        if(g_handlingFatalException && !g_crashedDuringExceptionHandling) {
            GrowingCrashLOG_DEBUG("Exception is fatal. Restoring original handlers.");
            growingcrashcm_setActiveMonitors(GrowingCrashMonitorTypeNone);
        }
    }
}
