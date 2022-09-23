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
//  GrowingCrashCString.m
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

#import "GrowingCrashCString.h"

@implementation GrowingCrashCString

@synthesize length = _length;
@synthesize bytes = _bytes;

+ (GrowingCrashCString*) stringWithString:(NSString*) string
{
    return [[self alloc] initWithString:string];
}

+ (GrowingCrashCString*) stringWithCString:(const char*) string
{
    return [[self alloc] initWithCString:string];
}

+ (GrowingCrashCString*) stringWithData:(NSData*) data
{
    return [[self alloc] initWithData:data];
}

+ (GrowingCrashCString*) stringWithData:(const char*) data length:(NSUInteger) length
{
    return [[self alloc] initWithData:data length:length];
}

- (id) initWithString:(NSString*) string
{
    return [self initWithCString:[string cStringUsingEncoding:NSUTF8StringEncoding]];
}

- (id) initWithCString:(const char*) string
{
    if((self = [super init]))
    {
        _bytes = strdup(string);
        _length = strlen(_bytes);
    }
    return self;
}

- (id) initWithData:(NSData*) data
{
    return [self initWithData:data.bytes length:data.length];
}

- (id) initWithData:(const char*) data length:(NSUInteger) length
{
    if((self = [super init]))
    {
        _length = length;
        char* bytes = malloc((unsigned)_length+1);
        memcpy(bytes, data, _length);
        bytes[_length] = 0;
        _bytes = bytes;
    }
    return self;
}

- (void) dealloc
{
    if(_bytes != NULL)
    {
        free((void*)_bytes);
    }
}

@end
