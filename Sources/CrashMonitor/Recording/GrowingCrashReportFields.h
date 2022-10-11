//  Created by Karl Stenerud on 2012-10-07.
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
//  GrowingCrashReportFields.h
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


#ifndef HDR_GrowingCrashReportFields_h
#define HDR_GrowingCrashReportFields_h


#pragma mark - Report Types -

#define GrowingCrashReportType_Minimal          "minimal"
#define GrowingCrashReportType_Standard         "standard"
#define GrowingCrashReportType_Custom           "custom"


#pragma mark - Memory Types -

#define GrowingCrashMemType_Block               "objc_block"
#define GrowingCrashMemType_Class               "objc_class"
#define GrowingCrashMemType_NullPointer         "null_pointer"
#define GrowingCrashMemType_Object              "objc_object"
#define GrowingCrashMemType_String              "string"
#define GrowingCrashMemType_Unknown             "unknown"


#pragma mark - Exception Types -

#define GrowingCrashExcType_CPPException        "cpp_exception"
#define GrowingCrashExcType_Deadlock            "deadlock"
#define GrowingCrashExcType_Mach                "mach"
#define GrowingCrashExcType_NSException         "nsexception"
#define GrowingCrashExcType_Signal              "signal"
#define GrowingCrashExcType_User                "user"


#pragma mark - Common -

#define GrowingCrashField_Address               "address"
#define GrowingCrashField_Contents              "contents"
#define GrowingCrashField_Exception             "exception"
#define GrowingCrashField_FirstObject           "first_object"
#define GrowingCrashField_Index                 "index"
#define GrowingCrashField_Ivars                 "ivars"
#define GrowingCrashField_Language              "language"
#define GrowingCrashField_Name                  "name"
#define GrowingCrashField_UserInfo              "userInfo"
#define GrowingCrashField_ReferencedObject      "referenced_object"
#define GrowingCrashField_Type                  "type"
#define GrowingCrashField_UUID                  "uuid"
#define GrowingCrashField_Value                 "value"

#define GrowingCrashField_Error                 "error"
#define GrowingCrashField_JSONData              "json_data"


#pragma mark - Notable Address -

#define GrowingCrashField_Class                 "class"
#define GrowingCrashField_LastDeallocObject     "last_deallocated_obj"


#pragma mark - Backtrace -

#define GrowingCrashField_InstructionAddr       "instruction_addr"
#define GrowingCrashField_LineOfCode            "line_of_code"
#define GrowingCrashField_ObjectAddr            "object_addr"
#define GrowingCrashField_ObjectName            "object_name"
#define GrowingCrashField_SymbolAddr            "symbol_addr"
#define GrowingCrashField_SymbolName            "symbol_name"


#pragma mark - Stack Dump -

#define GrowingCrashField_DumpEnd               "dump_end"
#define GrowingCrashField_DumpStart             "dump_start"
#define GrowingCrashField_GrowDirection         "grow_direction"
#define GrowingCrashField_Overflow              "overflow"
#define GrowingCrashField_StackPtr              "stack_pointer"


#pragma mark - Thread Dump -

#define GrowingCrashField_Backtrace             "backtrace"
#define GrowingCrashField_Basic                 "basic"
#define GrowingCrashField_Crashed               "crashed"
#define GrowingCrashField_CurrentThread         "current_thread"
#define GrowingCrashField_DispatchQueue         "dispatch_queue"
#define GrowingCrashField_NotableAddresses      "notable_addresses"
#define GrowingCrashField_Registers             "registers"
#define GrowingCrashField_Skipped               "skipped"
#define GrowingCrashField_Stack                 "stack"


#pragma mark - Binary Image -

#define GrowingCrashField_CPUSubType            "cpu_subtype"
#define GrowingCrashField_CPUType               "cpu_type"
#define GrowingCrashField_ImageAddress          "image_addr"
#define GrowingCrashField_ImageVmAddress        "image_vmaddr"
#define GrowingCrashField_ImageSize             "image_size"
#define GrowingCrashField_ImageMajorVersion     "major_version"
#define GrowingCrashField_ImageMinorVersion     "minor_version"
#define GrowingCrashField_ImageRevisionVersion  "revision_version"
#define GrowingCrashField_ImageCrashInfoMessage    "crash_info_message"
#define GrowingCrashField_ImageCrashInfoMessage2   "crash_info_message2"


#pragma mark - Memory -

#define GrowingCrashField_Free                  "free"
#define GrowingCrashField_Usable                "usable"


#pragma mark - Error -

