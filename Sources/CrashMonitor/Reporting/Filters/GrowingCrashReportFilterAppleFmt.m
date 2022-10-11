//  Created by Karl Stenerud on 2012-02-24.
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
//  GrowingCrashReportFilterAppleFmt.m
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


#import "GrowingCrashReportFilterAppleFmt.h"


#import <inttypes.h>
#import <mach/machine.h>

#import "GrowingCrashReportFields.h"
#import "GrowingCrashJSONCodecObjC.h"
#import "GrowingCrashMonitor_System.h"

#define CPU_SUBTYPE_ARM64E              ((cpu_subtype_t) 2)

#if defined(__LP64__)
    #define FMT_LONG_DIGITS "16"
    #define FMT_RJ_SPACES "18"
#else
    #define FMT_LONG_DIGITS "8"
    #define FMT_RJ_SPACES "10"
#endif

#define FMT_PTR_SHORT        @"0x%" PRIxPTR
#define FMT_PTR_LONG         @"0x%0" FMT_LONG_DIGITS PRIxPTR
#define FMT_PTR_RJ           @"%#" FMT_RJ_SPACES PRIxPTR
#define FMT_OFFSET           @"%" PRIuPTR
#define FMT_TRACE_PREAMBLE       @"%-4d%-31s " FMT_PTR_LONG
#define FMT_TRACE_UNSYMBOLICATED FMT_PTR_SHORT @" + " FMT_OFFSET
#define FMT_TRACE_SYMBOLICATED   @"%@ + " FMT_OFFSET

#define kAppleRedactedText @"<redacted>"

#define kExpectedMajorVersion 3


@interface GrowingCrashReportFilterAppleFmt ()

@property(nonatomic,readwrite,assign) GrowingCrashAppleReportStyle reportStyle;

/** Convert a crash report to Apple format.
 *
 * @param JSONReport The crash report.
 *
 * @return The converted crash report.
 */
- (NSString*) toAppleFormat:(NSDictionary*) JSONReport;

/** Determine the major CPU type.
 *
 * @param CPUArch The CPU architecture name.
 *
 * @return the major CPU type.
 */
- (NSString*) CPUType:(NSString*) CPUArch;

/** Determine the CPU architecture based on major/minor CPU architecture codes.
 *
 * @param majorCode The major part of the code.
 *
 * @param minorCode The minor part of the code.
 *
 * @return The CPU architecture.
 */
- (NSString*) CPUArchForMajor:(cpu_type_t) majorCode minor:(cpu_subtype_t) minorCode;

/** Take a UUID string and strip out all the dashes.
 *
 * @param uuid the UUID.
 *
 * @return the UUID in compact form.
 */
- (NSString*) toCompactUUID:(NSString*) uuid;

@end


@implementation GrowingCrashReportFilterAppleFmt

@synthesize reportStyle = _reportStyle;

/** Date formatter for Apple date format in crash reports. */
static NSDateFormatter* g_dateFormatter;

/** Date formatter for RFC3339 date format. */
static NSDateFormatter* g_rfc3339DateFormatter;

/** Printing order for registers. */
static NSDictionary* g_registerOrders;

+ (void) initialize
{
    g_dateFormatter = [[NSDateFormatter alloc] init];
    [g_dateFormatter setLocale:[NSLocale localeWithLocaleIdentifier:@"en_US_POSIX"]];
    [g_dateFormatter setDateFormat:@"yyyy-MM-dd HH:mm:ss.SSS ZZZ"];

    g_rfc3339DateFormatter = [[NSDateFormatter alloc] init];
    [g_rfc3339DateFormatter setLocale:[NSLocale localeWithLocaleIdentifier:@"en_US_POSIX"]];
    [g_rfc3339DateFormatter setDateFormat:@"yyyy'-'MM'-'dd'T'HH':'mm':'ss'.'SSSSSS'Z'"];
    [g_rfc3339DateFormatter setTimeZone:[NSTimeZone timeZoneForSecondsFromGMT:0]];

    NSArray* armOrder = [NSArray arrayWithObjects:
                         @"r0", @"r1", @"r2", @"r3", @"r4", @"r5", @"r6", @"r7",
                         @"r8", @"r9", @"r10", @"r11", @"ip",
                         @"sp", @"lr", @"pc", @"cpsr",
                         nil];

    NSArray* x86Order = [NSArray arrayWithObjects:
                         @"eax", @"ebx", @"ecx", @"edx",
                         @"edi", @"esi",
                         @"ebp", @"esp", @"ss",
                         @"eflags", @"eip",
                         @"cs", @"ds", @"es", @"fs", @"gs",
                         nil];

    NSArray* x86_64Order = [NSArray arrayWithObjects:
                            @"rax", @"rbx", @"rcx", @"rdx",
                            @"rdi", @"rsi",
                            @"rbp", @"rsp",
                            @"r8", @"r9", @"r10", @"r11", @"r12", @"r13",
                            @"r14", @"r15",
                            @"rip", @"rflags",
                            @"cs", @"fs", @"gs",
                            nil];

    g_registerOrders = [[NSDictionary alloc] initWithObjectsAndKeys:
                        armOrder, @"arm",
                        armOrder, @"armv6",
                        armOrder, @"armv7",
                        armOrder, @"armv7f",
                        armOrder, @"armv7k",
                        armOrder, @"armv7s",
                        x86Order, @"x86",
                        x86Order, @"i386",
                        x86Order, @"i486",
                        x86Order, @"i686",
                        x86_64Order, @"x86_64",
                        nil];
}

