//
//  GrowingAPM+Private.h
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

#import "GrowingAPM.h"
#import "GrowingAPMMonitor.h"

#if __has_include(<GrowingAPM/GrowingAPMLaunchMonitor.h>)
#import <GrowingAPM/GrowingAPMLaunchMonitor.h>
#ifndef GROWING_APM_LAUNCH
#define GROWING_APM_LAUNCH
#endif
#endif

#if __has_include(<GrowingAPM/GrowingAPMUIMonitor.h>)
#import <GrowingAPM/GrowingAPMUIMonitor.h>
#ifndef GROWING_APM_UI
#define GROWING_APM_UI
#endif
#endif

#if __has_include(<GrowingAPM/GrowingAPMCrashMonitor.h>)
#import <GrowingAPM/GrowingAPMCrashMonitor.h>
@class GrowingCrashInstallation;
#ifndef GROWING_APM_CRASH
#define GROWING_APM_CRASH
#endif
#endif

#if __has_include(<GrowingAPM/GrowingAPMNetworkMonitor.h>)
#import <GrowingAPM/GrowingAPMNetworkMonitor.h>
#ifndef GROWING_APM_NETWORK
#define GROWING_APM_NETWORK
#endif
#endif

NS_ASSUME_NONNULL_BEGIN

@interface GrowingAPM (Private)

@property (nonatomic, strong, readonly) id <GrowingAPMMonitor> crashMonitor;
@property (nonatomic, strong, readonly) id <GrowingAPMMonitor> launchMonitor;
@property (nonatomic, strong, readonly) id <GrowingAPMMonitor> pageLoadMonitor;
@property (nonatomic, strong, readonly) id <GrowingAPMMonitor> networkMonitor;

#ifdef GROWING_APM_CRASH
@property (class, nonatomic, weak) GrowingCrashInstallation *crashInstallation;
#endif

@end

NS_ASSUME_NONNULL_END
