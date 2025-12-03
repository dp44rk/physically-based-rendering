cmake_minimum_required(VERSION 3.10)
project(PBR_Renderer)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# OpenGL 및 라이브러리 찾기
find_package(OpenGL REQUIRED)
find_package(glfw3 REQUIRED)
find_package(assimp REQUIRED)
find_package(glad QUIET)

# GLAD 소스 파일
include_directories(${CMAKE_SOURCE_DIR}/include)

# GLM (OpenGL Mathematics)
if(NOT EXISTS "${CMAKE_SOURCE_DIR}/include/glm")
    message(STATUS "GLM not found. Please download GLM and place it in include/glm")
endif()

include_directories(
    ${CMAKE_SOURCE_DIR}/include
    ${OPENGL_INCLUDE_DIR}
    ${ASSIMP_INCLUDE_DIRS}
)

# 소스 파일
set(SOURCES
    src/main.cpp
    src/model.cpp
    src/shader.cpp
    src/camera.cpp
    src/mesh.cpp
    src/glad.c
)

# 실행 파일 생성
add_executable(${PROJECT_NAME} ${SOURCES})

# 라이브러리 링크
target_link_libraries(${PROJECT_NAME}
    ${OPENGL_LIBRARIES}
    glfw
    ${ASSIMP_LIBRARIES}
)

# 컴파일 옵션
if(APPLE)
    target_link_libraries(${PROJECT_NAME} "-framework IOKit" "-framework Cocoa")
endif()

