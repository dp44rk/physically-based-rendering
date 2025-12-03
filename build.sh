#!/bin/bash

# PBR Renderer 빌드 스크립트

set -e  # 에러 발생 시 스크립트 중단

echo "=== PBR Renderer 빌드 시작 ==="

# 빌드 디렉토리 생성
BUILD_DIR="build"
if [ -d "$BUILD_DIR" ]; then
    echo "기존 빌드 디렉토리 삭제 중..."
    rm -rf "$BUILD_DIR"
fi

mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

echo "CMake 구성 중..."
cmake ..

echo "빌드 중..."
make -j$(sysctl -n hw.ncpu 2>/dev/null || echo 4)

echo ""
echo "=== 빌드 완료 ==="
echo "실행 파일: $BUILD_DIR/PBR_Renderer"
echo ""
echo "실행하려면: ./$BUILD_DIR/PBR_Renderer"

