// ================================================================
// Xochimilco - Dia de Muertos
// 318041267
// 320035014
// ================================================================

#include <iostream>
#include <cmath>
#include <cstdlib>
#include <string>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Shader.h"
#include "Camera.h"
#include "Model.h"

// ------------------------- Prototipos ----------------------------

void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mode);
void MouseCallback(GLFWwindow* window, double xPos, double yPos);
void DoMovement();
void UpdateScene(float dt);
void UpdateKeyframes(float dt);
void SetLighting(Shader& shader, const glm::vec3& viewPosition);
void DrawLamp(Shader& shader, GLuint VAO, const glm::vec3& position,
              const glm::vec3& color, float scale);

// ------------------------- Ventana --------------------------------

const GLuint WIDTH = 1280;
const GLuint HEIGHT = 800;
int SCREEN_WIDTH = WIDTH;
int SCREEN_HEIGHT = HEIGHT;

// ------------------------- Camara ---------------------------------

Camera camera(glm::vec3(0.0f, 12.0f, 26.0f));
GLfloat lastX = WIDTH / 2.0f;
GLfloat lastY = HEIGHT / 2.0f;
bool keys[1024] = { false };
bool firstMouse = true;

GLfloat deltaTime = 0.0f;
GLfloat lastFrame = 0.0f;

// ------------------------- Escena ---------------------------------

// El centro del escenario se encuentra aproximadamente en (0,0,0).
// X: izquierda/derecha; Y: altura; Z: profundidad.

bool showLightMarkers = false;
bool nightLights = true;
bool animationEnabled = true;

// Luces cálidas distribuidas entre las islas.
// Se envían a pointLights[0..3] del shader.
glm::vec3 candlePositions[4] = {
    glm::vec3(-5.0f, 2.0f, -3.0f),
    glm::vec3( 4.0f, 2.0f, -4.0f),
    glm::vec3( 0.0f, 2.0f,  5.0f),
    glm::vec3( 0.0f, 4.0f,  0.0f)
};

glm::vec3 candleColors[4] = {
    glm::vec3(1.00f, 0.48f, 0.12f),
    glm::vec3(1.00f, 0.58f, 0.18f),
    glm::vec3(1.00f, 0.38f, 0.08f),
    glm::vec3(0.25f, 0.38f, 1.00f)
};

// ------------------------- Keyframes ------------------------------

// Cada keyframe define la posición y orientación de una trajinera.
// Se interpola entre puntos para generar un recorrido continuo.
struct BOAT_FRAME {
    glm::vec3 position;
    float rotationY;
    float duration;
};

const int BOAT_FRAME_COUNT = 6;

BOAT_FRAME boatPath[BOAT_FRAME_COUNT] = {
    { glm::vec3(-8.0f, 0.05f,  1.5f),   0.0f, 3.0f },
    { glm::vec3(-5.0f, 0.05f,  5.0f),  35.0f, 3.0f },
    { glm::vec3( 0.0f, 0.05f,  6.0f),  90.0f, 3.0f },
    { glm::vec3( 5.0f, 0.05f,  3.0f), 145.0f, 3.0f },
    { glm::vec3( 7.0f, 0.05f, -1.0f), 180.0f, 3.0f },
    { glm::vec3( 2.0f, 0.05f, -6.0f), 250.0f, 3.0f }
};

glm::vec3 boatPosition = boatPath[0].position;
float boatRotation = boatPath[0].rotationY;
int boatFrameIndex = 0;
float boatFrameTime = 0.0f;

// Segunda trajinera: desfase en el recorrido para que no viajen juntas.
glm::vec3 boat2Position(7.0f, 0.05f, 6.0f);
float boat2Rotation = 180.0f;

// Máquina de estados de la escena.
enum SceneState {
    SCENE_INTRO,
    SCENE_BOATS_MOVING,
    SCENE_BOATS_PAUSED,
    SCENE_FINALE
};

SceneState sceneState = SCENE_INTRO;
float stateTimer = 0.0f;
float sceneTime = 0.0f;

