#!/bin/bash
# build script for Mir on android arm devices
#
set -e

BUILD_DIR=build-android-arm

if [ "$MIR_NDK_PATH" = "" ]; then
    export MIR_NDK_PATH=`pwd`/partial-armhf-chroot
    if [ ! -d ${MIR_NDK_PATH} ]; then 
        echo "no partial root specified or detected. attempting to create one"
    fi
fi

pushd tools > /dev/null
    ./setup-partial-armhf-chroot.sh ${MIR_NDK_PATH}
popd > /dev/null

echo "Using MIR_NDK_PATH: $MIR_NDK_PATH"

#start with a clean build every time
rm -rf ${BUILD_DIR}
mkdir ${BUILD_DIR}
pushd ${BUILD_DIR} > /dev/null 

    export PKG_CONFIG_PATH="${MIR_NDK_PATH}/usr/lib/pkgconfig:${MIR_NDK_PATH}/usr/lib/arm-linux-gnueabihf/pkgconfig"
    echo "Using PKG_CONFIG_PATH: $PKG_CONFIG_PATH"
    cmake -DCMAKE_TOOLCHAIN_FILE=../cmake/LinuxCrossCompile.cmake \
      -DBoost_COMPILER=-gcc \
      -DMIR_PLATFORM=android \
      .. 

    cmake --build .

popd ${BUILD_DIR} > /dev/null 