+ (GrowingCrashReportFilterAppleFmt*) filterWithReportStyle:(GrowingCrashAppleReportStyle) reportStyle
{
    return [[self alloc] initWithReportStyle:reportStyle];
}

- (id) initWithReportStyle:(GrowingCrashAppleReportStyle) reportStyle
{
    if((self = [super init]))
    {
        self.reportStyle = reportStyle;
    }
    return self;
}

- (int) majorVersion:(NSDictionary*) report
{
    NSDictionary* info = [self infoReport:report];
    NSString* version = [info objectForKey:@GrowingCrashField_Version];
    if ([version isKindOfClass:[NSDictionary class]])
    {
        NSDictionary *oldVersion = (NSDictionary *)version;
        version = oldVersion[@"major"];
    }

    if([version respondsToSelector:@selector(intValue)])
    {
        return version.intValue;
    }
    return 0;
}

- (void) filterReports:(NSArray*) reports
          onCompletion:(GrowingCrashReportFilterCompletion) onCompletion
{
    NSMutableArray* filteredReports = [NSMutableArray arrayWithCapacity:[reports count]];
    for(NSDictionary* report in reports)
    {
        if([self majorVersion:report] == kExpectedMajorVersion)
        {
            id appleReport = [self toAppleFormat:report];
            if(appleReport != nil)
            {
                [filteredReports addObject:appleReport];
            }
        }
    }

    growingcrash_callCompletion(onCompletion, filteredReports, YES, nil);
}

- (NSString*) CPUType:(NSString*) CPUArch
{
    if([CPUArch rangeOfString:@"arm64"].location == 0)
    {
        return @"ARM-64";
    }
    if([CPUArch rangeOfString:@"arm"].location == 0)
    {
        return @"ARM";
    }
    if([CPUArch isEqualToString:@"x86"])
    {
        return @"X86";
    }
    if([CPUArch isEqualToString:@"x86_64"])
    {
        return @"X86_64";
    }
    return @"Unknown";
}

- (NSString*) CPUArchForMajor:(cpu_type_t) majorCode minor:(cpu_subtype_t) minorCode
{
    switch(majorCode)
    {
        case CPU_TYPE_ARM:
        {
            switch (minorCode)
            {
                case CPU_SUBTYPE_ARM_V6:
                    return @"armv6";
                case CPU_SUBTYPE_ARM_V7:
                    return @"armv7";
                case CPU_SUBTYPE_ARM_V7F:
                    return @"armv7f";
                case CPU_SUBTYPE_ARM_V7K:
                    return @"armv7k";
#ifdef CPU_SUBTYPE_ARM_V7S
                case CPU_SUBTYPE_ARM_V7S:
                    return @"armv7s";
#endif
            }
            return @"arm";
        }
#ifdef CPU_TYPE_ARM64
        case CPU_TYPE_ARM64:
        {
            switch (minorCode)
            {
#ifdef CPU_SUBTYPE_ARM64E
                case CPU_SUBTYPE_ARM64E:
                    return @"arm64e";
#endif
            }
            return @"arm64";
        }
#endif
        case CPU_TYPE_X86:
            return @"i386";
        case CPU_TYPE_X86_64:
            return @"x86_64";
    }
    return [NSString stringWithFormat:@"unknown(%d,%d)", majorCode, minorCode];
}

