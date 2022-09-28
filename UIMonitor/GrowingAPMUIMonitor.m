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
#import "GrowingViewControllerLifecycle.h"

@interface GrowingAPMUIMonitor () <GrowingViewControllerLifecycleDelegate>

@property (nonatomic, strong) NSMutableArray *ignoredPrivateControllers;

@end

@implementation GrowingAPMUIMonitor

#pragma mark - Monitor

- (void)startMonitor {
    [GrowingViewControllerLifecycle.sharedInstance addViewControllerLifecycleDelegate:self];
}

#pragma mark - GrowingViewControllerLifecycleDelegate

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
    
    if (self.monitorBlock) {
        double loadDuration = loadViewTime > 0 ? (viewDidAppearTime - loadViewTime) : (viewDidAppearTime - viewDidLoadTime);
        self.monitorBlock(pageName, loadDuration);
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
        ]];
    }
    return _ignoredPrivateControllers;
}

@end
