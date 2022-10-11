//
//  GrowingAPMUIMonitor.m
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

#import "GrowingAPMUIMonitor.h"
#import "GrowingAppLifecycle.h"

@interface GrowingAPMUIMonitor () <GrowingAppLifecycleDelegate>

@property (nonatomic, strong) NSMutableArray *ignoredPrivateControllers;
@property (nonatomic, copy) NSString *lastPageName;

@end

@implementation GrowingAPMUIMonitor

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
}

#pragma mark - Private Method

- (void)pageLoadCompletedWithViewController:(UIViewController *)viewController
                               loadViewTime:(double)loadViewTime
                            viewDidLoadTime:(double)viewDidLoadTime
                         viewWillAppearTime:(double)viewWillAppearTime
                          viewDidAppearTime:(double)viewDidAppearTime {
    NSString *pageName = NSStringFromClass([viewController class]);
    if ([pageName hasPrefix:@"GrowingTK"]) {
        return;
    }
    if ([self.ignoredPrivateControllers containsObject:pageName]) {
        return;
    }
    
    self.lastPageName = pageName;
    if (self.monitorBlock) {
        double loadDuration = loadViewTime + viewDidLoadTime + viewWillAppearTime + viewDidAppearTime;
        self.monitorBlock(pageName, loadDuration);
    }
}

#pragma mark - GrowingAppLifecycleDelegate

- (void)applicationDidBecomeActive {
    if (!self.lastPageName) {
        return;
    }
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
        self.monitorBlock(self.lastPageName, duration);
    }
}

#pragma mark - Getter & Setter

- (NSMutableArray *)ignoredPrivateControllers {
    if (!_ignoredPrivateControllers) {
        _ignoredPrivateControllers = [NSMutableArray arrayWithArray:@[
            @"UIInputWindowController",
            @"UIActivityGroupViewController",
            @"UIKeyboardHiddenViewController",
            @"UICompatibilityInputViewController",
            @"UISystemInputAssistantViewController",
            @"UIPredictionViewController",
            @"GrowingWindowViewController",
            @"UIApplicationRotationFollowingController",
            @"UIAlertController",
            @"FlutterViewController",
            @"UIEditingOverlayViewController" // did not call [super viewDidAppear:animated]
        ]];
    }
    return _ignoredPrivateControllers;
}

@end
