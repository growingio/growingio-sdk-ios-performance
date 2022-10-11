//  Created by Karl Stenerud on 2012-08-19.
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
//  GrowingCrashVarArgs.h
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


/* GrowingCrashVarArgs is a set of macros designed to make dealing with variable arguments
 * easier in Objective-C. All macros assume that the varargs list contains only
 * objective-c objects or object-like structures (assignable to type id).
 *
 * The base macro growingcrashva_iterate_list() iterates over the variable arguments,
 * invoking a block for each argument, until it encounters a terminating nil.
 *
 * The other macros are for convenience when converting to common collections.
 */


/** Block type used by growingcrashva_iterate_list.
 *
 * @param entry The current argument in the vararg list.
 */
typedef void (^GrowingCrashVA_Block)(id entry);


/**
 * Iterate over a va_list, executing the specified code block for each entry.
 *
 * @param FIRST_ARG_NAME The name of the first argument in the vararg list.
 * @param BLOCK A code block of type GrowingCrashVA_Block.
 */
#define growingcrashva_iterate_list(FIRST_ARG_NAME, BLOCK) \
{ \
    GrowingCrashVA_Block growingcrashva_block = BLOCK; \
    va_list growingcrashva_args; \
    va_start(growingcrashva_args,FIRST_ARG_NAME); \
    for(id growingcrashva_arg = FIRST_ARG_NAME; growingcrashva_arg != nil; growingcrashva_arg = va_arg(growingcrashva_args, id)) \
    { \
        growingcrashva_block(growingcrashva_arg); \
    } \
    va_end(growingcrashva_args); \
}

/**
 * Convert a variable argument list into an array. An autoreleased
 * NSMutableArray will be created in the current scope with the specified name.
 *
 * @param FIRST_ARG_NAME The name of the first argument in the vararg list.
 * @param ARRAY_NAME The name of the array to create in the current scope.
 */
#define growingcrashva_list_to_nsarray(FIRST_ARG_NAME, ARRAY_NAME) \
    NSMutableArray* ARRAY_NAME = [NSMutableArray array]; \
    growingcrashva_iterate_list(FIRST_ARG_NAME, ^(id entry) \
    { \
        [ARRAY_NAME addObject:entry]; \
    })

/**
 * Convert a variable argument list into a dictionary, interpreting the vararg
 * list as object, key, object, key, ...
 * An autoreleased NSMutableDictionary will be created in the current scope with
 * the specified name.
 *
 * @param FIRST_ARG_NAME The name of the first argument in the vararg list.
 * @param DICT_NAME The name of the dictionary to create in the current scope.
 */
#define growingcrashva_list_to_nsdictionary(FIRST_ARG_NAME, DICT_NAME) \
    NSMutableDictionary* DICT_NAME = [NSMutableDictionary dictionary]; \
    { \
        __block id growingcrashva_object = nil; \
        growingcrashva_iterate_list(FIRST_ARG_NAME, ^(id entry) \
        { \
            if(growingcrashva_object == nil) \
            { \
                growingcrashva_object = entry; \
            } \
            else \
            { \
                [DICT_NAME setObject:growingcrashva_object forKey:entry]; \
                growingcrashva_object = nil; \
            } \
        }); \
    }
