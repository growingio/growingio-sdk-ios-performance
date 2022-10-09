//
//  GrowingAPMCrashMonitor.h
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

#import "GrowingCrash.h"
#import "GrowingCrashC.h"
#import "GrowingCrashCString.h"
#import "GrowingCrashInstallation.h"
#import "GrowingCrashInstallation+Private.h"
#import "GrowingCrashMonitorType.h"
#import "GrowingCrashReportFilterAppleFmt.h"
#import "GrowingCrashReportFilterBasic.h"
#import "GrowingCrashReportFields.h"
#import "GrowingCrashReportFilter.h"
#import "GrowingCrashReportWriter.h"

#import "GrowingAPMMonitor.h"

NS_ASSUME_NONNULL_BEGIN

typedef void(^GrowingAPMCrashMonitorBlock)(NSArray *filteredReports, BOOL completed, NSError *error);

@interface GrowingAPMCrashMonitor : NSObject <GrowingAPMMonitor>

@property (nonatomic, copy) GrowingAPMCrashMonitorBlock monitorBlock;

@end

NS_ASSUME_NONNULL_END
