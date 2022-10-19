// swift-tools-version:5.3
// The swift-tools-version declares the minimum version of Swift required to build this package.

//
//  Package.swift
//  GrowingAnalytics
//
//  Created by YoloMao on 2022/10/17.
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

import PackageDescription

let package = Package(
    name: "GrowingAPM",
    platforms: [.iOS(.v9)],
    products: [
        .library(
            name: "GrowingAPM",
            targets: ["GrowingAPM_Wrapper"]
        ),
    ],
    dependencies: [
        .package(
            name: "GrowingUtils",
            url: "https://github.com/growingio/growingio-sdk-ios-utilities.git",
            .exact("0.0.1")
        ),
    ],
    targets: [
        
        // MARK: - GrowingAPM Wrapper
        
        .target(
            name: "GrowingAPM_Wrapper",
            dependencies: [
                "GrowingAPMCrashMonitor",
                "GrowingAPMUIMonitor",
                "GrowingAPMLaunchMonitor",
                .product(name: "GrowingUtilsAutotrackerCore", package: "GrowingUtils"),
            ],
            path: "Core",
            publicHeadersPath: ".",
            cxxSettings: [
                .define("GCC_ENABLE_CPP_EXCEPTIONS", to: "YES"),
            ],
            linkerSettings: [
                .linkedLibrary("c++"),
                .linkedLibrary("z"),
            ]
        ),
        
        // MARK: - GrowingAPM Modules
        
        .binaryTarget(
            name: "GrowingAPMCrashMonitor",
            path: "CrashMonitor/GrowingAPMCrashMonitor.xcframework"
        ),
        .binaryTarget(
            name: "GrowingAPMUIMonitor",
            path: "UIMonitor/GrowingAPMUIMonitor.xcframework"
        ),
        .binaryTarget(
            name: "GrowingAPMLaunchMonitor",
            path: "LaunchMonitor/GrowingAPMLaunchMonitor.xcframework"
        ),
    ]
)
