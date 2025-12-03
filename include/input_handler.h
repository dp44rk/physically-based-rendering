#ifndef INPUT_HANDLER_H
#define INPUT_HANDLER_H

#include <GLFW/glfw3.h>
#include <iostream>
#include "app_state.h"

// 키 토글 헬퍼 함수
inline bool handleToggleKey(GLFWwindow* window, int key, bool& keyPressed, bool& value, const std::string& name) {
    if (glfwGetKey(window, key) == GLFW_PRESS && !keyPressed) {
        value = !value;
        keyPressed = true;
        std::cout << name << ": " << (value ? "ON" : "OFF") << std::endl;
        return true;
    }
    if (glfwGetKey(window, key) == GLFW_RELEASE) {
        keyPressed = false;
    }
    return false;
}

// 마우스 커서 잠금 토글
inline void handleCursorLock(GLFWwindow* window, AppState& appState) {
    if (glfwGetKey(window, GLFW_KEY_0) == GLFW_PRESS && !appState.keyState.zeroPressed) {
        appState.cursorLocked = !appState.cursorLocked;
        if (appState.cursorLocked) {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            std::cout << "마우스 커서: 잠금됨" << std::endl;
            appState.firstMouse = true;
        } else {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            std::cout << "마우스 커서: 해제됨" << std::endl;
        }
        appState.keyState.zeroPressed = true;
    }
    if (glfwGetKey(window, GLFW_KEY_0) == GLFW_RELEASE) {
        appState.keyState.zeroPressed = false;
    }
}

#endif

