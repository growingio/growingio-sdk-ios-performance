#!/bin/bash

# 参考: https://help.apple.com/xcode/mac/11.4/#/dev544efab96
XCWORKSPACE_NAME="GrowingAPM"
DERIVED_DATA_PATH="derivedData"
ARCHIVE_PATH="archive"

echo "---------------------"
echo -e "\033[36m step: clear pod cache \033[0m"
rm -rf "./Podfile.lock"
rm -rf "./Pods"
pod cache clean GrowingUtils --all --verbose

echo "---------------------"
echo -e "\033[36m step: pod install \033[0m"
pod install

echo "---------------------"
echo -e "\033[36m step: clean build folder \033[0m"
rm -rf ${DERIVED_DATA_PATH}
rm -rf ${ARCHIVE_PATH}

schemes=("GrowingAPMCrashMonitor" "GrowingAPMLaunchMonitor" GrowingAPMUIMonitor)
if [ $# -gt 0 ]
then
	schemes=($@)
fi
echo "---------------------"
echo -e "\033[36m step: generate xcframework for ${schemes[@]} \033[0m"

for i in ${schemes[@]}; do 
	FRAMEWORK_NAME=$i
	iPHONE_OS_ARCHIVE_PATH="${ARCHIVE_PATH}/iphoneos"
	iPHONE_OS_FRAMEWORK_PATH=${iPHONE_OS_ARCHIVE_PATH}.xcarchive/Products/Library/Frameworks/${FRAMEWORK_NAME}.framework
	iPHONE_SIMULATOR_ARCHIVE_PATH="${ARCHIVE_PATH}/iphonesimulator"
	iPHONE_SIMULATOR_FRAMEWORK_PATH=${iPHONE_SIMULATOR_ARCHIVE_PATH}.xcarchive/Products/Library/Frameworks/${FRAMEWORK_NAME}.framework
	OUTPUT_PATH="../../Lib/${FRAMEWORK_NAME}.xcframework"

	echo "---------------------"
	echo -e "\033[36m step: clear output \033[0m"
	rm -rf ${OUTPUT_PATH}

	echo "---------------------"
	echo -e "\033[36m step: generate ios-arm64_armv7 framework \033[0m"
	xcodebuild archive \
	-workspace ${XCWORKSPACE_NAME}.xcworkspace \
	-scheme ${FRAMEWORK_NAME} \
	-destination "generic/platform=iOS" \
	-configuration "Release" \
	-archivePath ${iPHONE_OS_ARCHIVE_PATH} \
	-derivedDataPath ${DERIVED_DATA_PATH} || exit 1

	echo "---------------------"
	echo -e "\033[36m step: generate ios-arm64_i386_x86_64-simulator framework \033[0m"
	xcodebuild archive \
	-workspace ${XCWORKSPACE_NAME}.xcworkspace \
	-scheme ${FRAMEWORK_NAME} \
	-destination "generic/platform=iOS Simulator" \
	-configuration "Release" \
	-archivePath ${iPHONE_SIMULATOR_ARCHIVE_PATH} \
	-derivedDataPath ${DERIVED_DATA_PATH} || exit 1

	echo "---------------------"
	echo -e "\033[36m step: delete _CodeSignature folder in framework which is unnecessary \033[0m"
	rm -rf ${iPHONE_OS_FRAMEWORK_PATH}/_CodeSignature
	rm -rf ${iPHONE_SIMULATOR_FRAMEWORK_PATH}/_CodeSignature

	echo "---------------------"
	echo -e "\033[36m step: generate xcframework \033[0m"
	xcodebuild -create-xcframework \
	-framework ${iPHONE_OS_FRAMEWORK_PATH} \
	-framework ${iPHONE_SIMULATOR_FRAMEWORK_PATH} \
	-output ${OUTPUT_PATH} || exit 1
done

echo "---------------------"
echo -e "\033[36m step: clean build folder \033[0m"
rm -rf ${DERIVED_DATA_PATH}
rm -rf ${ARCHIVE_PATH}

echo "---------------------"
echo -e "\033[36m step: pod deintegrate \033[0m"
pod deintegrate
rm -rf "./Podfile.lock"
