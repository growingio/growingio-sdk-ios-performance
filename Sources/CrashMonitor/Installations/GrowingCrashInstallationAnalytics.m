//
//  GrowingCrashInstallationAnalytics.m
//  GrowingAnalytics
//
//  Created by YoloMao on 2022/9/23.
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

#import "GrowingCrashInstallationAnalytics.h"
#import "GrowingCrashInstallation+Private.h"
#import "GrowingCrashReportFilterBasic.h"
#import "GrowingCrashReportFilterAppleFmt.h"

@implementation GrowingCrashInstallationAnalytics

+ (instancetype)sharedInstance {
    static GrowingCrashInstallationAnalytics *sharedInstance = nil;
    static dispatch_once_t onceToken;
    dispatch_once(&onceToken, ^{
        sharedInstance = [[GrowingCrashInstallationAnalytics alloc] initWithRequiredProperties:nil];
    });
    return sharedInstance;
}

- (id <GrowingCrashReportFilter>)sink {
    id originFilter = [GrowingCrashReportFilterPassthrough filter];
    id errorFilter = [GrowingCrashReportFilterObjectForKey filterWithKey:@"crash/error" allowNotFound:YES];
    id formatFilter = [GrowingCrashReportFilterAppleFmt filterWithReportStyle:GrowingCrashAppleReportStyleSymbolicated];
    return [GrowingCrashReportFilterCombine filterWithFiltersAndKeys:originFilter, @"OriginReport",
                                                                     errorFilter, @"errorReport",
                                                                     formatFilter, @"AppleFmt",
                                                                     nil];
}

@end
