//  Created by Karl Stenerud on 2012-01-08.
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
//  GrowingCrashJSONCodecObjC.m
//  GrowingAnalytics
//
//  Created by YoloMao on 2022/9/19.
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


#import "GrowingCrashJSONCodecObjC.h"

#import "GrowingCrashJSONCodec.h"
#import "NSError+GrowingCrashSimpleConstructor.h"
#import "GrowingCrashDate.h"


@interface GrowingCrashJSONCodec ()

#pragma mark Properties

/** Callbacks from the C library */
@property(nonatomic,readwrite,assign) GrowingCrashJSONDecodeCallbacks* callbacks;

/** Stack of arrays/objects as the decoded content is built */
@property(nonatomic,readwrite,retain) NSMutableArray* containerStack;

/** Current array or object being decoded (weak ref) */
@property(nonatomic,readwrite,assign) id currentContainer;

/** Top level array or object in the decoded tree */
@property(nonatomic,readwrite,retain) id topLevelContainer;

/** Data that has been serialized into JSON form */
@property(nonatomic,readwrite,retain) NSMutableData* serializedData;

/** Any error that has occurred */
@property(nonatomic,readwrite,retain) NSError* error;

/** If true, pretty print while encoding */
@property(nonatomic,readwrite,assign) bool prettyPrint;

/** If true, sort object keys while encoding */
@property(nonatomic,readwrite,assign) bool sorted;

/** If true, don't store nulls in arrays */
@property(nonatomic,readwrite,assign) bool ignoreNullsInArrays;

/** If true, don't store nulls in objects */
@property(nonatomic,readwrite,assign) bool ignoreNullsInObjects;


#pragma mark Constructors

/** Convenience constructor.
 *
 * @param encodeOptions Optional behavior when encoding to JSON.
 *
 * @param decodeOptions Optional behavior when decoding from JSON.
 *
 * @return A new codec.
 */
+ (GrowingCrashJSONCodec*) codecWithEncodeOptions:(GrowingCrashJSONEncodeOption) encodeOptions
                          decodeOptions:(GrowingCrashJSONDecodeOption) decodeOptions;

/** Initializer.
 *
 * @param encodeOptions Optional behavior when encoding to JSON.
 *
 * @param decodeOptions Optional behavior when decoding from JSON.
 *
 * @return The initialized codec.
 */
- (id) initWithEncodeOptions:(GrowingCrashJSONEncodeOption) encodeOptions
               decodeOptions:(GrowingCrashJSONDecodeOption) decodeOptions;

@end


#pragma mark -
#pragma mark -


@implementation GrowingCrashJSONCodec

#pragma mark Properties

@synthesize topLevelContainer = _topLevelContainer;
@synthesize currentContainer = _currentContainer;
@synthesize containerStack = _containerStack;
@synthesize callbacks = _callbacks;
@synthesize serializedData = _serializedData;
@synthesize error = _error;
@synthesize prettyPrint = _prettyPrint;
@synthesize sorted = _sorted;
@synthesize ignoreNullsInArrays = _ignoreNullsInArrays;
@synthesize ignoreNullsInObjects = _ignoreNullsInObjects;

#pragma mark Constructors/Destructor

+ (GrowingCrashJSONCodec*) codecWithEncodeOptions:(GrowingCrashJSONEncodeOption) encodeOptions
                          decodeOptions:(GrowingCrashJSONDecodeOption) decodeOptions
{
    return [[self alloc] initWithEncodeOptions:encodeOptions decodeOptions:decodeOptions];
}