/** Convert a backtrace to a string.
 *
 * @param backtrace The backtrace to convert.
 *
 * @param reportStyle The style of report being generated.
 *
 * @param mainExecutableName Name of the app executable.
 *
 * @return The converted string.
 */
- (NSString*) backtraceString:(NSDictionary*) backtrace
                  reportStyle:(GrowingCrashAppleReportStyle) reportStyle
           mainExecutableName:(NSString*) mainExecutableName
{
    NSMutableString* str = [NSMutableString string];

    int traceNum = 0;
    for(NSDictionary* trace in [backtrace objectForKey:@GrowingCrashField_Contents])
    {
        uintptr_t pc = (uintptr_t)[[trace objectForKey:@GrowingCrashField_InstructionAddr] longLongValue];
        uintptr_t objAddr = (uintptr_t)[[trace objectForKey:@GrowingCrashField_ObjectAddr] longLongValue];
        NSString* objName = [[trace objectForKey:@GrowingCrashField_ObjectName] lastPathComponent];
        uintptr_t symAddr = (uintptr_t)[[trace objectForKey:@GrowingCrashField_SymbolAddr] longLongValue];
        NSString* symName = [trace objectForKey:@GrowingCrashField_SymbolName];
        bool isMainExecutable = mainExecutableName && [objName isEqualToString:mainExecutableName];
        GrowingCrashAppleReportStyle thisLineStyle = reportStyle;
        if(thisLineStyle == GrowingCrashAppleReportStylePartiallySymbolicated)
        {
            thisLineStyle = isMainExecutable ? GrowingCrashAppleReportStyleUnsymbolicated : GrowingCrashAppleReportStyleSymbolicated;
        }

        NSString* preamble = [NSString stringWithFormat:FMT_TRACE_PREAMBLE, traceNum, [objName UTF8String], pc];
        NSString* unsymbolicated = [NSString stringWithFormat:FMT_TRACE_UNSYMBOLICATED, objAddr, pc - objAddr];
        NSString* symbolicated = @"(null)";
        if(thisLineStyle != GrowingCrashAppleReportStyleUnsymbolicated && [symName isKindOfClass:[NSString class]])
        {
            symbolicated = [NSString stringWithFormat:FMT_TRACE_SYMBOLICATED, symName, pc - symAddr];
        }
        else
        {
            thisLineStyle = GrowingCrashAppleReportStyleUnsymbolicated;
        }


        // Apple has started replacing symbols for any function/method
        // beginning with an underscore with "<redacted>" in iOS 6.
        // No, I can't think of any valid reason to do this, either.
        if(thisLineStyle == GrowingCrashAppleReportStyleSymbolicated &&
           [symName isEqualToString:kAppleRedactedText])
        {
            thisLineStyle = GrowingCrashAppleReportStyleUnsymbolicated;
        }

        switch (thisLineStyle)
        {
            case GrowingCrashAppleReportStyleSymbolicatedSideBySide:
                [str appendFormat:@"%@ %@ (%@)\n", preamble, unsymbolicated, symbolicated];
                break;
            case GrowingCrashAppleReportStyleSymbolicated:
                [str appendFormat:@"%@ %@\n", preamble, symbolicated];
                break;
            case GrowingCrashAppleReportStylePartiallySymbolicated: // Should not happen
            case GrowingCrashAppleReportStyleUnsymbolicated:
                [str appendFormat:@"%@ %@\n", preamble, unsymbolicated];
                break;
        }
        traceNum++;
    }

    return str;
}

- (NSString*) toCompactUUID:(NSString*) uuid
{
    return [[uuid lowercaseString] stringByReplacingOccurrencesOfString:@"-" withString:@""];
}

- (NSString*) stringFromDate:(NSDate*) date
{
    if(![date isKindOfClass:[NSDate class]])
    {
        return nil;
    }
    return [g_dateFormatter stringFromDate:date];
}

- (NSDictionary*) recrashReport:(NSDictionary*) report
{
    return [report objectForKey:@GrowingCrashField_RecrashReport];
}

- (NSDictionary*) systemReport:(NSDictionary*) report
{
    return [report objectForKey:@GrowingCrashField_System];
}

- (NSDictionary*) infoReport:(NSDictionary*) report
{
    return [report objectForKey:@GrowingCrashField_Report];
}

