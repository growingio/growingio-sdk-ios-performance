//
//  GrowingAPM.m
//  GrowingAnalytics
//
//  Created by YoloMao on 2022/9/26.
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

#if __has_include(<GrowingAPM/GrowingAPM.h>)
#import <GrowingAPM/GrowingAPM.h>
#else
#import "GrowingAPM.h"
#endif

#import "GrowingAPM+Private.h"
#import "GrowingAppLifecycle.h"

// -------- UI MONITOR --------
#ifdef GROWING_APM_UI
#import "GrowingTimeUtil.h"

#ifdef GROWING_APM_UI_SOURCE
#import "GrowingAPMUIMonitor.h"
#else
#import <GrowingAPMUIMonitor/GrowingAPMUIMonitor.h>
#endif

#endif
// -------- UI MONITOR --------

// -------- CRASH MONITOR --------
#ifdef GROWING_APM_CRASH
#import <objc/runtime.h>

#ifdef GROWING_APM_CRASH_SOURCE
#import "GrowingAPMCrashMonitor.h"
#import "GrowingCrashInstallation.h"
#else
#import <GrowingAPMCrashMonitor/GrowingAPMCrashMonitor.h>
#import <GrowingAPMCrashMonitor/GrowingCrashInstallation.h>
#endif

#endif
// -------- CRASH MONITOR --------

@interface GrowingAPM ()

@property (nonatomic, copy) GrowingAPMConfig *config;
@property (nonatomic, strong, readwrite) id <GrowingAPMMonitor> crashMonitor;
@property (nonatomic, strong, readwrite) id <GrowingAPMMonitor> pageLoadMonitor;
@property (nonatomic, strong, readwrite) id <GrowingAPMMonitor> networkMonitor;

#ifdef GROWING_APM_UI
@property (nonatomic, assign) double coldRebootBeginTime;
#endif

#ifdef GROWING_APM_CRASH
@property (class, nonatomic, weak) GrowingCrashInstallation *crashInstallation;
#endif

@end

@implementation GrowingAPM

+ (instancetype)sharedInstance {
    static GrowingAPM *sharedInstance = nil;
    static dispatch_once_t onceToken;
    dispatch_once(&onceToken, ^{
        sharedInstance = [[GrowingAPM alloc] init];
    });
    return sharedInstance;
}

+ (void)startWithConfig:(GrowingAPMConfig *)config {
    if (![NSThread isMainThread]) {
        @throw [NSException exceptionWithName:@"初始化异常" reason:@"请在主线程中调用 +[GrowingAPM startWithConfig:] 进行初始化" userInfo:nil];
        return;
    }
    
    GrowingAPM *apm = GrowingAPM.sharedInstance;
    if (apm.config) {
        return;
    }
    apm.config = config;
    
    if (config.monitors & GrowingAPMMonitorsUserInterface) {
#ifdef GROWING_APM_UI
        GrowingAPMUIMonitor *monitor = [GrowingAPMUIMonitor sharedInstance];
        monitor.coldRebootBeginTime = apm.coldRebootBeginTime;
        [monitor startMonitor];
        apm.pageLoadMonitor = (id <GrowingAPMMonitor>)monitor;
#endif
    }
    
    if (config.monitors & GrowingAPMMonitorsCrash) {
#ifdef GROWING_APM_CRASH
        GrowingAPMCrashMonitor *monitor = [[GrowingAPMCrashMonitor alloc] init];
        [monitor startMonitor];
        apm.crashMonitor = (id <GrowingAPMMonitor>)monitor;
#endif
    }
    
    if (config.monitors & GrowingAPMMonitorsNetwork) {
#ifdef GROWING_APM_NETWORK
        
#endif
    }
}

+ (void)swizzle:(GrowingAPMMonitors)monitors {
    if (monitors & GrowingAPMMonitorsUserInterface) {
#ifdef GROWING_APM_UI
        [GrowingAppLifecycle setup];
        [GrowingAPMUIMonitor setup];
        GrowingAPM.sharedInstance.coldRebootBeginTime = GrowingTimeUtil.currentSystemTimeMillis;
#endif
    }
    
    if (monitors & GrowingAPMMonitorsCrash) {
#ifdef GROWING_APM_CRASH
        [GrowingAPM.crashInstallation install];
#endif
    }
    
    if (monitors & GrowingAPMMonitorsNetwork) {
#ifdef GROWING_APM_NETWORK
        
#endif
    }
}

#ifdef GROWING_APM_CRASH
#pragma mark - Getter & Setter

+ (GrowingCrashInstallation *)crashInstallation {
    return objc_getAssociatedObject(self, _cmd);
}

+ (void)setCrashInstallation:(GrowingCrashInstallation *)crashInstallation {
    objc_setAssociatedObject(self, @selector(crashInstallation), crashInstallation, OBJC_ASSOCIATION_ASSIGN);
}
#endif

@end