- (id) initWithEncodeOptions:(GrowingCrashJSONEncodeOption) encodeOptions
               decodeOptions:(GrowingCrashJSONDecodeOption) decodeOptions
{
    if((self = [super init]))
    {
        self.containerStack = [NSMutableArray array];
        self.callbacks = malloc(sizeof(*self.callbacks));
        self.callbacks->onBeginArray = onBeginArray;
        self.callbacks->onBeginObject = onBeginObject;
        self.callbacks->onBooleanElement = onBooleanElement;
        self.callbacks->onEndContainer = onEndContainer;
        self.callbacks->onEndData = onEndData;
        self.callbacks->onFloatingPointElement = onFloatingPointElement;
        self.callbacks->onIntegerElement = onIntegerElement;
        self.callbacks->onNullElement = onNullElement;
        self.callbacks->onStringElement = onStringElement;
        self.prettyPrint = (encodeOptions & GrowingCrashJSONEncodeOptionPretty) != 0;
        self.sorted = (encodeOptions & GrowingCrashJSONEncodeOptionSorted) != 0;
        self.ignoreNullsInArrays = (decodeOptions & GrowingCrashJSONDecodeOptionIgnoreNullInArray) != 0;
        self.ignoreNullsInObjects = (decodeOptions & GrowingCrashJSONDecodeOptionIgnoreNullInObject) != 0;
    }
    return self;
}

- (void) dealloc
{
    free(self.callbacks);
}

#pragma mark Utility

static inline NSString* stringFromCString(const char* const string)
{
    if(string == NULL)
    {
        return nil;
    }
    return [NSString stringWithCString:string encoding:NSUTF8StringEncoding];
}

#pragma mark Callbacks

static int onElement(GrowingCrashJSONCodec* codec, NSString* name, id element)
{
    if(codec->_currentContainer == nil)
    {
        codec.error = [NSError growingCrash_errorWithDomain:@"GrowingCrashJSONCodecObjC"
                                                       code:0
                                                description:@"Type %@ not allowed as top level container",
                       [element class]];
        return GrowingCrashJSON_ERROR_INVALID_DATA;
    }

    if([codec->_currentContainer isKindOfClass:[NSMutableDictionary class]])
    {
        [(NSMutableDictionary*)codec->_currentContainer setValue:element
                                                          forKey:name];
    }
    else
    {
        [(NSMutableArray*)codec->_currentContainer addObject:element];
    }
    return GrowingCrashJSON_OK;
}

static int onBeginContainer(GrowingCrashJSONCodec* codec, NSString* name, id container)
{
    if(codec->_topLevelContainer == nil)
    {
        codec->_topLevelContainer = container;
    }
    else
    {
        int result = onElement(codec, name, container);
        if(result != GrowingCrashJSON_OK)
        {
            return result;
        }
    }
    codec->_currentContainer = container;
    [codec->_containerStack addObject:container];
    return GrowingCrashJSON_OK;
}

static int onBooleanElement(const char* const cName, const bool value, void* const userData)
{
    NSString* name = stringFromCString(cName);
    id element = [NSNumber numberWithBool:value];
    GrowingCrashJSONCodec* codec = (__bridge GrowingCrashJSONCodec*)userData;
    return onElement(codec, name, element);
}

static int onFloatingPointElement(const char* const cName, const double value, void* const userData)
{
    NSString* name = stringFromCString(cName);
    id element = [NSNumber numberWithDouble:value];
    GrowingCrashJSONCodec* codec = (__bridge GrowingCrashJSONCodec*)userData;
    return onElement(codec, name, element);
}

static int onIntegerElement(const char* const cName,
                                       const int64_t value,
                                       void* const userData)
{
    NSString* name = stringFromCString(cName);
    id element = [NSNumber numberWithLongLong:value];
    GrowingCrashJSONCodec* codec = (__bridge GrowingCrashJSONCodec*)userData;
    return onElement(codec, name, element);
}

static int onNullElement(const char* const cName, void* const userData)
{
    NSString* name = stringFromCString(cName);
    GrowingCrashJSONCodec* codec = (__bridge GrowingCrashJSONCodec*)userData;

    if((codec->_ignoreNullsInArrays &&
        [codec->_currentContainer isKindOfClass:[NSArray class]]) ||
       (codec->_ignoreNullsInObjects &&
        [codec->_currentContainer isKindOfClass:[NSDictionary class]]))
    {
        return GrowingCrashJSON_OK;
    }

    return onElement(codec, name, [NSNull null]);
}

