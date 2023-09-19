#!/bin/bash

mkdir Lib

cd ./Project/GrowingAPM
sh ./generate_xcframework.sh
cd ../../

mkdir GrowingAPM
mkdir GrowingAPM/CrashMonitor
mkdir GrowingAPM/UIMonitor
cp -r Sources/Core GrowingAPM/
cp -r Sources/CrashMonitor/Resources GrowingAPM/CrashMonitor
cp -r Lib/GrowingAPMCrashMonitor.xcframework GrowingAPM/CrashMonitor
cp -r Lib/GrowingAPMUIMonitor.xcframework GrowingAPM/UIMonitor
cp ./GrowingAPM.podspec GrowingAPM/
cp ./Package.swift GrowingAPM/
cp -r ./SwiftPM-Wrap GrowingAPM/

mkdir GrowingAPM/SwiftPM-Wrap/GrowingAPMCrashMonitor-Wrapper/Resources
pushd GrowingAPM/SwiftPM-Wrap/GrowingAPMCrashMonitor-Wrapper/Resources
ln -s ../../../CrashMonitor/Resources/GrowingAPMCrashMonitor.bundle GrowingAPMCrashMonitor.bundle
popd

zip -q -r GrowingAPM.zip GrowingAPM/
rm -rf GrowingAPM/
rm -rf Lib/


