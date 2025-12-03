#!/bin/bash

# 깨끗한 빌드 스크립트 (모든 빌드 파일 삭제 후 재빌드)

set -e

echo "=== 깨끗한 빌드 시작 ==="

# 빌드 디렉토리 완전 삭제
BUILD_DIR="build"
if [ -d "$BUILD_DIR" ]; then
    echo "기존 빌드 디렉토리 삭제 중..."
    rm -rf "$BUILD_DIR"
fi

# CMake 캐시 파일 삭제
if [ -f "CMakeCache.txt" ]; then
    rm -f CMakeCache.txt
fi

if [ -d "CMakeFiles" ]; then
    rm -rf CMakeFiles
fi

# 빌드 실행
./build.sh

