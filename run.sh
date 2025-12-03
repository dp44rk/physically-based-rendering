#!/bin/bash

# PBR Renderer 실행 스크립트

BUILD_DIR="build"
EXECUTABLE="$BUILD_DIR/PBR_Renderer"

# 빌드가 안 되어 있으면 빌드
if [ ! -f "$EXECUTABLE" ]; then
    echo "실행 파일이 없습니다. 빌드를 시작합니다..."
    ./build.sh
fi

# 실행
echo "PBR Renderer 실행 중..."
cd "$(dirname "$0")"
./"$EXECUTABLE"

