//  Created by Karl Stenerud on 11-06-25.
//
//  Copyright (c) 2011 Karl Stenerud. All rights reserved.
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
//  GrowingCrashLogger.h
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


/**
 * GrowingCrashLogger
 * ========
 *
 * Prints log entries to the console consisting of:
 * - Level (Error, Warn, Info, Debug, Trace)
 * - File
 * - Line
 * - Function
 * - Message
 *
 * Allows setting the minimum logging level in the preprocessor.
 *
 * Works in C or Objective-C contexts, with or without ARC, using CLANG or GCC.
 *
 *
 * =====
 * USAGE
 * =====
 *
 * Set the log level in your "Preprocessor Macros" build setting. You may choose
 * TRACE, DEBUG, INFO, WARN, ERROR. If nothing is set, it defaults to ERROR.
 *
 * Example: GrowingCrashLogger_Level=WARN
 *
 * Anything below the level specified for GrowingCrashLogger_Level will not be compiled
 * or printed.
 * 
 *
 * Next, include the header file:
 *
 * #include "GrowingCrashLogger.h"
 *
 *
 * Next, call the logger functions from your code (using objective-c strings
 * in objective-C files and regular strings in regular C files):
 *
 * Code:
 *    GrowingCrashLOG_ERROR(@"Some error message");
 *
 * Prints:
 *    2011-07-16 05:41:01.379 TestApp[4439:f803] ERROR: SomeClass.m (21): -[SomeFunction]: Some error message 
 *
 * Code:
 *    GrowingCrashLOG_INFO(@"Info about %@", someObject);
 *
 * Prints:
 *    2011-07-16 05:44:05.239 TestApp[4473:f803] INFO : SomeClass.m (20): -[SomeFunction]: Info about <NSObject: 0xb622840>
 *
 *
 * The "BASIC" versions of the macros behave exactly like NSLog() or printf(),
 * except they respect the GrowingCrashLogger_Level setting:
 *
 * Code:
 *    GrowingCrashLOGBASIC_ERROR(@"A basic log entry");
 *
 * Prints:
 *    2011-07-16 05:44:05.916 TestApp[4473:f803] A basic log entry
 *
 *
 * NOTE: In C files, use "" instead of @"" in the format field. Logging calls
 *       in C files do not print the NSLog preamble:
 *
 * Objective-C version:
 *    GrowingCrashLOG_ERROR(@"Some error message");
 *
 *    2011-07-16 05:41:01.379 TestApp[4439:f803] ERROR: SomeClass.m (21): -[SomeFunction]: Some error message
 *
 * C version:
 *    GrowingCrashLOG_ERROR("Some error message");
 *
 *    ERROR: SomeClass.c (21): SomeFunction(): Some error message
 *
 *
 * =============
 * LOCAL LOGGING
 * =============
 *
 * You can control logging messages at the local file level using the
 * "GrowingCrashLogger_LocalLevel" define. Note that it must be defined BEFORE
 * including GrowingCrashLogger.h
 *
 * The GrowingCrashLOG_XX() and GrowingCrashLOGBASIC_XX() macros will print out based on the LOWER
 * of GrowingCrashLogger_Level and GrowingCrashLogger_LocalLevel, so if GrowingCrashLogger_Level is DEBUG
 * and GrowingCrashLogger_LocalLevel is TRACE, it will print all the way down to the trace
 * level for the local file where GrowingCrashLogger_LocalLevel was defined, and to the
 * debug level everywhere else.
 *
 * Example:
 *
 * // GrowingCrashLogger_LocalLevel, if defined, MUST come BEFORE including GrowingCrashLogger.h
 * #define GrowingCrashLogger_LocalLevel TRACE
 * #import "GrowingCrashLogger.h"
 *
 *
 * ===============
 * IMPORTANT NOTES
 * ===============
 *
 * The C logger changes its behavior depending on the value of the preprocessor
 * define GrowingCrashLogger_CBufferSize.
 *
 * If GrowingCrashLogger_CBufferSize is > 0, the C logger will behave in an async-safe
 * manner, calling write() instead of printf(). Any log messages that exceed the
 * length specified by GrowingCrashLogger_CBufferSize will be truncated.
 *
 * If GrowingCrashLogger_CBufferSize == 0, the C logger will use printf(), and there will
 * be no limit on the log message length.
 *
 * GrowingCrashLogger_CBufferSize can only be set as a preprocessor define, and will
 * default to 1024 if not specified during compilation.
 */


// ============================================================================
#pragma mark - (internal) -
// ============================================================================


#ifndef HDR_GrowingCrashLogger_h
#define HDR_GrowingCrashLogger_h

