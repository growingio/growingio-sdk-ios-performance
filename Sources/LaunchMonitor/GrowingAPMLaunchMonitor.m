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
@property (nonatomic, assign) BOOL didSendColdReboot;

@end

@implementation GrowingAPMLaunchMonitor

+ (instancetype)sharedInstance {
    static id _sharedInstance = nil;
    static dispatch_once_t onceToken;
    dispatch_once(&onceToken, ^{
        _sharedInstance = [[self alloc] init];
    });
    return _sharedInstance;
}

#pragma mark - Monitor

- (void)startMonitor {
    [GrowingAppLifecycle.sharedInstance addAppLifecycleDelegate:self];
    
    dispatch_async(dispatch_get_main_queue(), ^{
        [self sendColdReboot];
    });
}

+ (void)setup {
    GrowingAPMLaunchMonitor *monitor = [GrowingAPMLaunchMonitor sharedInstance];
    [GrowingViewControllerLifecycle.sharedInstance addViewControllerLifecycleDelegate:monitor];
}

#pragma mark - Private Method

- (void)sendColdReboot {
    if (self.didSendColdReboot) {
        return;
    }
    
    if (self.firstVCDidAppearTime == 0) {
        return;
    }
    
    if (self.coldRebootBeginTime == 0) {
        return;
    }
    
    if (self.monitorBlock) {
        self.monitorBlock(self.firstVCDidAppearTime - self.coldRebootBeginTime, NO);
        self.didSendColdReboot = YES;
        
        dispatch_async(dispatch_get_main_queue(), ^{
            [GrowingViewControllerLifecycle.sharedInstance removeViewControllerLifecycleDelegate:self];
        });
    }
}

#pragma mark - GrowingViewControllerLifecycleDelegate

- (void)viewControllerDidAppear:(UIViewController *)controller {
    // cold reboot
    if (self.firstVCDidAppearTime == 0) {
        self.firstVCDidAppearTime = [GrowingTimeUtil currentSystemTimeMillis];
    }
    
    [self sendColdReboot];
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