// Oscilaciones de decoración.
float papelPicadoAngle = 0.0f;
float waterOffset = 0.0f;

// ------------------------- Cubo de luz ----------------------------

float lampVertices[] = {
    // Posición             // Normal
    -0.5f,-0.5f,-0.5f,  0,0,-1,   0.5f,-0.5f,-0.5f,  0,0,-1,   0.5f, 0.5f,-0.5f,  0,0,-1,
     0.5f, 0.5f,-0.5f,  0,0,-1,  -0.5f, 0.5f,-0.5f,  0,0,-1,  -0.5f,-0.5f,-0.5f,  0,0,-1,
    -0.5f,-0.5f, 0.5f,  0,0, 1,   0.5f,-0.5f, 0.5f,  0,0, 1,   0.5f, 0.5f, 0.5f,  0,0, 1,
     0.5f, 0.5f, 0.5f,  0,0, 1,  -0.5f, 0.5f, 0.5f,  0,0, 1,  -0.5f,-0.5f, 0.5f,  0,0, 1,
    -0.5f, 0.5f, 0.5f, -1,0, 0,  -0.5f, 0.5f,-0.5f, -1,0, 0,  -0.5f,-0.5f,-0.5f, -1,0, 0,
    -0.5f,-0.5f,-0.5f, -1,0, 0,  -0.5f,-0.5f, 0.5f, -1,0, 0,  -0.5f, 0.5f, 0.5f, -1,0, 0,
     0.5f, 0.5f, 0.5f,  1,0, 0,   0.5f, 0.5f,-0.5f,  1,0, 0,   0.5f,-0.5f,-0.5f,  1,0, 0,
     0.5f,-0.5f,-0.5f,  1,0, 0,   0.5f,-0.5f, 0.5f,  1,0, 0,   0.5f, 0.5f, 0.5f,  1,0, 0,
    -0.5f,-0.5f,-0.5f,  0,-1,0,   0.5f,-0.5f,-0.5f,  0,-1,0,   0.5f,-0.5f, 0.5f,  0,-1,0,
     0.5f,-0.5f, 0.5f,  0,-1,0,  -0.5f,-0.5f, 0.5f,  0,-1,0,  -0.5f,-0.5f,-0.5f,  0,-1,0,
    -0.5f, 0.5f,-0.5f,  0, 1,0,   0.5f, 0.5f,-0.5f,  0, 1,0,   0.5f, 0.5f, 0.5f,  0, 1,0,
     0.5f, 0.5f, 0.5f,  0, 1,0,  -0.5f, 0.5f, 0.5f,  0, 1,0,  -0.5f, 0.5f,-0.5f,  0, 1,0
};

// ------------------------- Funciones auxiliares -------------------

static glm::mat4 Transform(const glm::vec3& position,
                          const glm::vec3& scale,
                          float rotationY = 0.0f) {
    glm::mat4 model(1.0f);
    model = glm::translate(model, position);
    model = glm::rotate(model, glm::radians(rotationY), glm::vec3(0, 1, 0));
    model = glm::scale(model, scale);
    return model;
}

static void DrawObject(Shader& shader, Model& object,
                       const glm::vec3& position,
                       const glm::vec3& scale,
                       float rotationY = 0.0f) {
    glm::mat4 model = Transform(position, scale, rotationY);
    glUniformMatrix4fv(glGetUniformLocation(shader.Program, "model"),
                       1, GL_FALSE, glm::value_ptr(model));
    object.Draw(shader);
}

// ------------------------- Iluminación ----------------------------

