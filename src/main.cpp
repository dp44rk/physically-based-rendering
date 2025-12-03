#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include "../include/shader.h"
#include "../include/model.h"
#include "../include/camera.h"
#include "../include/app_state.h"
#include "../include/input_handler.h"

// 전역 변수 (콜백 함수에서 접근하기 위해)
AppState* g_appState = nullptr;
GLFWwindow* g_window = nullptr;

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow *window);
void setupShader(Shader& shader, const AppState& appState);
void updateShaderUniforms(Shader& shader, const AppState& appState, 
                         const glm::vec3* lightPositions, const glm::vec3* lightColors);

int main()
{
    using namespace AppConstants;
    
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
    
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "PBR Renderer", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }
    
    glEnable(GL_DEPTH_TEST);
    
    // 애플리케이션 상태 초기화
    AppState appState;
    g_appState = &appState;
    g_window = window;
    
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    
    // 키 매핑 안내 출력
    std::cout << "\n=== PBR Renderer 키 매핑 ===" << std::endl;
    std::cout << "W/A/S/D: 카메라 이동" << std::endl;
    std::cout << "마우스 이동: 시점 변경" << std::endl;
    std::cout << "마우스 휠: 줌 인/아웃" << std::endl;
    std::cout << "V: Tangent Space 모드 토글" << std::endl;
    std::cout << "B: IBL (Image Based Lighting) 모드 토글" << std::endl;
    std::cout << "N: Albedo sRGB 모드 토글" << std::endl;
    std::cout << "0: 마우스 커서 잠금/해제" << std::endl;
    std::cout << "ESC: 종료\n" << std::endl;
    
    Shader shader("shader.vert", "shader.frag");
    Model ourModel("mjolnirFBX.FBX");
    
    // PBR 조명 설정
    glm::vec3 lightPositions[MAX_LIGHTS] = {
        glm::vec3( 4.0f,  4.0f,  4.0f),  // 우상전
        glm::vec3(-4.0f,  4.0f,  4.0f),  // 좌상전
        glm::vec3( 0.0f,  4.0f, -4.0f),  // 상후쪽 리머라이트
        glm::vec3( 0.0f, -4.0f,  4.0f)   // 하전쪽 필
    };
    glm::vec3 lightColors[MAX_LIGHTS] = {
        glm::vec3(65.0f, 65.0f, 65.0f),
        glm::vec3(65.0f, 65.0f, 65.0f),
        glm::vec3(65.0f, 65.0f, 65.0f),
        glm::vec3(65.0f, 65.0f, 65.0f)
    };
    
    setupShader(shader, appState);
    
    while (!glfwWindowShouldClose(window))
    {
        appState.updateTime();
        processInput(window);
        
        glClearColor(CLEAR_COLOR_R, CLEAR_COLOR_G, CLEAR_COLOR_B, CLEAR_COLOR_A);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        shader.use();
        updateShaderUniforms(shader, appState, lightPositions, lightColors);
        
        glm::mat4 projection = glm::perspective(glm::radians(appState.camera.Zoom), 
                                                (float)SCR_WIDTH / (float)SCR_HEIGHT, 
                                                NEAR_PLANE, FAR_PLANE);
        glm::mat4 view = appState.camera.GetViewMatrix();
        shader.setMat4("projection", projection);
        shader.setMat4("view", view);
        
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::scale(model, glm::vec3(MODEL_SCALE));
        shader.setMat4("model", model);
        
        ourModel.Draw(shader, appState.useTangentSpace);
        
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    
    glfwTerminate();
    return 0;
}

void setupShader(Shader& shader, const AppState& appState)
{
    using namespace AppConstants;
    
    shader.use();
    
    // 텍스처 유닛 설정
    shader.setInt("albedoMap", TEXTURE_UNIT_ALBEDO);
    shader.setInt("normalMap", TEXTURE_UNIT_NORMAL);
    shader.setInt("metallicMap", TEXTURE_UNIT_METALLIC);
    shader.setInt("roughnessMap", TEXTURE_UNIT_ROUGHNESS);
    shader.setInt("aoMap", TEXTURE_UNIT_AO);
    shader.setInt("irradianceMap", TEXTURE_UNIT_IRRADIANCE);
    shader.setInt("prefilterMap", TEXTURE_UNIT_PREFILTER);
    shader.setInt("brdfLUT", TEXTURE_UNIT_BRDF_LUT);
    
    // 렌더링 모드 설정
    shader.setBool("useIBL", appState.useIBL);
    shader.setBool("albedoIsSRGB", appState.albedoIsSRGB);
    shader.setBool("useTangentSpace", appState.useTangentSpace);
    
    // 기본 Material 값 설정
    shader.setVec3("albedo", DEFAULT_ALBEDO_R, DEFAULT_ALBEDO_G, DEFAULT_ALBEDO_B);
    shader.setFloat("metallic", DEFAULT_METALLIC);
    shader.setFloat("roughness", DEFAULT_ROUGHNESS);
    shader.setFloat("ao", DEFAULT_AO);
}

void updateShaderUniforms(Shader& shader, const AppState& appState, 
                         const glm::vec3* lightPositions, const glm::vec3* lightColors)
{
    using namespace AppConstants;
    
    // 조명 설정
    for (int i = 0; i < MAX_LIGHTS; ++i)
    {
        shader.setVec3("lightPositions[" + std::to_string(i) + "]", lightPositions[i]);
        shader.setVec3("lightColors[" + std::to_string(i) + "]", lightColors[i]);
    }
    shader.setVec3("viewPos", appState.camera.Position);
    shader.setInt("numLights", MAX_LIGHTS);
    
    // 렌더링 모드 업데이트
    shader.setBool("useTangentSpace", appState.useTangentSpace);
    shader.setBool("useIBL", appState.useIBL);
    shader.setBool("albedoIsSRGB", appState.albedoIsSRGB);
}

void processInput(GLFWwindow *window)
{
    if (!g_appState) return;
    
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
    
    // 카메라 이동
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        g_appState->camera.ProcessKeyboard(FORWARD, g_appState->deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        g_appState->camera.ProcessKeyboard(BACKWARD, g_appState->deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        g_appState->camera.ProcessKeyboard(LEFT, g_appState->deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        g_appState->camera.ProcessKeyboard(RIGHT, g_appState->deltaTime);
    
    // 키 토글 처리 (헬퍼 함수 사용)
    handleToggleKey(window, GLFW_KEY_V, g_appState->keyState.vPressed, 
                   g_appState->useTangentSpace, "Tangent Space");
    handleToggleKey(window, GLFW_KEY_B, g_appState->keyState.bPressed, 
                   g_appState->useIBL, "IBL (Image Based Lighting)");
    handleToggleKey(window, GLFW_KEY_N, g_appState->keyState.nPressed, 
                   g_appState->albedoIsSRGB, "Albedo sRGB");
    
    // 마우스 커서 잠금 처리
    handleCursorLock(window, *g_appState);
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
    if (!g_appState) return;
    
    if (!g_appState->cursorLocked)
        return;
    
    if (g_appState->firstMouse)
    {
        g_appState->lastX = xpos;
        g_appState->lastY = ypos;
        g_appState->firstMouse = false;
    }
    
    float xoffset = xpos - g_appState->lastX;
    float yoffset = g_appState->lastY - ypos;
    
    g_appState->lastX = xpos;
    g_appState->lastY = ypos;
    
    g_appState->camera.ProcessMouseMovement(xoffset, yoffset);
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    if (!g_appState) return;
    g_appState->camera.ProcessMouseScroll(yoffset);
}