#define GrowingCrashField_Backtrace             "backtrace"
#define GrowingCrashField_Code                  "code"
#define GrowingCrashField_CodeName              "code_name"
#define GrowingCrashField_CPPException          "cpp_exception"
#define GrowingCrashField_ExceptionName         "exception_name"
#define GrowingCrashField_Mach                  "mach"
#define GrowingCrashField_NSException           "nsexception"
#define GrowingCrashField_Reason                "reason"
#define GrowingCrashField_Signal                "signal"
#define GrowingCrashField_Subcode               "subcode"
#define GrowingCrashField_UserReported          "user_reported"


#pragma mark - Process State -

#define GrowingCrashField_LastDeallocedNSException "last_dealloced_nsexception"
#define GrowingCrashField_ProcessState             "process"


#pragma mark - App Stats -

#define GrowingCrashField_ActiveTimeSinceCrash  "active_time_since_last_crash"
#define GrowingCrashField_ActiveTimeSinceLaunch "active_time_since_launch"
#define GrowingCrashField_AppActive             "application_active"
#define GrowingCrashField_AppInFG               "application_in_foreground"
#define GrowingCrashField_BGTimeSinceCrash      "background_time_since_last_crash"
#define GrowingCrashField_BGTimeSinceLaunch     "background_time_since_launch"
#define GrowingCrashField_LaunchesSinceCrash    "launches_since_last_crash"
#define GrowingCrashField_SessionsSinceCrash    "sessions_since_last_crash"
#define GrowingCrashField_SessionsSinceLaunch   "sessions_since_launch"


#pragma mark - Report -

#define GrowingCrashField_Crash                 "crash"
#define GrowingCrashField_Debug                 "debug"
#define GrowingCrashField_Diagnosis             "diagnosis"
#define GrowingCrashField_ID                    "id"
#define GrowingCrashField_ProcessName           "process_name"
#define GrowingCrashField_Report                "report"
#define GrowingCrashField_Timestamp             "timestamp"
#define GrowingCrashField_Version               "version"

#pragma mark Minimal
#define GrowingCrashField_CrashedThread         "crashed_thread"

#pragma mark Standard
#define GrowingCrashField_AppStats              "application_stats"
#define GrowingCrashField_BinaryImages          "binary_images"
#define GrowingCrashField_System                "system"
#define GrowingCrashField_Memory                "memory"
#define GrowingCrashField_Threads               "threads"
#define GrowingCrashField_User                  "user"
#define GrowingCrashField_ConsoleLog            "console_log"

#pragma mark Incomplete
#define GrowingCrashField_Incomplete            "incomplete"
#define GrowingCrashField_RecrashReport         "recrash_report"

#pragma mark System
#define GrowingCrashField_AppStartTime          "app_start_time"
#define GrowingCrashField_AppUUID               "app_uuid"
#define GrowingCrashField_BootTime              "boot_time"
#define GrowingCrashField_BundleID              "CFBundleIdentifier"
#define GrowingCrashField_BundleName            "CFBundleName"
#define GrowingCrashField_BundleShortVersion    "CFBundleShortVersionString"
#define GrowingCrashField_BundleVersion         "CFBundleVersion"
#define GrowingCrashField_CPUArch               "cpu_arch"
#define GrowingCrashField_CPUType               "cpu_type"
#define GrowingCrashField_CPUSubType            "cpu_subtype"
#define GrowingCrashField_BinaryCPUType         "binary_cpu_type"
#define GrowingCrashField_BinaryCPUSubType      "binary_cpu_subtype"
#define GrowingCrashField_DeviceAppHash         "device_app_hash"
#define GrowingCrashField_Executable            "CFBundleExecutable"
#define GrowingCrashField_ExecutablePath        "CFBundleExecutablePath"
#define GrowingCrashField_Jailbroken            "jailbroken"
#define GrowingCrashField_KernelVersion         "kernel_version"
#define GrowingCrashField_Machine               "machine"
#define GrowingCrashField_Model                 "model"
#define GrowingCrashField_OSVersion             "os_version"
#define GrowingCrashField_ParentProcessID       "parent_process_id"
#define GrowingCrashField_ProcessID             "process_id"
#define GrowingCrashField_ProcessName           "process_name"
#define GrowingCrashField_Size                  "size"
#define GrowingCrashField_Storage               "storage"
#define GrowingCrashField_SystemName            "system_name"
#define GrowingCrashField_SystemVersion         "system_version"
#define GrowingCrashField_TimeZone              "time_zone"
#define GrowingCrashField_BuildType             "build_type"

#endif