void SetLighting(Shader& shader, const glm::vec3& viewPosition) {
    shader.Use();

    glUniform3f(glGetUniformLocation(shader.Program, "viewPos"),
                viewPosition.x, viewPosition.y, viewPosition.z);

    // Luz direccional: iluminación azulada de ambiente nocturno.
    glUniform3f(glGetUniformLocation(shader.Program, "dirLight.direction"),
                -0.2f, -1.0f, -0.25f);
    glUniform3f(glGetUniformLocation(shader.Program, "dirLight.ambient"),
                0.10f, 0.12f, 0.22f);
    glUniform3f(glGetUniformLocation(shader.Program, "dirLight.diffuse"),
                0.22f, 0.25f, 0.38f);
    glUniform3f(glGetUniformLocation(shader.Program, "dirLight.specular"),
                0.25f, 0.25f, 0.35f);

    // Cuatro luces puntuales cálidas/azuladas.
    for (int i = 0; i < 4; ++i) {
        std::string base = "pointLights[" + std::to_string(i) + "].";
        float flicker = 0.82f + 0.18f * sin(sceneTime * (5.0f + i) + i);

        glm::vec3 color = nightLights ? candleColors[i] * flicker
                                      : glm::vec3(0.015f);

        glUniform3f(glGetUniformLocation(shader.Program, (base + "position").c_str()),
                    candlePositions[i].x, candlePositions[i].y, candlePositions[i].z);
        glUniform3f(glGetUniformLocation(shader.Program, (base + "ambient").c_str()),
                    color.r * 0.16f, color.g * 0.16f, color.b * 0.16f);
        glUniform3f(glGetUniformLocation(shader.Program, (base + "diffuse").c_str()),
                    color.r, color.g, color.b);
        glUniform3f(glGetUniformLocation(shader.Program, (base + "specular").c_str()),
                    color.r, color.g, color.b);
        glUniform1f(glGetUniformLocation(shader.Program, (base + "constant").c_str()), 1.0f);
        glUniform1f(glGetUniformLocation(shader.Program, (base + "linear").c_str()), 0.09f);
        glUniform1f(glGetUniformLocation(shader.Program, (base + "quadratic").c_str()), 0.032f);
    }

    // Reflector unido a la cámara: ayuda a inspeccionar el escenario.
    glUniform3f(glGetUniformLocation(shader.Program, "spotLight.position"),
                viewPosition.x, viewPosition.y, viewPosition.z);
    glUniform3f(glGetUniformLocation(shader.Program, "spotLight.direction"),
                camera.GetFront().x, camera.GetFront().y, camera.GetFront().z);
    glUniform3f(glGetUniformLocation(shader.Program, "spotLight.ambient"),
                0.015f, 0.015f, 0.025f);
    glUniform3f(glGetUniformLocation(shader.Program, "spotLight.diffuse"),
                0.20f, 0.20f, 0.30f);
    glUniform3f(glGetUniformLocation(shader.Program, "spotLight.specular"),
                0.25f, 0.25f, 0.30f);
    glUniform1f(glGetUniformLocation(shader.Program, "spotLight.constant"), 1.0f);
    glUniform1f(glGetUniformLocation(shader.Program, "spotLight.linear"), 0.09f);
    glUniform1f(glGetUniformLocation(shader.Program, "spotLight.quadratic"), 0.032f);
    glUniform1f(glGetUniformLocation(shader.Program, "spotLight.cutOff"),
                glm::cos(glm::radians(12.5f)));
    glUniform1f(glGetUniformLocation(shader.Program, "spotLight.outerCutOff"),
                glm::cos(glm::radians(17.5f)));

    glUniform1f(glGetUniformLocation(shader.Program, "material.shininess"), 32.0f);
}

void DrawLamp(Shader& shader, GLuint VAO, const glm::vec3& position,
              const glm::vec3& color, float scale) {
    shader.Use();
    glUniform3f(glGetUniformLocation(shader.Program, "lampColor"),
                color.r, color.g, color.b);

    glm::mat4 model(1.0f);
    model = glm::translate(model, position);
    model = glm::scale(model, glm::vec3(scale));
    glUniformMatrix4fv(glGetUniformLocation(shader.Program, "model"),
                       1, GL_FALSE, glm::value_ptr(model));

    glBindVertexArray(VAO);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);
}

// ------------------------- Animaciones ----------------------------

