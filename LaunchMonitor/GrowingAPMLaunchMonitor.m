//
//  GrowingAPMLaunchMonitor.m
//  GrowingAnalytics
//
//  Created by YoloMao on 2022/9/27.
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

#import "GrowingAPMLaunchMonitor.h"
#import "GrowingTimeUtil.h"
#import "GrowingViewControllerLifecycle.h"
#import "GrowingAppLifecycle.h"

@interface GrowingAPMLaunchMonitor () <GrowingViewControllerLifecycleDelegate, GrowingAppLifecycleDelegate>

@property (nonatomic, assign) double firstVCDidAppearTime;

@end

@implementation GrowingAPMLaunchMonitor

#pragma mark - Monitor

- (void)startMonitor {
    [GrowingViewControllerLifecycle.sharedInstance addViewControllerLifecycleDelegate:self];
    [GrowingAppLifecycle.sharedInstance addAppLifecycleDelegate:self];
}

#pragma mark - GrowingViewControllerLifecycleDelegate

- (void)pageLoadCompletedWithViewController:(UIViewController *)viewController
                               loadViewTime:(double)loadViewTime
                            viewDidLoadTime:(double)viewDidLoadTime
                         viewWillAppearTime:(double)viewWillAppearTime
                          viewDidAppearTime:(double)viewDidAppearTime {
    if (self.firstVCDidAppearTime > 0) {
        return;
    }
    
    // cold reboot
    self.firstVCDidAppearTime = viewDidAppearTime;
    if (self.monitorBlock) {
        self.monitorBlock(viewDidAppearTime - self.coldRebootBeginTime, NO);
    }
    
    dispatch_async(dispatch_get_global_queue(0, 0), ^{
        [GrowingViewControllerLifecycle.sharedInstance removeViewControllerLifecycleDelegate:self];
    });
}

#pragma mark - GrowingAppLifecycleDelegate

- (void)applicationDidBecomeActive {
    GrowingAppLifecycle *appLifecycle = GrowingAppLifecycle.sharedInstance;
    if (appLifecycle.appDidEnterBackgroundTime == 0) {
        // 首次启动
        return;
    }
    if (appLifecycle.appWillEnterForegroundTime < appLifecycle.appWillResignActiveTime) {
        // 下拉通知或进入后台应用列表
        return;
    }
    
    // warm reboot
    double duration = appLifecycle.appDidBecomeActiveTime - appLifecycle.appWillEnterForegroundTime;
    if (self.monitorBlock) {
        self.monitorBlock(duration, YES);
    }
}

@end
