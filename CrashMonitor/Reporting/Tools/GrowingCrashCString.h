//  Created by Karl Stenerud on 2013-02-23.
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
//  GrowingCrashCString.h
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

#import <Foundation/Foundation.h>

/**
 * A string, stored C style with null termination.
 */
@interface GrowingCrashCString : NSObject

/** Length of the string in bytes (not characters!). Length does not include null terminator. */
@property(nonatomic,readonly,assign) NSUInteger length;

/** String contents, including null terminator */
@property(nonatomic,readonly,assign) const char* bytes;

/** Constructor for NSString */
+ (GrowingCrashCString*) stringWithString:(NSString*) string;

/** Constructor for null-terminated C string (assumes UTF-8 encoding). */
+ (GrowingCrashCString*) stringWithCString:(const char*) string;

/** Constructor for string contained in NSData (assumes UTF-8 encoding). */
+ (GrowingCrashCString*) stringWithData:(NSData*) data;

/** Constructor for non-terminated string (assumes UTF-8 encoding). */
+ (GrowingCrashCString*) stringWithData:(const char*) data length:(NSUInteger) length;

- (id) initWithString:(NSString*) string;
- (id) initWithCString:(const char*) string;
- (id) initWithData:(NSData*) data;
- (id) initWithData:(const char*) data length:(NSUInteger) length;

@end