- (NSDictionary*) processReport:(NSDictionary*) report
{
    return [report objectForKey:@GrowingCrashField_ProcessState];
}

- (NSDictionary*) crashReport:(NSDictionary*) report
{
    return [report objectForKey:@GrowingCrashField_Crash];
}

- (NSArray*) binaryImagesReport:(NSDictionary*) report
{
    return [report objectForKey:@GrowingCrashField_BinaryImages];
}

- (NSDictionary*) crashedThread:(NSDictionary*) report
{
    NSDictionary* crash = [self crashReport:report];
    NSArray* threads = [crash objectForKey:@GrowingCrashField_Threads];
    for(NSDictionary* thread in threads)
    {
        BOOL crashed = [[thread objectForKey:@GrowingCrashField_Crashed] boolValue];
        if(crashed)
        {
            return thread;
        }
    }

    return [crash objectForKey:@GrowingCrashField_CrashedThread];
}

- (NSString*) mainExecutableNameForReport:(NSDictionary*) report
{
    NSDictionary* info = [self infoReport:report];
    return [info objectForKey:@GrowingCrashField_ProcessName];
}

- (NSString*) cpuArchForReport:(NSDictionary*) report
{
    NSDictionary* system = [self systemReport:report];
    cpu_type_t cpuType = [[system objectForKey:@GrowingCrashField_BinaryCPUType] intValue];
    cpu_subtype_t cpuSubType = [[system objectForKey:@GrowingCrashField_BinaryCPUSubType] intValue];
    return [self CPUArchForMajor:cpuType minor:cpuSubType];
}

- (NSString*) headerStringForReport:(NSDictionary*) report
{
    NSDictionary* system = [self systemReport:report];
    NSDictionary* reportInfo = [self infoReport:report];
    NSString *reportID = [reportInfo objectForKey:@GrowingCrashField_ID];
    NSDate* crashTime = [g_rfc3339DateFormatter dateFromString:[reportInfo objectForKey:@GrowingCrashField_Timestamp]];

    return [self headerStringForSystemInfo:system reportID:reportID crashTime:crashTime];
}

- (NSString*)headerStringForSystemInfo:(NSDictionary*)system reportID:(NSString*)reportID crashTime:(NSDate*)crashTime
{
    NSMutableString* str = [NSMutableString string];
    NSString* executablePath = [system objectForKey:@GrowingCrashField_ExecutablePath];
    NSString* cpuArch = [system objectForKey:@GrowingCrashField_CPUArch];
    NSString* cpuArchType = [self CPUType:cpuArch];

    [str appendFormat:@"Incident Identifier: %@\n", reportID];
    [str appendFormat:@"CrashReporter Key:   %@\n", [system objectForKey:@GrowingCrashField_DeviceAppHash]];
    [str appendFormat:@"Hardware Model:      %@\n", [system objectForKey:@GrowingCrashField_Machine]];
    [str appendFormat:@"Process:         %@ [%@]\n",
     [system objectForKey:@GrowingCrashField_ProcessName],
     [system objectForKey:@GrowingCrashField_ProcessID]];
    [str appendFormat:@"Path:            %@\n", executablePath];
    [str appendFormat:@"Identifier:      %@\n", [system objectForKey:@GrowingCrashField_BundleID]];
    [str appendFormat:@"Version:         %@ (%@)\n",
     [system objectForKey:@GrowingCrashField_BundleVersion],
     [system objectForKey:@GrowingCrashField_BundleShortVersion]];
    [str appendFormat:@"Code Type:       %@\n", cpuArchType];
    [str appendFormat:@"Parent Process:  ? [%@]\n",
     [system objectForKey:@GrowingCrashField_ParentProcessID]];
    [str appendFormat:@"\n"];
    [str appendFormat:@"Date/Time:       %@\n", [self stringFromDate:crashTime]];
    [str appendFormat:@"OS Version:      %@ %@ (%@)\n",
     [system objectForKey:@GrowingCrashField_SystemName],
     [system objectForKey:@GrowingCrashField_SystemVersion],
     [system objectForKey:@GrowingCrashField_OSVersion]];
    [str appendFormat:@"Report Version:  104\n"];

    return str;
}