static int onStringElement(const char* const cName, const char* const value, void* const userData)
{
    NSString* name = stringFromCString(cName);
    id element = [NSString stringWithCString:value
                                    encoding:NSUTF8StringEncoding];
    GrowingCrashJSONCodec* codec = (__bridge GrowingCrashJSONCodec*)userData;
    return onElement(codec, name, element);
}

static int onBeginObject(const char* const cName, void* const userData)
{
    NSString* name = stringFromCString(cName);
    id container = [NSMutableDictionary dictionary];
    GrowingCrashJSONCodec* codec = (__bridge GrowingCrashJSONCodec*)userData;
    return onBeginContainer(codec, name, container);
}

static int onBeginArray(const char* const cName, void* const userData)
{
    NSString* name = stringFromCString(cName);
    id container = [NSMutableArray array];
    GrowingCrashJSONCodec* codec = (__bridge GrowingCrashJSONCodec*)userData;
    return onBeginContainer(codec, name, container);
}

static int onEndContainer(void* const userData)
{
    GrowingCrashJSONCodec* codec = (__bridge GrowingCrashJSONCodec*)userData;

    if([codec->_containerStack count] == 0)
    {
        codec.error = [NSError growingCrash_errorWithDomain:@"GrowingCrashJSONCodecObjC"
                                                       code:0
                                                description:@"Already at the top level; no container left to end"];
        return GrowingCrashJSON_ERROR_INVALID_DATA;
    }
    [codec->_containerStack removeLastObject];
    NSUInteger count = [codec->_containerStack count];
    if(count > 0)
    {
        codec->_currentContainer = [codec->_containerStack objectAtIndex:count - 1];
    }
    else
    {
        codec->_currentContainer = nil;
    }
    return GrowingCrashJSON_OK;
}

static int onEndData(__unused void* const userData)
{
    return GrowingCrashJSON_OK;
}

static int addJSONData(const char* const bytes, const int length, void* const userData)
{
    NSMutableData* data = (__bridge NSMutableData*)userData;
    [data appendBytes:bytes length:(unsigned)length];
    return GrowingCrashJSON_OK;
}

