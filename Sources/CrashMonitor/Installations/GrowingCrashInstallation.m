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
//  GrowingCrashInstallation.m
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
#import "GrowingCrashInstallation+Private.h"
#import "GrowingCrashReportFilterBasic.h"
#import "GrowingCrash.h"
#import "GrowingCrashCString.h"
#import "GrowingCrashJSONCodecObjC.h"
#import "GrowingCrashLogger.h"
#import "NSError+GrowingCrashSimpleConstructor.h"
#import <objc/runtime.h>


/** Max number of properties that can be defined for writing to the report */
#define kMaxProperties 500


typedef struct
{
    const char* key;
    const char* value;
} ReportField;

typedef struct
{
    GrowingCrashReportWriteCallback userCrashCallback;
    int reportFieldsCount;
    ReportField* reportFields[0];
} CrashHandlerData;


static CrashHandlerData* g_crashHandlerData;


static void crashCallback(const GrowingCrashReportWriter* writer)
{
    for(int i = 0; i < g_crashHandlerData->reportFieldsCount; i++)
    {
        ReportField* field = g_crashHandlerData->reportFields[i];
        if(field->key != NULL && field->value != NULL)
        {
            writer->addJSONElement(writer, field->key, field->value, true);
        }
    }
    if(g_crashHandlerData->userCrashCallback != NULL)
    {
        g_crashHandlerData->userCrashCallback(writer);
    }
}


@interface GrowingCrashInstReportField: NSObject

@property(nonatomic,readonly,assign) int index;
@property(nonatomic,readonly,assign) ReportField* field;

@property(nonatomic,readwrite,retain) NSString* key;
@property(nonatomic,readwrite,retain) id value;

@property(nonatomic,readwrite,retain) NSMutableData* fieldBacking;
@property(nonatomic,readwrite,retain) GrowingCrashCString* keyBacking;
@property(nonatomic,readwrite,retain) GrowingCrashCString* valueBacking;

@end

@implementation GrowingCrashInstReportField

@synthesize index = _index;
@synthesize key = _key;
@synthesize value = _value;
@synthesize fieldBacking = _fieldBacking;
@synthesize keyBacking = _keyBacking;
@synthesize valueBacking= _valueBacking;

+ (GrowingCrashInstReportField*) fieldWithIndex:(int) index
{
    return [(GrowingCrashInstReportField*)[self alloc] initWithIndex:index];
}

- (id) initWithIndex:(int) index
{
    if((self = [super init]))
    {
        _index = index;
        self.fieldBacking = [NSMutableData dataWithLength:sizeof(*self.field)];
    }
    return self;
}

- (ReportField*) field
{
    return (ReportField*)self.fieldBacking.mutableBytes;
}

- (void) setKey:(NSString*) key
{
    _key = key;
    if(key == nil)
    {
        self.keyBacking = nil;
    }
    else
    {
        self.keyBacking = [GrowingCrashCString stringWithString:key];
    }
    self.field->key = self.keyBacking.bytes;
}

- (void) setValue:(id) value
{
    if(value == nil)
    {
        _value = nil;
        self.valueBacking = nil;
        return;
    }
    
    NSError* error = nil;
    NSData* jsonData = [GrowingCrashJSONCodec encode:value options:GrowingCrashJSONEncodeOptionPretty | GrowingCrashJSONEncodeOptionSorted error:&error];
    if(jsonData == nil)
    {
        GrowingCrashLOG_ERROR(@"Could not set value %@ for property %@: %@", value, self.key, error);
    }
    else
    {
        _value = value;
        self.valueBacking = [GrowingCrashCString stringWithData:jsonData];
        self.field->value = self.valueBacking.bytes;
    }
}

@end

@interface GrowingCrashInstallation ()

@property(nonatomic,readwrite,assign) int nextFieldIndex;
@property(nonatomic,readonly,assign) CrashHandlerData* crashHandlerData;
@property(nonatomic,readwrite,retain) NSMutableData* crashHandlerDataBacking;
@property(nonatomic,readwrite,retain) NSMutableDictionary* fields;
@property(nonatomic,readwrite,retain) NSArray* requiredProperties;
@property(nonatomic,readwrite,retain) GrowingCrashReportFilterPipeline* prependedFilters;

@end


@implementation GrowingCrashInstallation

@synthesize nextFieldIndex = _nextFieldIndex;
@synthesize crashHandlerDataBacking = _crashHandlerDataBacking;
@synthesize fields = _fields;
@synthesize requiredProperties = _requiredProperties;
@synthesize prependedFilters = _prependedFilters;

- (id) init
{
    [NSException raise:NSInternalInconsistencyException
                format:@"%@ does not support init. Subclasses must call initWithMaxReportFieldCount:requiredProperties:", [self class]];
    return nil;
}

