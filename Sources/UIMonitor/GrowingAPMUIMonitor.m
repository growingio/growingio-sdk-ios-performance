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
#import "GrowingAPMMonitor.h"
#import "GrowingTimeUtil.h"
#import "GrowingAppLifecycle.h"
#import "UIViewController+GrowingUIMonitor.h"

@interface GrowingAPMUIMonitor () <GrowingAPMMonitor, GrowingAppLifecycleDelegate>

@property (nonatomic, copy) NSString *firstPageName;
@property (nonatomic, assign) double firstPageloadDuration;
@property (nonatomic, assign) double firstPageDidAppearTime;
@property (nonatomic, assign) BOOL didSendColdReboot;

@property (nonatomic, strong) NSMutableArray *ignoredPrivateControllers;

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
    
    // 延迟初始化时，补发 cold reboot (如果未发)
    dispatch_async(dispatch_get_main_queue(), ^{
        [self sendColdReboot];
    });
}

+ (void)setup {
    static dispatch_once_t onceToken;
    dispatch_once(&onceToken, ^{
        [UIViewController growingapm_startUIMonitorSwizzle];
    });
}

#pragma mark - Private Method

- (void)sendColdReboot {
    if (self.didSendColdReboot) {
        return;
    }
    
    if (self.firstPageName.length == 0
        || self.firstPageloadDuration == 0
        || self.firstPageDidAppearTime == 0
        || self.coldRebootBeginTime == 0) {
        return;
    }
    
    if (self.monitorBlock) {
        self.monitorBlock(self.firstPageName,
                          self.firstPageloadDuration,
                          self.firstPageDidAppearTime - self.coldRebootBeginTime,
                          NO);
        self.didSendColdReboot = YES;
    }
}

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
    
    double loadDuration = loadViewTime + viewDidLoadTime + viewWillAppearTime + viewDidAppearTime;
    
    if (!self.didSendColdReboot) {
        // cold reboot
        if (self.firstPageDidAppearTime == 0) {
            self.firstPageName = pageName;
            self.firstPageloadDuration = loadDuration;
            self.firstPageDidAppearTime = [GrowingTimeUtil currentSystemTimeMillis];
        }
        
        [self sendColdReboot];
    } else {
        // usual page loading
        if (self.monitorBlock) {
            self.monitorBlock(pageName, loadDuration, 0, NO);
        }
    }
}

- (UIViewController *)topViewController:(UIViewController *)controller {
    if ([controller isKindOfClass:[UINavigationController class]]) {
        return [self topViewController:[(UINavigationController *)controller topViewController]];
    } else if ([controller isKindOfClass:[UITabBarController class]]) {
        return [self topViewController:[(UITabBarController *)controller selectedViewController]];
    } else {
        return controller;
    }
    return nil;
}

- (UIWindow *)keyWindow {
    if (@available(iOS 13.0, *)) {
        for (UIWindowScene *windowScene in [UIApplication sharedApplication].connectedScenes) {
            if (windowScene.activationState == UISceneActivationStateForegroundActive) {
                NSArray *windows = windowScene.windows;
                for (UIWindow *window in windows) {
                    if (!window.hidden) {
                        return window;
                    }
                }
            }
        }
    }
    
    UIWindow *keyWindow = nil;
    if ([UIApplication.sharedApplication.delegate respondsToSelector:@selector(window)]) {
        keyWindow = UIApplication.sharedApplication.delegate.window;
    } else {
        NSArray *windows = [UIApplication sharedApplication].windows;
        for (UIWindow *window in windows) {
            if (!window.hidden) {
                keyWindow = window;
                break;
            }
        }
    }
    return keyWindow;
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
    
    // 动态获取当前页面
    UIViewController *curController = [self topViewController:self.keyWindow.rootViewController];
    while (curController.presentedViewController) {
        curController = [self topViewController:curController.presentedViewController];
    }
    if (!curController) {
        return;
    }
    
    // warm reboot
    double duration = appLifecycle.appDidBecomeActiveTime - appLifecycle.appWillEnterForegroundTime;
    if (self.monitorBlock) {
        self.monitorBlock(NSStringFromClass([curController class]), duration, duration, YES);
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
