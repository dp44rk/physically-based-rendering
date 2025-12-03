#ifndef APP_STATE_H
#define APP_STATE_H

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include "camera.h"

// 상수 정의
namespace AppConstants {
    constexpr unsigned int SCR_WIDTH = 1280;
    constexpr unsigned int SCR_HEIGHT = 720;
    constexpr int MAX_LIGHTS = 4;
    constexpr float NEAR_PLANE = 0.1f;
    constexpr float FAR_PLANE = 100.0f;
    constexpr float MODEL_SCALE = 0.1f;
    constexpr float CLEAR_COLOR_R = 0.1f;
    constexpr float CLEAR_COLOR_G = 0.1f;
    constexpr float CLEAR_COLOR_B = 0.1f;
    constexpr float CLEAR_COLOR_A = 1.0f;
    
    // 텍스처 유닛 번호
    constexpr int TEXTURE_UNIT_ALBEDO = 0;
    constexpr int TEXTURE_UNIT_NORMAL = 1;
    constexpr int TEXTURE_UNIT_METALLIC = 2;
    constexpr int TEXTURE_UNIT_ROUGHNESS = 3;
    constexpr int TEXTURE_UNIT_AO = 4;
    constexpr int TEXTURE_UNIT_IRRADIANCE = 5;
    constexpr int TEXTURE_UNIT_PREFILTER = 6;
    constexpr int TEXTURE_UNIT_BRDF_LUT = 7;
    
    // 기본 Material 값
    constexpr float DEFAULT_ALBEDO_R = 0.8f;
    constexpr float DEFAULT_ALBEDO_G = 0.8f;
    constexpr float DEFAULT_ALBEDO_B = 0.8f;
    constexpr float DEFAULT_METALLIC = 0.5f;
    constexpr float DEFAULT_ROUGHNESS = 0.3f;
    constexpr float DEFAULT_AO = 1.0f;
}

// 애플리케이션 상태 관리 클래스
class AppState {
public:
    // 렌더링 설정
    bool useTangentSpace = true;
    bool useIBL = true;
    bool albedoIsSRGB = true;
    bool cursorLocked = true;
    
    // 카메라
    Camera camera;
    
    // 마우스 상태
    float lastX = AppConstants::SCR_WIDTH / 2.0f;
    float lastY = AppConstants::SCR_HEIGHT / 2.0f;
    bool firstMouse = true;
    
    // 시간
    float deltaTime = 0.0f;
    float lastFrame = 0.0f;
    
    // 키 상태 추적
    struct KeyState {
        bool vPressed = false;  // V: Tangent Space
        bool bPressed = false;  // B: IBL
        bool nPressed = false;  // N: Albedo sRGB
        bool zeroPressed = false;  // 0: Cursor lock
    } keyState;
    
    AppState() : camera(glm::vec3(0.0f, 0.0f, 10.0f)) {}
    
    void updateTime() {
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;
    }
};

#endif