#ifdef __cplusplus
extern "C" {
#endif


#include <stdbool.h>


#ifdef __OBJC__

#import <CoreFoundation/CoreFoundation.h>

void i_growingcrashlog_logObjC(const char* level,
                     const char* file,
                     int line,
                     const char* function,
                     CFStringRef fmt, ...);

void i_growingcrashlog_logObjCBasic(CFStringRef fmt, ...);

#define i_GrowingCrashLOG_FULL(LEVEL,FILE,LINE,FUNCTION,FMT,...) i_growingcrashlog_logObjC(LEVEL,FILE,LINE,FUNCTION,(__bridge CFStringRef)FMT,##__VA_ARGS__)
#define i_GrowingCrashLOG_BASIC(FMT, ...) i_growingcrashlog_logObjCBasic((__bridge CFStringRef)FMT,##__VA_ARGS__)

#else // __OBJC__

void i_growingcrashlog_logC(const char* level,
                  const char* file,
                  int line,
                  const char* function,
                  const char* fmt, ...);

void i_growingcrashlog_logCBasic(const char* fmt, ...);

#define i_GrowingCrashLOG_FULL i_growingcrashlog_logC
#define i_GrowingCrashLOG_BASIC i_growingcrashlog_logCBasic

#endif // __OBJC__


/* Back up any existing defines by the same name */
#ifdef GROWINGCRASH_NONE
    #define GrowingCrashLOG_BAK_NONE GROWINGCRASH_NONE
    #undef GROWINGCRASH_NONE
#endif
#ifdef ERROR
    #define GrowingCrashLOG_BAK_ERROR ERROR
    #undef ERROR
#endif
#ifdef WARN
    #define GrowingCrashLOG_BAK_WARN WARN
    #undef WARN
#endif
#ifdef INFO
    #define GrowingCrashLOG_BAK_INFO INFO
    #undef INFO
#endif
#ifdef DEBUG
    #define GrowingCrashLOG_BAK_DEBUG DEBUG
    #undef DEBUG
#endif
#ifdef TRACE
    #define GrowingCrashLOG_BAK_TRACE TRACE
    #undef TRACE
#endif


#define GrowingCrashLogger_Level_None   0
#define GrowingCrashLogger_Level_Error 10
#define GrowingCrashLogger_Level_Warn  20
#define GrowingCrashLogger_Level_Info  30
#define GrowingCrashLogger_Level_Debug 40
#define GrowingCrashLogger_Level_Trace 50

#define GROWINGCRASH_NONE  GrowingCrashLogger_Level_None
#define ERROR GrowingCrashLogger_Level_Error
#define WARN  GrowingCrashLogger_Level_Warn
#define INFO  GrowingCrashLogger_Level_Info
#define DEBUG GrowingCrashLogger_Level_Debug
#define TRACE GrowingCrashLogger_Level_Trace


#ifndef GrowingCrashLogger_Level
    #define GrowingCrashLogger_Level GrowingCrashLogger_Level_Error
#endif

#ifndef GrowingCrashLogger_LocalLevel
    #define GrowingCrashLogger_LocalLevel GrowingCrashLogger_Level_None
#endif

#define a_GrowingCrashLOG_FULL(LEVEL, FMT, ...) \
    i_GrowingCrashLOG_FULL(LEVEL, \
                 __FILE__, \
                 __LINE__, \
                 __PRETTY_FUNCTION__, \
                 FMT, \
                 ##__VA_ARGS__)



// ============================================================================
#pragma mark - API -
// ============================================================================

/** Set the filename to log to.
 *
 * @param filename The file to write to (NULL = write to stdout).
 *
 * @param overwrite If true, overwrite the log file.
 */
bool growingcrashlog_setLogFilename(const char* filename, bool overwrite);

/** Clear the log file. */
bool growingcrashlog_clearLogFile(void);

/** Tests if the logger would print at the specified level.
 *
 * @param LEVEL The level to test for. One of:
 *            GrowingCrashLogger_Level_Error,
 *            GrowingCrashLogger_Level_Warn,
 *            GrowingCrashLogger_Level_Info,
 *            GrowingCrashLogger_Level_Debug,
 *            GrowingCrashLogger_Level_Trace,
 *
 * @return TRUE if the logger would print at the specified level.
 */
#define GrowingCrashLOG_PRINTS_AT_LEVEL(LEVEL) \
    (GrowingCrashLogger_Level >= LEVEL || GrowingCrashLogger_LocalLevel >= LEVEL)

/** Log a message regardless of the log settings.
 * Normal version prints out full context. Basic version prints directly.
 *
 * @param FMT The format specifier, followed by its arguments.
 */
#define GrowingCrashLOG_ALWAYS(FMT, ...) a_GrowingCrashLOG_FULL("FORCE", FMT, ##__VA_ARGS__)
#define GrowingCrashLOGBASIC_ALWAYS(FMT, ...) i_GrowingCrashLOG_BASIC(FMT, ##__VA_ARGS__)


/** Log an error.
 * Normal version prints out full context. Basic version prints directly.
 *
 * @param FMT The format specifier, followed by its arguments.
 */
#if GrowingCrashLOG_PRINTS_AT_LEVEL(GrowingCrashLogger_Level_Error)
    #define GrowingCrashLOG_ERROR(FMT, ...) a_GrowingCrashLOG_FULL("ERROR", FMT, ##__VA_ARGS__)
    #define GrowingCrashLOGBASIC_ERROR(FMT, ...) i_GrowingCrashLOG_BASIC(FMT, ##__VA_ARGS__)
#else
    #define GrowingCrashLOG_ERROR(FMT, ...)
    #define GrowingCrashLOGBASIC_ERROR(FMT, ...)
#endif

/** Log a warning.
 * Normal version prints out full context. Basic version prints directly.
 *
 * @param FMT The format specifier, followed by its arguments.
 */
#if GrowingCrashLOG_PRINTS_AT_LEVEL(GrowingCrashLogger_Level_Warn)
    #define GrowingCrashLOG_WARN(FMT, ...)  a_GrowingCrashLOG_FULL("WARN ", FMT, ##__VA_ARGS__)
    #define GrowingCrashLOGBASIC_WARN(FMT, ...) i_GrowingCrashLOG_BASIC(FMT, ##__VA_ARGS__)
#else
    #define GrowingCrashLOG_WARN(FMT, ...)
    #define GrowingCrashLOGBASIC_WARN(FMT, ...)
#endif

/** Log an info message.
 * Normal version prints out full context. Basic version prints directly.
 *
 * @param FMT The format specifier, followed by its arguments.
 */
#if GrowingCrashLOG_PRINTS_AT_LEVEL(GrowingCrashLogger_Level_Info)
    #define GrowingCrashLOG_INFO(FMT, ...)  a_GrowingCrashLOG_FULL("INFO ", FMT, ##__VA_ARGS__)
    #define GrowingCrashLOGBASIC_INFO(FMT, ...) i_GrowingCrashLOG_BASIC(FMT, ##__VA_ARGS__)
#else
    #define GrowingCrashLOG_INFO(FMT, ...)
    #define GrowingCrashLOGBASIC_INFO(FMT, ...)
#endif

/** Log a debug message.
 * Normal version prints out full context. Basic version prints directly.
 *
 * @param FMT The format specifier, followed by its arguments.
 */
#if GrowingCrashLOG_PRINTS_AT_LEVEL(GrowingCrashLogger_Level_Debug)
    #define GrowingCrashLOG_DEBUG(FMT, ...) a_GrowingCrashLOG_FULL("DEBUG", FMT, ##__VA_ARGS__)
    #define GrowingCrashLOGBASIC_DEBUG(FMT, ...) i_GrowingCrashLOG_BASIC(FMT, ##__VA_ARGS__)
#else
    #define GrowingCrashLOG_DEBUG(FMT, ...)
    #define GrowingCrashLOGBASIC_DEBUG(FMT, ...)
#endif

/** Log a trace message.
 * Normal version prints out full context. Basic version prints directly.
 *
 * @param FMT The format specifier, followed by its arguments.
 */
#if GrowingCrashLOG_PRINTS_AT_LEVEL(GrowingCrashLogger_Level_Trace)
    #define GrowingCrashLOG_TRACE(FMT, ...) a_GrowingCrashLOG_FULL("TRACE", FMT, ##__VA_ARGS__)
    #define GrowingCrashLOGBASIC_TRACE(FMT, ...) i_GrowingCrashLOG_BASIC(FMT, ##__VA_ARGS__)
#else
    #define GrowingCrashLOG_TRACE(FMT, ...)
    #define GrowingCrashLOGBASIC_TRACE(FMT, ...)
#endif



// ============================================================================
#pragma mark - (internal) -
// ============================================================================

/* Put everything back to the way we found it. */
#undef ERROR
#ifdef GrowingCrashLOG_BAK_ERROR
    #define ERROR GrowingCrashLOG_BAK_ERROR
    #undef GrowingCrashLOG_BAK_ERROR
#endif
#undef WARNING
#ifdef GrowingCrashLOG_BAK_WARN
    #define WARNING GrowingCrashLOG_BAK_WARN
    #undef GrowingCrashLOG_BAK_WARN
#endif
#undef INFO
#ifdef GrowingCrashLOG_BAK_INFO
    #define INFO GrowingCrashLOG_BAK_INFO
    #undef GrowingCrashLOG_BAK_INFO
#endif
#undef DEBUG
#ifdef GrowingCrashLOG_BAK_DEBUG
    #define DEBUG GrowingCrashLOG_BAK_DEBUG
    #undef GrowingCrashLOG_BAK_DEBUG
#endif
#undef TRACE
#ifdef GrowingCrashLOG_BAK_TRACE
    #define TRACE GrowingCrashLOG_BAK_TRACE
    #undef GrowingCrashLOG_BAK_TRACE
#endif


#ifdef __cplusplus
}
#endif

#endif // HDR_GrowingCrashLogger_h
