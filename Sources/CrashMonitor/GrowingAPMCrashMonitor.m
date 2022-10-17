//
//  GrowingAPMCrashMonitor.m
//  GrowingAnalytics
//
//  Created by YoloMao on 2022/9/28.
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

#import "GrowingAPMCrashMonitor.h"
#import "GrowingAPMMonitor.h"
#import "GrowingAPM+Private.h"
#import "GrowingCrashInstallation.h"

@interface GrowingAPMCrashMonitor () <GrowingAPMMonitor>

@end

@implementation GrowingAPMCrashMonitor

#pragma mark - Monitor

- (void)startMonitor {
    __weak typeof(self) weakSelf = self;
    dispatch_async(dispatch_get_main_queue(), ^{
        [GrowingAPM.crashInstallation sendAllReportsWithCompletion:^(NSArray *filteredReports, BOOL completed, NSError *error) {
            if (weakSelf.monitorBlock) {
                weakSelf.monitorBlock(filteredReports, completed, error);
            }
        }];
    });
}

@end
