//  Created by Karl Stenerud on 2013-02-10.
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
//  GrowingCrashInstallation+Private.h
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


#import "GrowingCrashInstallation.h"


/** Implement a property to be used as a "key". */
#define IMPLEMENT_REPORT_KEY_PROPERTY(NAME, NAMEUPPER) \
@synthesize NAME##Key = _##NAME##Key; \
- (void) set##NAMEUPPER##Key:(NSString*) value \
{ \
    _##NAME##Key; \
    _##NAME##Key = value; \
    [self reportFieldForProperty:@#NAME setKey:value]; \
}

/** Implement a property to be used as a "value". */
#define IMPLEMENT_REPORT_VALUE_PROPERTY(NAME, NAMEUPPER, TYPE) \
@synthesize NAME = _##NAME; \
- (void) set##NAMEUPPER:(TYPE) value \
{ \
    _##NAME; \
    _##NAME = value; \
    [self reportFieldForProperty:@#NAME setValue:value]; \
}

/** Implement a standard report property (with key and value properties) */
#define IMPLEMENT_REPORT_PROPERTY(NAME, NAMEUPPER, TYPE) \
IMPLEMENT_REPORT_VALUE_PROPERTY(NAME, NAMEUPPER, TYPE) \
IMPLEMENT_REPORT_KEY_PROPERTY(NAME, NAMEUPPER)


@interface GrowingCrashInstallation ()

/** Initializer.
 *
 * @param requiredProperties Properties that MUST be set when sending reports.
 */
- (id) initWithRequiredProperties:(NSArray*) requiredProperties;

/** Set the key to be used for the specified report property.
 *
 * @param propertyName The name of the property.
 * @param key The key to use.
 */
- (void) reportFieldForProperty:(NSString*) propertyName setKey:(id) key;

/** Set the value of the specified report property.
 *
 * @param propertyName The name of the property.
 * @param value The value to set.
 */
- (void) reportFieldForProperty:(NSString*) propertyName setValue:(id) value;

/** Create a new sink. Subclasses must implement this.
 */
- (id<GrowingCrashReportFilter>) sink;

/** Make an absolute key path if the specified path is not already absolute. */
- (NSString*) makeKeyPath:(NSString*) keyPath;

/** Make an absolute key paths from the specified paths. */
- (NSArray*) makeKeyPaths:(NSArray*) keyPaths;

@end