void UpdateKeyframes(float dt) {
    if (!animationEnabled) return;

    if (sceneState == SCENE_INTRO) {
        stateTimer += dt;
        if (stateTimer >= 2.0f) {
            sceneState = SCENE_BOATS_MOVING;
            stateTimer = 0.0f;
        }
    }
    else if (sceneState == SCENE_BOATS_MOVING) {
        boatFrameTime += dt;

        float duration = boatPath[boatFrameIndex].duration;
        float t = glm::clamp(boatFrameTime / duration, 0.0f, 1.0f);

        int nextIndex = (boatFrameIndex + 1) % BOAT_FRAME_COUNT;
        boatPosition = glm::mix(boatPath[boatFrameIndex].position,
                                boatPath[nextIndex].position, t);

        float angleA = boatPath[boatFrameIndex].rotationY;
        float angleB = boatPath[nextIndex].rotationY;
        boatRotation = glm::mix(angleA, angleB, t);

        if (boatFrameTime >= duration) {
            boatFrameTime = 0.0f;
            boatFrameIndex = nextIndex;
            if (boatFrameIndex == 0) {
                sceneState = SCENE_BOATS_PAUSED;
                stateTimer = 0.0f;
            }
        }
    }
    else if (sceneState == SCENE_BOATS_PAUSED) {
        stateTimer += dt;
        if (stateTimer >= 2.5f) {
            sceneState = SCENE_FINALE;
            stateTimer = 0.0f;
        }
    }
    else if (sceneState == SCENE_FINALE) {
        // La segunda trajinera recorre una trayectoria circular suave.
        float t = sceneTime * 0.22f;
        boat2Position = glm::vec3(6.0f * cos(t), 0.05f, 4.5f * sin(t));
        boat2Rotation = -glm::degrees(t) + 90.0f;
    }
}

void UpdateScene(float dt) {
    sceneTime += dt;
    waterOffset += dt * 0.12f;
    papelPicadoAngle = 5.0f * sin(sceneTime * 2.0f);
    UpdateKeyframes(dt);
}

// ------------------------- Movimiento de camara -------------------

void DoMovement() {
    if (keys[GLFW_KEY_W] || keys[GLFW_KEY_UP])
        camera.ProcessKeyboard(FORWARD, deltaTime);
    if (keys[GLFW_KEY_S] || keys[GLFW_KEY_DOWN])
        camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (keys[GLFW_KEY_A] || keys[GLFW_KEY_LEFT])
        camera.ProcessKeyboard(LEFT, deltaTime);
    if (keys[GLFW_KEY_D] || keys[GLFW_KEY_RIGHT])
        camera.ProcessKeyboard(RIGHT, deltaTime);
}

// ------------------------- Callbacks -------------------------------

void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mode) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GL_TRUE);

    if (key >= 0 && key < 1024) {
        if (action == GLFW_PRESS) keys[key] = true;
        else if (action == GLFW_RELEASE) keys[key] = false;
    }

    if (action == GLFW_PRESS) {
        if (key == GLFW_KEY_SPACE) nightLights = !nightLights;
        if (key == GLFW_KEY_M) showLightMarkers = !showLightMarkers;
        if (key == GLFW_KEY_P) animationEnabled = !animationEnabled;

        if (key == GLFW_KEY_R) {
            sceneState = SCENE_INTRO;
            stateTimer = 0.0f;
            boatFrameIndex = 0;
            boatFrameTime = 0.0f;
            boatPosition = boatPath[0].position;
            boatRotation = boatPath[0].rotationY;
            animationEnabled = true;
        }
    }
}

void MouseCallback(GLFWwindow* window, double xPos, double yPos) {
    if (firstMouse) {
        lastX = (GLfloat)xPos;
        lastY = (GLfloat)yPos;
        firstMouse = false;
    }

    GLfloat xOffset = (GLfloat)xPos - lastX;
    GLfloat yOffset = lastY - (GLfloat)yPos;

    lastX = (GLfloat)xPos;
    lastY = (GLfloat)yPos;

    camera.ProcessMouseMovement(xOffset, yOffset);
}

// ------------------------- Programa principal --------------------