static int encodeObject(GrowingCrashJSONCodec* codec, id object, NSString* name, GrowingCrashJSONEncodeContext* context)
{
    int result;
    const char* cName = [name UTF8String];
    if([object isKindOfClass:[NSString class]])
    {
        NSData* data = [object dataUsingEncoding:NSUTF8StringEncoding];
        result = growingcrashjson_addStringElement(context, cName, data.bytes, (int)data.length);
        if(result == GrowingCrashJSON_ERROR_INVALID_CHARACTER)
        {
            codec.error = [NSError growingCrash_errorWithDomain:@"GrowingCrashJSONCodecObjC"
                                                           code:0
                                                    description:@"Invalid character in %@", object];
        }
        return result;
    }

    if([object isKindOfClass:[NSNumber class]])
    {
        switch (CFNumberGetType((__bridge CFNumberRef)object))
        {
            case kCFNumberFloat32Type:
            case kCFNumberFloat64Type:
            case kCFNumberFloatType:
            case kCFNumberCGFloatType:
            case kCFNumberDoubleType:
                return growingcrashjson_addFloatingPointElement(context, cName, [object doubleValue]);
            case kCFNumberCharType:
                return growingcrashjson_addBooleanElement(context, cName, [object boolValue]);
            default:
                return growingcrashjson_addIntegerElement(context, cName, [object longLongValue]);
        }
    }

    if([object isKindOfClass:[NSArray class]])
    {
        if((result = growingcrashjson_beginArray(context, cName)) != GrowingCrashJSON_OK)
        {
            return result;
        }
        for(id subObject in object)
        {
            if((result = encodeObject(codec, subObject, NULL, context)) != GrowingCrashJSON_OK)
            {
                return result;
            }
        }
        return growingcrashjson_endContainer(context);
    }

    if([object isKindOfClass:[NSDictionary class]])
    {
        if((result = growingcrashjson_beginObject(context, cName)) != GrowingCrashJSON_OK)
        {
            return result;
        }
        NSArray* keys = [(NSDictionary*)object allKeys];
        if(codec->_sorted)
        {
            keys = [keys sortedArrayUsingSelector:@selector(compare:)];
        }
        for(id key in keys)
        {
            if((result = encodeObject(codec, [object valueForKey:key], key, context)) != GrowingCrashJSON_OK)
            {
                return result;
            }
        }
        return growingcrashjson_endContainer(context);
    }

    if([object isKindOfClass:[NSNull class]])
    {
        return growingcrashjson_addNullElement(context, cName);
    }

    if([object isKindOfClass:[NSDate class]])
    {
        char string[21];
        time_t timestamp = (time_t)((NSDate*)object).timeIntervalSince1970;
        growingcrashdate_utcStringFromTimestamp(timestamp, string);
        NSData* data = [NSData dataWithBytes:string length:strnlen(string, 20)];
        return growingcrashjson_addStringElement(context, cName, data.bytes, (int)data.length);
    }

    if([object isKindOfClass:[NSData class]])
    {
        NSData* data = (NSData*)object;
        return growingcrashjson_addDataElement(context, cName, data.bytes, (int)data.length);
    }

    codec.error = [NSError growingCrash_errorWithDomain:@"GrowingCrashJSONCodecObjC"
                                                   code:0
                                            description:@"Could not determine type of %@", [object class]];
    return GrowingCrashJSON_ERROR_INVALID_DATA;
}


#pragma mark Public API

+ (NSData*) encode:(id) object
           options:(GrowingCrashJSONEncodeOption) encodeOptions
             error:(NSError* __autoreleasing *) error
{
    NSMutableData* data = [NSMutableData data];
    GrowingCrashJSONEncodeContext JSONContext;
    growingcrashjson_beginEncode(&JSONContext,
                       encodeOptions & GrowingCrashJSONEncodeOptionPretty,
                       addJSONData,
                       (__bridge void*)data);
    GrowingCrashJSONCodec* codec = [self codecWithEncodeOptions:encodeOptions
                                        decodeOptions:GrowingCrashJSONDecodeOptionNone];

    int result = encodeObject(codec, object, NULL, &JSONContext);
    if(error != nil)
    {
        *error = codec.error;
    }
    return result == GrowingCrashJSON_OK ? data : nil;
}

+ (id) decode:(NSData*) JSONData
      options:(GrowingCrashJSONDecodeOption) decodeOptions
        error:(NSError* __autoreleasing *) error
{
    GrowingCrashJSONCodec* codec = [self codecWithEncodeOptions:0
                                        decodeOptions:decodeOptions];
    NSMutableData* stringData = [NSMutableData dataWithLength:10001];
    int errorOffset;
    int result = growingcrashjson_decode(JSONData.bytes,
                               (int)JSONData.length,
                               stringData.mutableBytes,
                               (int)stringData.length,
                               codec.callbacks,
                               (__bridge void*)codec, &errorOffset);
    if(result != GrowingCrashJSON_OK && codec.error == nil)
    {
        codec.error = [NSError growingCrash_errorWithDomain:@"GrowingCrashJSONCodecObjC"
                                                       code:0
                                                description:@"%s (offset %d)",
                       growingcrashjson_stringForError(result),
                       errorOffset];
    }
    if(error != nil)
    {
        *error = codec.error;
    }

    if(result != GrowingCrashJSON_OK && !(decodeOptions & GrowingCrashJSONDecodeOptionKeepPartialObject))
    {
        return nil;
    }
    return codec.topLevelContainer;
}

@end