- (NSString*) binaryImagesStringForReport:(NSDictionary*) report
{
    NSMutableString* str = [NSMutableString string];

    NSArray* binaryImages = [self binaryImagesReport:report];
    NSDictionary* system = [self systemReport:report];
    NSString* executablePath = [system objectForKey:@GrowingCrashField_ExecutablePath];

    [str appendString:@"\nBinary Images:\n"];
    if(binaryImages)
    {
        NSMutableArray* images = [NSMutableArray arrayWithArray:binaryImages];
        [images sortUsingComparator:^NSComparisonResult(id obj1, id obj2)
         {
             NSNumber* num1 = [(NSDictionary*)obj1 objectForKey:@GrowingCrashField_ImageAddress];
             NSNumber* num2 = [(NSDictionary*)obj2 objectForKey:@GrowingCrashField_ImageAddress];
             if(num1 == nil || num2 == nil)
             {
                 return NSOrderedSame;
             }
             return [num1 compare:num2];
         }];
        for(NSDictionary* image in images)
        {
            cpu_type_t cpuType = [[image objectForKey:@GrowingCrashField_CPUType] intValue];
            cpu_subtype_t cpuSubtype = [[image objectForKey:@GrowingCrashField_CPUSubType] intValue];
            uintptr_t imageAddr = (uintptr_t)[[image objectForKey:@GrowingCrashField_ImageAddress] longLongValue];
            uintptr_t imageSize = (uintptr_t)[[image objectForKey:@GrowingCrashField_ImageSize] longLongValue];
            NSString* path = [image objectForKey:@GrowingCrashField_Name];
            NSString* name = [path lastPathComponent];
            NSString* uuid = [self toCompactUUID:[image objectForKey:@GrowingCrashField_UUID]];
            NSString* isBaseImage = (path && [executablePath isEqualToString:path]) ? @"+" : @" ";

            [str appendFormat:FMT_PTR_RJ @" - " FMT_PTR_RJ @" %@%@ %@  <%@> %@\n",
             imageAddr,
             imageAddr + imageSize - 1,
             isBaseImage,
             name,
             [self CPUArchForMajor:cpuType minor:cpuSubtype],
             uuid,
             path];
        }
    }

    return str;
}

- (NSString*) crashedThreadCPUStateStringForReport:(NSDictionary*) report
                                           cpuArch:(NSString*) cpuArch
{
    NSDictionary* thread = [self crashedThread:report];
    if(thread == nil)
    {
        return @"";
    }
    int threadIndex = [[thread objectForKey:@GrowingCrashField_Index] intValue];

    NSString* cpuArchType = [self CPUType:cpuArch];

    NSMutableString* str = [NSMutableString string];

    [str appendFormat:@"\nThread %d crashed with %@ Thread State:\n",
     threadIndex, cpuArchType];

    NSDictionary* registers = [(NSDictionary*)[thread objectForKey:@GrowingCrashField_Registers] objectForKey:@GrowingCrashField_Basic];
    NSArray* regOrder = [g_registerOrders objectForKey:cpuArch];
    if(regOrder == nil)
    {
        regOrder = [[registers allKeys] sortedArrayUsingSelector:@selector(compare:)];
    }
    NSUInteger numRegisters = [regOrder count];
    NSUInteger i = 0;
    while(i < numRegisters)
    {
        NSUInteger nextBreak = i + 4;
        if(nextBreak > numRegisters)
        {
            nextBreak = numRegisters;
        }
        for(;i < nextBreak; i++)
        {
            NSString* regName = [regOrder objectAtIndex:i];
            uintptr_t addr = (uintptr_t)[[registers objectForKey:regName] longLongValue];
            [str appendFormat:@"%6s: " FMT_PTR_LONG @" ",
             [regName cStringUsingEncoding:NSUTF8StringEncoding],
             addr];
        }
        [str appendString:@"\n"];
    }

    return str;
}

