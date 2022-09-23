//  Created by Karl Stenerud on 2012-05-11.
//
//  Copyright (c) 2012 Karl Stenerud. All rights reserved.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall remain in place
// in this source code.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//
//  GrowingCrashReportFilterBasic.h
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


#import "GrowingCrashReportFilter.h"


/**
 * Very basic filter that passes through reports untouched.
 *
 * Input: Anything.
 * Output: Same as input (passthrough).
 */
@interface GrowingCrashReportFilterPassthrough : NSObject <GrowingCrashReportFilter>

+ (GrowingCrashReportFilterPassthrough*) filter;

@end


/**
 * Passes reports to a series of subfilters, then stores the results of those operations
 * as keyed values in final master reports.
 *
 * Input: Anything
 * Output: NSDictionary
 */
@interface GrowingCrashReportFilterCombine : NSObject <GrowingCrashReportFilter>

/** Constructor.
 *
 * @param firstFilter The first filter, followed by key, filter, key, ...
 *                    Each "filter" can be id<GrowingCrashReportFilter> or an NSArray
 *                    of filters (which gets wrapped in a pipeline filter).
 */
+ (GrowingCrashReportFilterCombine*) filterWithFiltersAndKeys:(id) firstFilter, ... NS_REQUIRES_NIL_TERMINATION;

/** Initializer.
 *
 * @param firstFilter The first filter, followed by key, filter, key, ...
 *                    Each "filter" can be id<GrowingCrashReportFilter> or an NSArray
 *                    of filters (which gets wrapped in a pipeline filter).
 */
- (id) initWithFiltersAndKeys:(id)firstFilter, ... NS_REQUIRES_NIL_TERMINATION;

@end


/**
 * A pipeline of filters. Reports get passed through each subfilter in order.
 *
 * Input: Depends on what's in the pipeline.
 * Output: Depends on what's in the pipeline.
 */
@interface GrowingCrashReportFilterPipeline : NSObject <GrowingCrashReportFilter>

/** The filters in this pipeline. */
@property(nonatomic,readonly,retain) NSArray* filters;

/** Constructor.
 *
 * @param firstFilter The first filter, followed by filter, filter, ...
 */
+ (GrowingCrashReportFilterPipeline*) filterWithFilters:(id) firstFilter, ... NS_REQUIRES_NIL_TERMINATION;

/** Initializer.
 *
 * @param firstFilter The first filter, followed by filter, filter, ...
 */
- (id) initWithFilters:(id) firstFilter, ... NS_REQUIRES_NIL_TERMINATION;

- (void) addFilter:(id<GrowingCrashReportFilter>) filter;

@end


/**
 * Extracts data associated with a key from each report.
 */
@interface GrowingCrashReportFilterObjectForKey : NSObject <GrowingCrashReportFilter>

/** Constructor.
 *
 * @param key The key to search for in each report. If the key is a string,
 *            it will be interpreted as a key path.
 * @param allowNotFound If NO, filtering will stop with an error if the key
 *                      was not found in a report.
 */
+ (GrowingCrashReportFilterObjectForKey*) filterWithKey:(id) key
                                     allowNotFound:(BOOL) allowNotFound;

/** Initializer.
 *
 * @param key The key to search for in each report. If the key is a string,
 *            it will be interpreted as a key path.
 * @param allowNotFound If NO, filtering will stop with an error if the key
 *                      was not found in a report.
 */
- (id) initWithKey:(id) key
     allowNotFound:(BOOL) allowNotFound;

@end


/**
 * Takes values by key from the report and concatenates their string representations.
 *
 * Input: NSDictionary
 * Output: NSString
 */
@interface GrowingCrashReportFilterConcatenate : NSObject <GrowingCrashReportFilter>

/** Constructor.
 *
 * @param separatorFmt Formatting text to use when separating the values. You may include
 *                     %@ in the formatting text to include the key name as well.
 * @param firstKey Series of keys to extract from the source report.
 */
+ (GrowingCrashReportFilterConcatenate*) filterWithSeparatorFmt:(NSString*) separatorFmt
                                                      keys:(id) firstKey, ... NS_REQUIRES_NIL_TERMINATION;

/** Constructor.
 *
 * @param separatorFmt Formatting text to use when separating the values. You may include
 *                     %@ in the formatting text to include the key name as well.
 * @param firstKey Series of keys to extract from the source report.
 */
- (id) initWithSeparatorFmt:(NSString*) separatorFmt
                       keys:(id) firstKey, ... NS_REQUIRES_NIL_TERMINATION;

@end


/**
 * Fetches subsets of data from the source reports. All other data is discarded.
 *
 * Input: NSDictionary
 * Output: NSDictionary
 */
@interface GrowingCrashReportFilterSubset : NSObject <GrowingCrashReportFilter>

/** Constructor.
 *
 * @param firstKeyPath Series of key paths to search in the source reports.
 */
+ (GrowingCrashReportFilterSubset*) filterWithKeys:(id) firstKeyPath, ... NS_REQUIRES_NIL_TERMINATION;

/** Initializer.
 *
 * @param firstKeyPath Series of key paths to search in the source reports.
 */
- (id) initWithKeys:(id) firstKeyPath, ... NS_REQUIRES_NIL_TERMINATION;

@end


/**
 * Convert UTF-8 data to an NSString.
 *
 * Input: NSData
 * Output: NSString
 */
@interface GrowingCrashReportFilterDataToString : NSObject <GrowingCrashReportFilter>

+ (GrowingCrashReportFilterDataToString*) filter;

@end


/**
 * Convert NSString to UTF-8 encoded NSData.
 *
 * Input: NSString
 * Output: NSData
 */
@interface GrowingCrashReportFilterStringToData : NSObject <GrowingCrashReportFilter>

+ (GrowingCrashReportFilterStringToData*) filter;

@end
