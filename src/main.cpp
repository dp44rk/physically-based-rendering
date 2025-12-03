#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include "../include/shader.h"
#include "../include/model.h"
#include "../include/camera.h"

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow *window);

const unsigned int SCR_WIDTH = 1280;
const unsigned int SCR_HEIGHT = 720;

// 기본 뷰: 모델이 프레임 안에 들어오도록 적당한 거리
Camera camera(glm::vec3(0.0f, 0.0f, 10.0f));
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

float deltaTime = 0.0f;
float lastFrame = 0.0f;

// 키 상태 추적 (한 번만 감지하기 위해)
bool useTangentSpace = true;
bool useIBL = true;
bool albedoIsSRGB = true;
bool vPressed = false;  // v: Tangent Space
bool bPressed = false;  // b: IBL
bool nPressed = false;  // n: Albedo sRGB
bool mPressed = false;  // m: (예비)
bool zeroPressed = false;
bool cursorLocked = true;  // 마우스 커서 잠금 상태

int main()
{
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
    
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }
    
    glEnable(GL_DEPTH_TEST);
    
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
    
    // 모델 로드 (FBX 버전을 사용해 UV/법선/탄젠트 보존)
    Model ourModel("mjolnirFBX.FBX");
    
    // PBR 조명 설정 - 모델 주변에 배치 (스케일 조정)
    glm::vec3 lightPositions[] = {
        glm::vec3( 4.0f,  4.0f,  4.0f),  // 우상전
        glm::vec3(-4.0f,  4.0f,  4.0f),  // 좌상전
        glm::vec3( 0.0f,  4.0f, -4.0f),  // 상후쪽 리머라이트
        glm::vec3( 0.0f, -4.0f,  4.0f)   // 하전쪽 필
    };
    glm::vec3 lightColors[] = {
        glm::vec3(65.0f, 65.0f, 65.0f),
        glm::vec3(65.0f, 65.0f, 65.0f),
        glm::vec3(65.0f, 65.0f, 65.0f),
        glm::vec3(65.0f, 65.0f, 65.0f)
    };
    
    shader.use();
    shader.setInt("albedoMap", 0);
    shader.setInt("normalMap", 1);
    shader.setInt("metallicMap", 2);
    shader.setInt("roughnessMap", 3);
    shader.setInt("aoMap", 4);
    shader.setInt("irradianceMap", 5);
    shader.setInt("prefilterMap", 6);
    shader.setInt("brdfLUT", 7);
    shader.setBool("useIBL", useIBL);
    shader.setBool("albedoIsSRGB", albedoIsSRGB);
    shader.setBool("useTangentSpace", useTangentSpace);
    
    // 기본 Material 값 설정 (맵이 없을 때 사용) - 밝은 색으로 변경
    shader.setVec3("albedo", 0.8f, 0.8f, 0.8f);  // 밝은 회색
    shader.setFloat("metallic", 0.5f);
    shader.setFloat("roughness", 0.3f);
    shader.setFloat("ao", 1.0f);
    
    while (!glfwWindowShouldClose(window))
    {
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;
        
        processInput(window);
        
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        shader.use();
        
        // 조명 위치 및 색상 설정
        for (int i = 0; i < 4; ++i)
        {
            shader.setVec3("lightPositions[" + std::to_string(i) + "]", lightPositions[i]);
            shader.setVec3("lightColors[" + std::to_string(i) + "]", lightColors[i]);
        }
        shader.setVec3("viewPos", camera.Position);
        shader.setInt("numLights", 4);
        
        // 셰이더 설정 업데이트
        shader.setBool("useTangentSpace", useTangentSpace);
        shader.setBool("useIBL", useIBL);
        shader.setBool("albedoIsSRGB", albedoIsSRGB);
        
        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
        glm::mat4 view = camera.GetViewMatrix();
        shader.setMat4("projection", projection);
        shader.setMat4("view", view);
        
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(0.0f, 0.0f, 0.0f));
        model = glm::scale(model, glm::vec3(0.1f, 0.1f, 0.1f));
        shader.setMat4("model", model);
        
        ourModel.Draw(shader, useTangentSpace);
        
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    
    glfwTerminate();
    return 0;
}

void processInput(GLFWwindow *window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
    
    // 카메라 이동
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.ProcessKeyboard(RIGHT, deltaTime);
    
    // V 키: Tangent Space 모드 토글
    if (glfwGetKey(window, GLFW_KEY_V) == GLFW_PRESS && !vPressed)
    {
        useTangentSpace = !useTangentSpace;
        vPressed = true;
        std::cout << "Tangent Space: " << (useTangentSpace ? "ON" : "OFF") << std::endl;
    }
    if (glfwGetKey(window, GLFW_KEY_V) == GLFW_RELEASE)
    {
        vPressed = false;
    }
    
    // B 키: IBL 모드 토글
    if (glfwGetKey(window, GLFW_KEY_B) == GLFW_PRESS && !bPressed)
    {
        useIBL = !useIBL;
        bPressed = true;
        std::cout << "IBL (Image Based Lighting): " << (useIBL ? "ON" : "OFF") << std::endl;
    }
    if (glfwGetKey(window, GLFW_KEY_B) == GLFW_RELEASE)
    {
        bPressed = false;
    }
    
    // N 키: Albedo sRGB 모드 토글
    if (glfwGetKey(window, GLFW_KEY_N) == GLFW_PRESS && !nPressed)
    {
        albedoIsSRGB = !albedoIsSRGB;
        nPressed = true;
        std::cout << "Albedo sRGB: " << (albedoIsSRGB ? "ON" : "OFF") << std::endl;
    }
    if (glfwGetKey(window, GLFW_KEY_N) == GLFW_RELEASE)
    {
        nPressed = false;
    }
    
    // 0 키: 마우스 커서 잠금/해제 토글
    if (glfwGetKey(window, GLFW_KEY_0) == GLFW_PRESS && !zeroPressed)
    {
        cursorLocked = !cursorLocked;
        if (cursorLocked)
        {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            std::cout << "마우스 커서: 잠금됨" << std::endl;
            firstMouse = true;  // 마우스 잠금 시 첫 이동 초기화
        }
        else
        {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            std::cout << "마우스 커서: 해제됨" << std::endl;
        }
        zeroPressed = true;
    }
    if (glfwGetKey(window, GLFW_KEY_0) == GLFW_RELEASE)
    {
        zeroPressed = false;
    }
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
    // 마우스 커서가 잠겨있을 때만 카메라 회전
    if (!cursorLocked)
        return;
    
    if (firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }
    
    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos;
    
    lastX = xpos;
    lastY = ypos;
    
    camera.ProcessMouseMovement(xoffset, yoffset);
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    camera.ProcessMouseScroll(yoffset);
}