- (NSString*) extraInfoStringForReport:(NSDictionary*) report
                    mainExecutableName:(NSString*) mainExecutableName
{
    NSMutableString* str = [NSMutableString string];

    [str appendString:@"\nExtra Information:\n"];

    NSDictionary* system = [self systemReport:report];
    NSDictionary* crash = [self crashReport:report];
    NSDictionary* error = [crash objectForKey:@GrowingCrashField_Error];
    NSDictionary* nsexception = [error objectForKey:@GrowingCrashField_NSException];
    NSDictionary* referencedObject = [nsexception objectForKey:@GrowingCrashField_ReferencedObject];
    if(referencedObject != nil)
    {
        [str appendFormat:@"Object referenced by NSException:\n%@\n", [self JSONForObject:referencedObject]];
    }

    NSDictionary* crashedThread = [self crashedThread:report];
    if(crashedThread != nil)
    {
        NSDictionary* stack = [crashedThread objectForKey:@GrowingCrashField_Stack];
        if(stack != nil)
        {
            [str appendFormat:@"\nStack Dump (" FMT_PTR_LONG "-" FMT_PTR_LONG "):\n\n%@\n",
             (uintptr_t)[[stack objectForKey:@GrowingCrashField_DumpStart] unsignedLongLongValue],
             (uintptr_t)[[stack objectForKey:@GrowingCrashField_DumpEnd] unsignedLongLongValue],
             [stack objectForKey:@GrowingCrashField_Contents]];
        }

        NSDictionary* notableAddresses = [crashedThread objectForKey:@GrowingCrashField_NotableAddresses];
        if(notableAddresses != nil)
        {
            [str appendFormat:@"\nNotable Addresses:\n%@\n", [self JSONForObject:notableAddresses]];
        }
    }

    NSDictionary* lastException = [[self processReport:report] objectForKey:@GrowingCrashField_LastDeallocedNSException];
    if(lastException != nil)
    {
        uintptr_t address = (uintptr_t)[[lastException objectForKey:@GrowingCrashField_Address] unsignedLongLongValue];
        NSString* name = [lastException objectForKey:@GrowingCrashField_Name];
        NSString* reason = [lastException objectForKey:@GrowingCrashField_Reason];
        referencedObject = [lastException objectForKey:@GrowingCrashField_ReferencedObject];
        [str appendFormat:@"\nLast deallocated NSException (" FMT_PTR_LONG "): %@: %@\n",
         address, name, reason];
        if(referencedObject != nil)
        {
            [str appendFormat:@"Referenced object:\n%@\n", [self JSONForObject:referencedObject]];
        }
        [str appendString:
         [self backtraceString:[lastException objectForKey:@GrowingCrashField_Backtrace]
                   reportStyle:self.reportStyle
            mainExecutableName:mainExecutableName]];
    }

    NSDictionary* appStats = [system objectForKey:@GrowingCrashField_AppStats];
    if(appStats != nil)
    {
        [str appendFormat:@"\nApplication Stats:\n%@\n", [self JSONForObject:appStats]];
    }

    NSDictionary* crashReport = [report objectForKey:@GrowingCrashField_Crash];
    NSString* diagnosis = [crashReport objectForKey:@GrowingCrashField_Diagnosis];
    if(diagnosis != nil)
    {
        [str appendFormat:@"\nCrashDoctor Diagnosis: %@\n", diagnosis];
    }

    return str;
}

- (NSString*) JSONForObject:(id) object
{
    NSError* error = nil;
    NSData* encoded = [GrowingCrashJSONCodec encode:object
                                  options:GrowingCrashJSONEncodeOptionPretty |
                       GrowingCrashJSONEncodeOptionSorted
                                    error:&error];
    if(error != nil)
    {
        return [NSString stringWithFormat:@"Error encoding JSON: %@", error];
    }
    else
    {
        return [[NSString alloc] initWithData:encoded encoding:NSUTF8StringEncoding];
    }
}

- (BOOL) isZombieNSException:(NSDictionary*) report
{
    NSDictionary* crash = [self crashReport:report];
    NSDictionary* error = [crash objectForKey:@GrowingCrashField_Error];
    NSDictionary* mach = [error objectForKey:@GrowingCrashField_Mach];
    NSString* machExcName = [mach objectForKey:@GrowingCrashField_ExceptionName];
    NSString* machCodeName = [mach objectForKey:@GrowingCrashField_CodeName];
    if(![machExcName isEqualToString:@"EXC_BAD_ACCESS"] ||
       ![machCodeName isEqualToString:@"KERN_INVALID_ADDRESS"])
    {
        return NO;
    }

    NSDictionary* lastException = [[self processReport:report] objectForKey:@GrowingCrashField_LastDeallocedNSException];
    if(lastException == nil)
    {
        return NO;
    }
    NSNumber* lastExceptionAddress = [lastException objectForKey:@GrowingCrashField_Address];

    NSDictionary* thread = [self crashedThread:report];
    NSDictionary* registers = [(NSDictionary*)[thread objectForKey:@GrowingCrashField_Registers] objectForKey:@GrowingCrashField_Basic];

    for(NSString* reg in registers)
    {
        NSNumber* address = [registers objectForKey:reg];
        if(lastExceptionAddress && [address isEqualToNumber:lastExceptionAddress])
        {
            return YES;
        }
    }

    return NO;
}

