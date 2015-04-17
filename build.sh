#!/bin/sh

SOURCE_TREE=$(dirname $(readlink -f "$0"))

echo "build directory: ${PWD}"
echo "source tree directory: ${SOURCE_TREE}"

if [ ${PWD} = ${SOURCE_TREE} ] ; then
	echo "$0 must be executed out-of-tree"
fi

BUILD_BASE_DIR="${PWD}"

BUILD_DIR_W32="${BUILD_BASE_DIR}/windows-x86_32"
BUILD_DIR_W64="${BUILD_BASE_DIR}/windows-x86_64"

if [ ! -d ${BUILD_DIR_W32} ] ; then
	mkdir -p ${BUILD_DIR_W32}
fi

if [ ! -d ${BUILD_DIR_W64} ] ; then
	mkdir -p ${BUILD_DIR_W64}
fi

cd ${BUILD_DIR_W32}
cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE="${SOURCE_TREE}/cmake/Toolchain-cross-mingw32-linux.cmake" "${SOURCE_TREE}"
make ${MAKEOPTS}

cd ${BUILD_DIR_W64}
cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE="${SOURCE_TREE}/cmake/Toolchain-cross-mingw64-linux.cmake" "${SOURCE_TREE}"
make ${MAKEOPTS}
