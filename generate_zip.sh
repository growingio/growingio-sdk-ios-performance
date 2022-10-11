#!/bin/bash

mkdir Lib
cd ./Project/GrowingAPM
sh ./generate_xcframework.sh
cd ../../
mkdir GrowingAPM
mkdir GrowingAPM/CrashMonitor
mkdir GrowingAPM/UIMonitor
mkdir GrowingAPM/LaunchMonitor
cp -r Sources/Core GrowingAPM/
cp -r Lib/GrowingAPMCrashMonitor.xcframework GrowingAPM/CrashMonitor
cp -r Lib/GrowingAPMUIMonitor.xcframework GrowingAPM/UIMonitor
cp -r Lib/GrowingAPMLaunchMonitor.xcframework GrowingAPM/LaunchMonitor
cp ./GrowingAPM.podspec GrowingAPM/
zip -q -r GrowingAPM.zip GrowingAPM/
rm -rf GrowingAPM/


