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

#import "GrowingAPM.h"
#import "GrowingAPM+Private.h"
#import "GrowingAPMConfig.h"
#import "GrowingCrashInstallation.h"

@interface GrowingAPM ()

@property (nonatomic, copy) GrowingAPMConfig *config;
@property (nonatomic, strong) GrowingCrashInstallation *crashInstallation;

@end

@implementation GrowingAPM

+ (instancetype)sharedInstance {
    static GrowingAPM *sharedInstance = nil;
    static dispatch_once_t onceToken;
    dispatch_once(&onceToken, ^{
        sharedInstance = [[GrowingAPM alloc] init];
        [sharedInstance prepare];
    });
    return sharedInstance;
}

+ (void)startWithConfig:(GrowingAPMConfig *)config {
    GrowingAPM *apm = GrowingAPM.sharedInstance;
    apm.config = config;
    
    dispatch_async(dispatch_get_main_queue(), ^{
        [apm.crashInstallation sendAllReportsWithCompletion:nil];
    });
}

+ (void)swizzle:(GrowingAPMMonitors)monitors {
    GrowingAPM *apm = GrowingAPM.sharedInstance;
    if (monitors & GrowingAPMMonitorsLaunch) {
        
    }
    
    if (monitors & GrowingAPMMonitorsUserInterface) {
        
    }
    
    if (monitors & GrowingAPMMonitorsCrash) {
        [apm.crashInstallation install];
    }
    
    if (monitors & GrowingAPMMonitorsNetwork) {
        
    }
}

- (void)prepare {
    // override by category
}

@end
