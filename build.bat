@echo off
REM PBR Renderer 빌드 스크립트 (Windows)

echo === PBR Renderer 빌드 시작 ===

REM 빌드 디렉토리 생성
if exist build (
    echo 기존 빌드 디렉토리 삭제 중...
    rmdir /s /q build
)

mkdir build
cd build

echo CMake 구성 중...
cmake ..

echo 빌드 중...
cmake --build . --config Release

echo.
echo === 빌드 완료 ===
echo 실행 파일: build\Release\PBR_Renderer.exe
echo.
echo 실행하려면: build\Release\PBR_Renderer.exe

cd ..