- (NSString*) errorInfoStringForReport:(NSDictionary*) report
{
    NSMutableString* str = [NSMutableString string];

    NSDictionary* thread = [self crashedThread:report];
    NSDictionary* crash = [self crashReport:report];
    NSDictionary* error = [crash objectForKey:@GrowingCrashField_Error];
    NSDictionary* type = [error objectForKey:@GrowingCrashField_Type];

    NSDictionary* nsexception = [error objectForKey:@GrowingCrashField_NSException];
    NSDictionary* cppexception = [error objectForKey:@GrowingCrashField_CPPException];
    NSDictionary* lastException = [[self processReport:report] objectForKey:@GrowingCrashField_LastDeallocedNSException];
    NSDictionary* userException = [error objectForKey:@GrowingCrashField_UserReported];
    NSDictionary* mach = [error objectForKey:@GrowingCrashField_Mach];
    NSDictionary* signal = [error objectForKey:@GrowingCrashField_Signal];

    NSString* machExcName = [mach objectForKey:@GrowingCrashField_ExceptionName];
    if(machExcName == nil)
    {
        machExcName = @"0";
    }
    NSString* signalName = [signal objectForKey:@GrowingCrashField_Name];
    if(signalName == nil)
    {
        signalName = [[signal objectForKey:@GrowingCrashField_Signal] stringValue];
    }
    NSString* machCodeName = [mach objectForKey:@GrowingCrashField_CodeName];
    if(machCodeName == nil)
    {
        machCodeName = @"0x00000000";
    }

    [str appendFormat:@"\n"];
    [str appendFormat:@"Exception Type:  %@ (%@)\n", machExcName, signalName];
    [str appendFormat:@"Exception Codes: %@ at " FMT_PTR_LONG @"\n",
     machCodeName,
     (uintptr_t)[[error objectForKey:@GrowingCrashField_Address] longLongValue]];

    [str appendFormat:@"Crashed Thread:  %d\n",
     [[thread objectForKey:@GrowingCrashField_Index] intValue]];

    if(nsexception != nil)
    {
        [str appendString:[self stringWithUncaughtExceptionName:[nsexception objectForKey:@GrowingCrashField_Name]
                                                         reason:[error objectForKey:@GrowingCrashField_Reason]]];
    }
    else if([self isZombieNSException:report])
    {
        [str appendString:[self stringWithUncaughtExceptionName:[lastException objectForKey:@GrowingCrashField_Name]
                                                         reason:[lastException objectForKey:@GrowingCrashField_Reason]]];
        [str appendString:@"NOTE: This exception has been deallocated! Stack trace is crash from attempting to access this zombie exception.\n"];
    }
    else if(userException != nil)
    {
        [str appendString:[self stringWithUncaughtExceptionName:[userException objectForKey:@GrowingCrashField_Name]
                                                         reason:[error objectForKey:@GrowingCrashField_Reason]]];
        NSString* trace = [self userExceptionTrace:userException];
        if(trace.length > 0)
        {
            [str appendFormat:@"\n%@\n", trace];
        }
    }
    else if([type isEqual:@GrowingCrashExcType_CPPException])
    {
        [str appendString:[self stringWithUncaughtExceptionName:[cppexception objectForKey:@GrowingCrashField_Name]
                                                         reason:[error objectForKey:@GrowingCrashField_Reason]]];
    }

    NSString* crashType = [error objectForKey:@GrowingCrashField_Type];
    if(crashType && [@GrowingCrashExcType_Deadlock isEqualToString:crashType])
    {
        [str appendFormat:@"\nApplication main thread deadlocked\n"];
    }

    return str;
}

- (NSString*) stringWithUncaughtExceptionName:(NSString*) name reason:(NSString*) reason
{
    return [NSString stringWithFormat:
            @"\nApplication Specific Information:\n"
            @"*** Terminating app due to uncaught exception '%@', reason: '%@'\n",
            name, reason];
}

