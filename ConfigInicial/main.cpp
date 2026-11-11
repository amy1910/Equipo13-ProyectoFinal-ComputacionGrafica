#include <cstdlib>
#include <iostream>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "LightManager.h"
#include "Renderer.h"
#include "SceneManager.h"

namespace {
constexpr int kWindowWidth = 1280;
constexpr int kWindowHeight = 800;

void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mode) {
    (void)scancode;
    (void)mode;
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    }
    if (SceneManager* scene = static_cast<SceneManager*>(glfwGetWindowUserPointer(window))) {
        scene->HandleKey(key, action);
    }
}

void MouseCallback(GLFWwindow* window, double xPosition, double yPosition) {
    if (SceneManager* scene = static_cast<SceneManager*>(glfwGetWindowUserPointer(window))) {
        scene->HandleMouse(xPosition, yPosition);
    }
}
}

int main() {
    if (!glfwInit()) {
        std::cerr << "No se pudo inicializar GLFW.\n";
        return EXIT_FAILURE;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(kWindowWidth, kWindowHeight,
        "Xochimilco - Dia de Muertos", nullptr, nullptr);
    if (!window) {
        std::cerr << "No se pudo crear la ventana GLFW.\n";
        glfwTerminate();
        return EXIT_FAILURE;
    }

    glfwMakeContextCurrent(window);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
        std::cerr << "No se pudo inicializar GLEW.\n";
        glfwDestroyWindow(window);
        glfwTerminate();
        return EXIT_FAILURE;
    }

    glEnable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);

    int framebufferWidth = 0;
    int framebufferHeight = 0;
    glfwGetFramebufferSize(window, &framebufferWidth, &framebufferHeight);

    {
        SceneManager scene;
        LightManager lights;
        Renderer renderer(framebufferWidth, framebufferHeight);
        glfwSetWindowUserPointer(window, &scene);
        glfwSetKeyCallback(window, KeyCallback);
        glfwSetCursorPosCallback(window, MouseCallback);

        double previousTime = glfwGetTime();
        while (!glfwWindowShouldClose(window)) {
            const double currentTime = glfwGetTime();
            const float deltaTime = static_cast<float>(currentTime - previousTime);
            previousTime = currentTime;

            glfwPollEvents();
            scene.Update(deltaTime);
            renderer.Draw(scene, lights, static_cast<float>(currentTime));
            glfwSwapBuffers(window);
        }
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    return EXIT_SUCCESS;
}