int main() {
    if (!glfwInit()) {
        std::cout << "No se pudo inicializar GLFW.\n";
        return EXIT_FAILURE;
    }

    // OpenGL 3.3 Core Profile.
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(
        WIDTH, HEIGHT, "Xochimilco - Dia de Muertos", nullptr, nullptr);

    if (!window) {
        std::cout << "No se pudo crear la ventana GLFW.\n";
        glfwTerminate();
        return EXIT_FAILURE;
    }

    glfwMakeContextCurrent(window);
    glfwGetFramebufferSize(window, &SCREEN_WIDTH, &SCREEN_HEIGHT);

    glfwSetKeyCallback(window, KeyCallback);
    glfwSetCursorPosCallback(window, MouseCallback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
        std::cout << "No se pudo inicializar GLEW.\n";
        glfwTerminate();
        return EXIT_FAILURE;
    }

    glViewport(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Shaders del proyecto original.
    Shader lightingShader("Shader/lighting.vs", "Shader/lighting.frag");
    Shader lampShader("Shader/lamp.vs", "Shader/lamp.frag");

    // ============================================================
    // CARGA DE MODELOS OBJ
    // Si un modelo tiene otro nombre, cambia únicamente su ruta.
    // ============================================================

    // Agua, suelo e islas.
    Model Water((char*)"Models/agua.obj");
    Model Island((char*)"Models/isla.obj");
    Model IslandEdge((char*)"Models/borde_isla.obj");
    Model Bridge((char*)"Models/puente_madera.obj");

    // Isla de las Munecas.
    Model Tree((char*)"Models/arbol.obj");
    Model HangingDoll((char*)"Models/muneca_colgante.obj");
    Model Cabin((char*)"Models/cabana.obj");
    Model Reeds((char*)"Models/tule.obj");

    // Isla de la Ofrenda.
    Model Altar((char*)"Models/ofrenda.obj");
    Model Cempasuchil((char*)"Models/cempasuchil.obj");
    Model Candle((char*)"Models/veladora.obj");
    Model PapelPicado((char*)"Models/papel_picado.obj");
    Model PhotoFrame((char*)"Models/marco_fotografia.obj");
    Model Skull((char*)"Models/calavera.obj");

    // Isla de la Musica y la Convivencia.
    Model Stage((char*)"Models/escenario_musica.obj");
    Model Musician((char*)"Models/musico.obj");
    Model Table((char*)"Models/mesa.obj");
    Model Chair((char*)"Models/silla.obj");
    Model Person((char*)"Models/persona.obj");

    // Elementos que se mueven por los canales.
    Model Trajinera((char*)"Models/trajinera.obj");

    // Cielo / decoracion.
    Model Moon((char*)"Models/luna.obj");
    Model Rock((char*)"Models/piedra.obj");

    // ============================================================
    // VAO / VBO para representar visualmente las luces.
    // ============================================================

    GLuint lampVAO = 0, lampVBO = 0;
    glGenVertexArrays(1, &lampVAO);
    glGenBuffers(1, &lampVBO);

    glBindVertexArray(lampVAO);
    glBindBuffer(GL_ARRAY_BUFFER, lampVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(lampVertices),
                 lampVertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE,
                          6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE,
                          6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindVertexArray(0);

    // Inclinar inicialmente la camara hacia el centro de la escena.
    // Evita mirar horizontalmente por encima de las islas.
    camera.ProcessMouseMovement(0.0f, -100.0f); // ≈ -25° con sensibilidad 0.25

    // Proyeccion en perspectiva.
    glm::mat4 projection = glm::perspective(
        camera.GetZoom(),
        (float)SCREEN_WIDTH / (float)SCREEN_HEIGHT,
        0.1f, 150.0f);


    // Unidades de textura (se conserva la convencion del proyecto).
    lightingShader.Use();
    glUniform1i(glGetUniformLocation(lightingShader.Program, "material.diffuse"), 0);
    glUniform1i(glGetUniformLocation(lightingShader.Program, "material.specular"), 1);

    std::cout << "\n============================================\n";
    std::cout << " Xochimilco - Dia de Muertos\n";
    std::cout << "============================================\n";
    std::cout << "WASD / flechas : mover camara\n";
    std::cout << "Mouse          : mirar alrededor\n";
    std::cout << "ESPACIO        : encender/apagar luces\n";
    std::cout << "M              : mostrar/ocultar marcadores\n";
    std::cout << "P              : pausar/reanudar animaciones\n";
    std::cout << "R              : reiniciar recorrido\n";
    std::cout << "ESC            : salir\n\n";

    // ------------------------- Ciclo principal --------------------

    while (!glfwWindowShouldClose(window)) {
        GLfloat currentFrame = (GLfloat)glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        if (deltaTime > 0.05f) deltaTime = 0.05f;
        lastFrame = currentFrame;

        glfwPollEvents();
        DoMovement();
        UpdateScene(deltaTime);

        glClearColor(0.015f, 0.025f, 0.075f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glm::mat4 view = camera.GetViewMatrix();
        glm::vec3 cameraPosition = camera.GetPosition();

        lightingShader.Use();
        SetLighting(lightingShader, cameraPosition);

        glUniformMatrix4fv(
            glGetUniformLocation(lightingShader.Program, "view"),
            1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(
            glGetUniformLocation(lightingShader.Program, "projection"),
            1, GL_FALSE, glm::value_ptr(projection));

        glUniform1i(glGetUniformLocation(lightingShader.Program, "transparency"), 0);

        // ========================================================
        // CANAL PRINCIPAL
        // ========================================================

        DrawObject(lightingShader, Water,
                   glm::vec3(0.0f, -0.25f, 0.0f),
                   glm::vec3(22.0f, 1.0f, 19.0f));

        // ========================================================
        // ISLA 1: ISLA DE LAS MUNECAS (parte izquierda)
        // ========================================================

        DrawObject(lightingShader, Island,
                   glm::vec3(-5.0f, 0.0f, -3.0f),
                   glm::vec3(1.25f, 0.65f, 1.15f));
        DrawObject(lightingShader, IslandEdge,
                   glm::vec3(-5.0f, -0.05f, -3.0f),
                   glm::vec3(1.28f, 0.7f, 1.18f));

        DrawObject(lightingShader, Cabin,
                   glm::vec3(-5.4f, 0.45f, -3.1f),
                   glm::vec3(0.85f), 15.0f);

        // Arboles alrededor de la cabana.
        DrawObject(lightingShader, Tree, glm::vec3(-7.2f, 0.25f, -3.2f), glm::vec3(1.1f));
        DrawObject(lightingShader, Tree, glm::vec3(-3.6f, 0.25f, -3.6f), glm::vec3(0.9f));
        DrawObject(lightingShader, Tree, glm::vec3(-6.8f, 0.25f, -1.2f), glm::vec3(0.8f));
        DrawObject(lightingShader, Tree, glm::vec3(-3.7f, 0.25f, -1.3f), glm::vec3(1.0f));

        // Munecas distribuidas en la isla.
        DrawObject(lightingShader, HangingDoll, glm::vec3(-6.6f, 0.65f, -3.0f), glm::vec3(0.45f));
        DrawObject(lightingShader, HangingDoll, glm::vec3(-4.5f, 0.75f, -4.2f), glm::vec3(0.42f));
        DrawObject(lightingShader, HangingDoll, glm::vec3(-3.8f, 0.65f, -2.1f), glm::vec3(0.38f));
        DrawObject(lightingShader, HangingDoll, glm::vec3(-6.0f, 0.70f, -1.7f), glm::vec3(0.40f));
        DrawObject(lightingShader, HangingDoll, glm::vec3(-5.0f, 0.80f, -2.0f), glm::vec3(0.36f));

        DrawObject(lightingShader, Reeds, glm::vec3(-7.4f, 0.0f, -4.0f), glm::vec3(0.9f));
        DrawObject(lightingShader, Reeds, glm::vec3(-3.2f, 0.0f, -4.0f), glm::vec3(0.8f));

        // ========================================================
        // ISLA 2: ISLA DE LA OFRENDA (parte derecha)
        // ========================================================

        DrawObject(lightingShader, Island,
                   glm::vec3(4.7f, 0.0f, -3.0f),
                   glm::vec3(1.25f, 0.65f, 1.15f));
        DrawObject(lightingShader, IslandEdge,
                   glm::vec3(4.7f, -0.05f, -3.0f),
                   glm::vec3(1.28f, 0.7f, 1.18f));

        DrawObject(lightingShader, Altar,
                   glm::vec3(4.7f, 0.40f, -3.0f),
                   glm::vec3(1.0f));

        // Flores de cempasuchil alrededor de la ofrenda.
        DrawObject(lightingShader, Cempasuchil, glm::vec3(3.0f, 0.28f, -4.1f), glm::vec3(0.55f));
        DrawObject(lightingShader, Cempasuchil, glm::vec3(4.0f, 0.28f, -4.5f), glm::vec3(0.55f));
        DrawObject(lightingShader, Cempasuchil, glm::vec3(5.3f, 0.28f, -4.5f), glm::vec3(0.55f));
        DrawObject(lightingShader, Cempasuchil, glm::vec3(6.2f, 0.28f, -3.7f), glm::vec3(0.55f));
        DrawObject(lightingShader, Cempasuchil, glm::vec3(3.2f, 0.28f, -2.0f), glm::vec3(0.50f));
        DrawObject(lightingShader, Cempasuchil, glm::vec3(6.0f, 0.28f, -2.0f), glm::vec3(0.50f));

        DrawObject(lightingShader, Candle, glm::vec3(3.6f, 0.45f, -3.3f), glm::vec3(0.35f));
        DrawObject(lightingShader, Candle, glm::vec3(5.8f, 0.45f, -3.3f), glm::vec3(0.35f));
        DrawObject(lightingShader, Candle, glm::vec3(4.7f, 0.45f, -4.0f), glm::vec3(0.35f));
        DrawObject(lightingShader, PhotoFrame, glm::vec3(4.7f, 1.15f, -3.3f), glm::vec3(0.45f));
        DrawObject(lightingShader, Skull, glm::vec3(4.0f, 0.55f, -2.8f), glm::vec3(0.35f));
        DrawObject(lightingShader, Skull, glm::vec3(5.4f, 0.55f, -2.8f), glm::vec3(0.35f));

        // Papel picado: rotacion oscilatoria simulando viento.
        DrawObject(lightingShader, PapelPicado,
                   glm::vec3(4.7f, 1.9f, -4.5f),
                   glm::vec3(1.1f),
                   papelPicadoAngle);

        // ========================================================
        // ISLA 3: MUSICA Y CONVIVENCIA (parte frontal)
        // ========================================================

        DrawObject(lightingShader, Island,
                   glm::vec3(0.0f, 0.0f, 5.0f),
                   glm::vec3(1.35f, 0.65f, 1.1f));
        DrawObject(lightingShader, IslandEdge,
                   glm::vec3(0.0f, -0.05f, 5.0f),
                   glm::vec3(1.38f, 0.7f, 1.13f));

        DrawObject(lightingShader, Stage,
                   glm::vec3(0.0f, 0.38f, 4.0f),
                   glm::vec3(0.95f));

        // Musicos en el escenario.
        DrawObject(lightingShader, Musician, glm::vec3(-1.0f, 0.45f, 3.7f), glm::vec3(0.48f));
        DrawObject(lightingShader, Musician, glm::vec3(-0.35f, 0.45f, 3.7f), glm::vec3(0.48f));
        DrawObject(lightingShader, Musician, glm::vec3( 0.35f, 0.45f, 3.7f), glm::vec3(0.48f));
        DrawObject(lightingShader, Musician, glm::vec3( 1.0f, 0.45f, 3.7f), glm::vec3(0.48f));

        // Mesas y visitantes.
        DrawObject(lightingShader, Table, glm::vec3(-1.5f, 0.35f, 5.8f), glm::vec3(0.65f));
        DrawObject(lightingShader, Table, glm::vec3( 0.0f, 0.35f, 6.2f), glm::vec3(0.65f));
        DrawObject(lightingShader, Table, glm::vec3( 1.5f, 0.35f, 5.8f), glm::vec3(0.65f));

        DrawObject(lightingShader, Chair, glm::vec3(-2.0f, 0.25f, 5.8f), glm::vec3(0.42f));
        DrawObject(lightingShader, Chair, glm::vec3(-1.0f, 0.25f, 6.3f), glm::vec3(0.42f));
        DrawObject(lightingShader, Chair, glm::vec3( 0.5f, 0.25f, 6.4f), glm::vec3(0.42f));
        DrawObject(lightingShader, Chair, glm::vec3( 1.9f, 0.25f, 5.8f), glm::vec3(0.42f));

        DrawObject(lightingShader, Person, glm::vec3(-2.2f, 0.35f, 4.8f), glm::vec3(0.45f));
        DrawObject(lightingShader, Person, glm::vec3(-0.8f, 0.35f, 6.7f), glm::vec3(0.45f));
        DrawObject(lightingShader, Person, glm::vec3( 0.8f, 0.35f, 6.7f), glm::vec3(0.45f));
        DrawObject(lightingShader, Person, glm::vec3( 2.0f, 0.35f, 4.8f), glm::vec3(0.45f));

        DrawObject(lightingShader, PapelPicado,
                   glm::vec3(0.0f, 1.8f, 3.2f),
                   glm::vec3(1.0f), -papelPicadoAngle);

        // ========================================================
        // DECORACION GENERAL Y PUENTES
        // ========================================================

        DrawObject(lightingShader, Bridge, glm::vec3(-2.0f, 0.0f, 0.0f),
                   glm::vec3(0.8f), 90.0f);
        DrawObject(lightingShader, Bridge, glm::vec3(2.0f, 0.0f, 0.0f),
                   glm::vec3(0.8f), -90.0f);

        DrawObject(lightingShader, Moon, glm::vec3(0.0f, 8.0f, -8.0f),
                   glm::vec3(1.2f));

        DrawObject(lightingShader, Rock, glm::vec3(-8.5f, -0.05f, 5.0f), glm::vec3(0.8f));
        DrawObject(lightingShader, Rock, glm::vec3( 8.5f, -0.05f, 4.0f), glm::vec3(0.8f));

        // Veladoras visibles en las tres islas.
        DrawObject(lightingShader, Candle, glm::vec3(-5.0f, 0.45f, -1.6f), glm::vec3(0.28f));
        DrawObject(lightingShader, Candle, glm::vec3(-6.3f, 0.45f, -4.0f), glm::vec3(0.28f));
        DrawObject(lightingShader, Candle, glm::vec3( 0.0f, 0.45f,  4.8f), glm::vec3(0.28f));
        DrawObject(lightingShader, Candle, glm::vec3( 1.5f, 0.45f,  5.5f), glm::vec3(0.28f));

        // ========================================================
        // TRAJINERAS ANIMADAS POR KEYFRAMES
        // ========================================================

        DrawObject(lightingShader, Trajinera,
                   boatPosition, glm::vec3(0.85f), boatRotation);

        DrawObject(lightingShader, Trajinera,
                   boat2Position, glm::vec3(0.75f), boat2Rotation);

        // ========================================================
        // MARCADORES VISUALES DE LAS FUENTES DE LUZ
        // ========================================================

        if (showLightMarkers) {
            DrawLamp(lampShader, lampVAO, candlePositions[0], candleColors[0], 0.18f);
            DrawLamp(lampShader, lampVAO, candlePositions[1], candleColors[1], 0.18f);
            DrawLamp(lampShader, lampVAO, candlePositions[2], candleColors[2], 0.18f);
            DrawLamp(lampShader, lampVAO, candlePositions[3], candleColors[3], 0.18f);
        }

        glfwSwapBuffers(window);
    }

    glDeleteVertexArrays(1, &lampVAO);
    glDeleteBuffers(1, &lampVBO);

    glfwTerminate();
    return EXIT_SUCCESS;
}