- (NSString*) userExceptionTrace:(NSDictionary*)userException
{
    NSMutableString* str = [NSMutableString string];
    NSString* line = [userException objectForKey:@GrowingCrashField_LineOfCode];
    if(line != nil)
    {
        [str appendFormat:@"Line: %@\n", line];
    }
    NSArray* backtrace = [userException objectForKey:@GrowingCrashField_Backtrace];
    for(NSString* entry in backtrace)
    {
        [str appendFormat:@"%@\n", entry];
    }

    if(str.length > 0)
    {
        return [@"Custom Backtrace:\n" stringByAppendingString:str];
    }
    return @"";
}

- (NSString*) threadStringForThread:(NSDictionary*) thread
                 mainExecutableName:(NSString*) mainExecutableName
{
    NSMutableString* str = [NSMutableString string];

    [str appendFormat:@"\n"];
    BOOL crashed = [[thread objectForKey:@GrowingCrashField_Crashed] boolValue];
    int index = [[thread objectForKey:@GrowingCrashField_Index] intValue];
    NSString* name = [thread objectForKey:@GrowingCrashField_Name];
    NSString* queueName = [thread objectForKey:@GrowingCrashField_DispatchQueue];

    if(name != nil)
    {
        [str appendFormat:@"Thread %d name:  %@\n", index, name];
    }
    else if(queueName != nil)
    {
        [str appendFormat:@"Thread %d name:  Dispatch queue: %@\n", index, queueName];
    }

    if(crashed)
    {
        [str appendFormat:@"Thread %d Crashed:\n", index];
    }
    else
    {
        [str appendFormat:@"Thread %d:\n", index];
    }

    [str appendString:
     [self backtraceString:[thread objectForKey:@GrowingCrashField_Backtrace]
               reportStyle:self.reportStyle
        mainExecutableName:mainExecutableName]];

    return str;
}

- (NSString*) threadListStringForReport:(NSDictionary*) report
                     mainExecutableName:(NSString*) mainExecutableName
{
    NSMutableString* str = [NSMutableString string];

    NSDictionary* crash = [self crashReport:report];
    NSArray* threads = [crash objectForKey:@GrowingCrashField_Threads];

    for(NSDictionary* thread in threads)
    {
        [str appendString:[self threadStringForThread:thread mainExecutableName:mainExecutableName]];
    }

    return str;
}

- (NSString*) crashReportString:(NSDictionary*) report
{
    NSMutableString* str = [NSMutableString string];
    NSString* executableName = [self mainExecutableNameForReport:report];

    [str appendString:[self headerStringForReport:report]];
    [str appendString:[self errorInfoStringForReport:report]];
    [str appendString:[self threadListStringForReport:report mainExecutableName:executableName]];
    [str appendString:[self crashedThreadCPUStateStringForReport:report cpuArch:[self cpuArchForReport:report]]];
    [str appendString:[self binaryImagesStringForReport:report]];
    [str appendString:[self extraInfoStringForReport:report mainExecutableName:executableName]];

    return str;
}

- (NSString*) recrashReportString:(NSDictionary*) report
{
    NSMutableString* str = [NSMutableString string];
    
    NSDictionary* recrashReport = [self recrashReport:report];
    NSDictionary* system = [self systemReport:recrashReport];
    NSString* executablePath = [system objectForKey:@GrowingCrashField_ExecutablePath];
    NSString* executableName = [executablePath lastPathComponent];
    NSDictionary* crash = [self crashReport:report];
    NSDictionary* thread = [crash objectForKey:@GrowingCrashField_CrashedThread];

    [str appendString:@"\nHandler crashed while reporting:\n"];
    [str appendString:[self errorInfoStringForReport:report]];
    [str appendString:[self threadStringForThread:thread mainExecutableName:executableName]];
    [str appendString:[self crashedThreadCPUStateStringForReport:report
                                                         cpuArch:[self cpuArchForReport:recrashReport]]];
    NSString* diagnosis = [crash objectForKey:@GrowingCrashField_Diagnosis];
    if(diagnosis != nil)
    {
        [str appendFormat:@"\nRecrash Diagnosis: %@", diagnosis];
    }

    return str;
}


- (NSString*) toAppleFormat:(NSDictionary*) report
{
    NSMutableString* str = [NSMutableString string];
    
    NSDictionary* recrashReport = report[@GrowingCrashField_RecrashReport];
    if (recrashReport) {
        [str appendString:[self crashReportString:recrashReport]];
        [str appendString:[self recrashReportString:report]];
    } else {
        [str appendString:[self crashReportString:report]];
    }

    return str;
}

@end