- (id) initWithRequiredProperties:(NSArray*) requiredProperties
{
    if((self = [super init]))
    {
        self.crashHandlerDataBacking = [NSMutableData dataWithLength:sizeof(*self.crashHandlerData) +
                                        sizeof(*self.crashHandlerData->reportFields) * kMaxProperties];
        self.fields = [NSMutableDictionary dictionary];
        self.requiredProperties = requiredProperties;
        self.prependedFilters = [GrowingCrashReportFilterPipeline filterWithFilters:nil];
    }
    return self;
}

- (void) dealloc
{
    GrowingCrash* handler = [GrowingCrash sharedInstance];
    @synchronized(handler)
    {
        if(g_crashHandlerData == self.crashHandlerData)
        {
            g_crashHandlerData = NULL;
            handler.onCrash = NULL;
        }
    }
}

- (CrashHandlerData*) crashHandlerData
{
    return (CrashHandlerData*)self.crashHandlerDataBacking.mutableBytes;
}

- (GrowingCrashInstReportField*) reportFieldForProperty:(NSString*) propertyName
{
    GrowingCrashInstReportField* field = [self.fields objectForKey:propertyName];
    if(field == nil)
    {
        field = [GrowingCrashInstReportField fieldWithIndex:self.nextFieldIndex];
        self.nextFieldIndex++;
        self.crashHandlerData->reportFieldsCount = self.nextFieldIndex;
        self.crashHandlerData->reportFields[field.index] = field.field;
        [self.fields setObject:field forKey:propertyName];
    }
    return field;
}

- (void) reportFieldForProperty:(NSString*) propertyName setKey:(id) key
{
    GrowingCrashInstReportField* field = [self reportFieldForProperty:propertyName];
    field.key = key;
}

- (void) reportFieldForProperty:(NSString*) propertyName setValue:(id) value
{
    GrowingCrashInstReportField* field = [self reportFieldForProperty:propertyName];
    field.value = value;
}

- (NSError*) validateProperties
{
    NSMutableString* errors = [NSMutableString string];
    for(NSString* propertyName in self.requiredProperties)
    {
        NSString* nextError = nil;
        @try
        {
            id value = [self valueForKey:propertyName];
            if(value == nil)
            {
                nextError = @"is nil";
            }
        }
        @catch (NSException *exception)
        {
            nextError = @"property not found";
        }
        if(nextError != nil)
        {
            if([errors length] > 0)
            {
                [errors appendString:@", "];
            }
            [errors appendFormat:@"%@ (%@)", propertyName, nextError];
        }
    }
    if([errors length] > 0)
    {
        return [NSError growingCrash_errorWithDomain:[[self class] description]
                                   code:0
                            description:@"Installation properties failed validation: %@", errors];
    }
    return nil;
}

- (NSString*) makeKeyPath:(NSString*) keyPath
{
    if([keyPath length] == 0)
    {
        return keyPath;
    }
    BOOL isAbsoluteKeyPath = [keyPath length] > 0 && [keyPath characterAtIndex:0] == '/';
    return isAbsoluteKeyPath ? keyPath : [@"user/" stringByAppendingString:keyPath];
}

- (NSArray*) makeKeyPaths:(NSArray*) keyPaths
{
    if([keyPaths count] == 0)
    {
        return keyPaths;
    }
    NSMutableArray* result = [NSMutableArray arrayWithCapacity:[keyPaths count]];
    for(NSString* keyPath in keyPaths)
    {
        [result addObject:[self makeKeyPath:keyPath]];
    }
    return result;
}

- (GrowingCrashReportWriteCallback) onCrash
{
    @synchronized(self)
    {
        return self.crashHandlerData->userCrashCallback;
    }
}

- (void) setOnCrash:(GrowingCrashReportWriteCallback)onCrash
{
    @synchronized(self)
    {
        self.crashHandlerData->userCrashCallback = onCrash;
    }
}

- (void) install
{
    GrowingCrash* handler = [GrowingCrash sharedInstance];
    @synchronized(handler)
    {
        g_crashHandlerData = self.crashHandlerData;
        handler.onCrash = crashCallback;
        [handler install];
    }
}

- (void) sendAllReportsWithCompletion:(GrowingCrashReportFilterCompletion) onCompletion
{
    NSError* error = [self validateProperties];
    if(error != nil)
    {
        if(onCompletion != nil)
        {
            onCompletion(nil, NO, error);
        }
        return;
    }

    id<GrowingCrashReportFilter> sink = [self sink];
    if(sink == nil)
    {
        onCompletion(nil, NO, [NSError growingCrash_errorWithDomain:[[self class] description]
                                                  code:0
                                           description:@"Sink was nil (subclasses must implement method \"sink\")"]);
        return;
    }
    
    sink = [GrowingCrashReportFilterPipeline filterWithFilters:self.prependedFilters, sink, nil];

    GrowingCrash* handler = [GrowingCrash sharedInstance];
    handler.sink = sink;
    [handler sendAllReportsWithCompletion:onCompletion];
}

- (void) addPreFilter:(id<GrowingCrashReportFilter>) filter
{
    [self.prependedFilters addFilter:filter];
}

- (id<GrowingCrashReportFilter>) sink
{
    return nil;
}

@end
