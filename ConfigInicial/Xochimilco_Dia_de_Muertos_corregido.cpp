// ================================================================
// Xochimilco - Dia de Muertos (Sin transparencias, puentes y rutas fijas)
// 318041267
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

Camera camera(glm::vec3(0.0f, 8.0f, 18.5f));
GLfloat lastX = WIDTH / 2.0f;
GLfloat lastY = HEIGHT / 2.0f;
bool keys[1024] = { false };
bool firstMouse = true;

GLfloat deltaTime = 0.0f;
GLfloat lastFrame = 0.0f;

// ------------------------- Escena ---------------------------------

bool showLightMarkers = false;
bool nightLights = true;
bool animationEnabled = true;

// 4 fuentes de luz principales sobre las islas y el canal
glm::vec3 candlePositions[4] = {
    glm::vec3(-5.4f, 2.3f, -3.2f), // Isla de las Munecas
    glm::vec3(5.4f, 2.3f, -3.2f), // Isla de la Ofrenda
    glm::vec3(0.0f, 2.5f,  5.2f), // Isla de la Musica
    glm::vec3(0.0f, 6.0f,  0.0f)  // Luz central sobre los puentes
};

glm::vec3 candleColors[4] = {
    glm::vec3(1.35f, 0.55f, 0.15f), // Ambar (Munecas)
    glm::vec3(1.80f, 0.85f, 0.20f), // Dorado cempasuchil (Ofrenda)
    glm::vec3(1.50f, 0.45f, 0.85f), // Rosa festivo (Musica)
    glm::vec3(0.45f, 0.65f, 1.35f)  // Azul lunar (Canal)
};

// Posiciones de las veladoras fisicas
const int TOTAL_CANDLES = 9;
glm::vec3 allCandleProps[TOTAL_CANDLES] = {
    // Isla Ofrenda
    glm::vec3(4.2f, 0.45f, -3.5f),
    glm::vec3(6.6f, 0.45f, -3.5f),
    glm::vec3(5.4f, 0.45f, -4.2f),
    glm::vec3(4.8f, 0.85f, -3.2f),
    glm::vec3(6.0f, 0.85f, -3.2f),
    // Isla Munecas
    glm::vec3(-5.2f, 0.45f, -1.8f),
    glm::vec3(-6.5f, 0.45f, -4.2f),
    // Isla Musica
    glm::vec3(0.0f, 0.45f,  5.0f),
    glm::vec3(1.5f, 0.45f,  5.7f)
};

// ------------------------- Keyframes ------------------------------

struct BOAT_FRAME {
    glm::vec3 position;
    float rotationY;
    float duration;
};

const int BOAT_FRAME_COUNT = 6;

// Circuito perimetral amplio por fuera de las 3 islas (sin colisiones)
BOAT_FRAME boatPath[BOAT_FRAME_COUNT] = {
    { glm::vec3(-10.5f, 0.08f,  1.5f), -52.0f, 3.8f },
    { glm::vec3(-4.5f, 0.08f,  9.5f),   0.0f, 3.8f },
    { glm::vec3(4.5f, 0.08f,  9.5f),  52.0f, 3.8f },
    { glm::vec3(10.5f, 0.08f,  1.5f),  98.0f, 3.8f },
    { glm::vec3(9.2f, 0.08f, -7.5f), 180.0f, 3.8f },
    { glm::vec3(-9.2f, 0.08f, -7.5f), 262.0f, 3.8f }
};

glm::vec3 boatPosition = boatPath[0].position;
float boatRotation = boatPath[0].rotationY;
int boatFrameIndex = 0;
float boatFrameTime = 0.0f;

// La segunda trajinera inicia en el extremo opuesto del circuito (keyframe 3)
glm::vec3 boat2Position = boatPath[3].position;
float boat2Rotation = boatPath[3].rotationY;

enum SceneState {
    SCENE_INTRO,
    SCENE_BOATS_MOVING,
    SCENE_BOATS_PAUSED,
    SCENE_FINALE
};

SceneState sceneState = SCENE_INTRO;
float stateTimer = 0.0f;
float sceneTime = 0.0f;

float papelPicadoAngle = 0.0f;
float waterOffset = 0.0f;

// ------------------------- Cubo de luz ----------------------------

float lampVertices[] = {
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
    float rotationY = 0.0f,
    float rotationZ = 0.0f) {
    glm::mat4 model(1.0f);
    model = glm::translate(model, position);
    model = glm::rotate(model, glm::radians(rotationY), glm::vec3(0, 1, 0));
    if (rotationZ != 0.0f)
        model = glm::rotate(model, glm::radians(rotationZ), glm::vec3(0, 0, 1));
    model = glm::scale(model, scale);
    return model;
}

static void DrawObject(Shader& shader, Model& object,
    const glm::vec3& position,
    const glm::vec3& scale,
    float rotationY = 0.0f,
    float rotationZ = 0.0f) {
    glm::mat4 model = Transform(position, scale, rotationY, rotationZ);
    glUniformMatrix4fv(glGetUniformLocation(shader.Program, "model"),
        1, GL_FALSE, glm::value_ptr(model));
    object.Draw(shader);
}

// Interpola angulos por el camino mas corto evitando giros bruscos de 360 grados
static float LerpAngleDegrees(float a, float b, float t) {
    float diff = fmod(b - a + 540.0f, 360.0f) - 180.0f;
    return a + diff * t;
}

// ------------------------- Iluminación ----------------------------

void SetLighting(Shader& shader, const glm::vec3& viewPosition) {
    shader.Use();

    glUniform3f(glGetUniformLocation(shader.Program, "viewPos"),
        viewPosition.x, viewPosition.y, viewPosition.z);

    // Luz direccional nocturna clara
    glUniform3f(glGetUniformLocation(shader.Program, "dirLight.direction"),
        -0.25f, -1.0f, -0.3f);
    glUniform3f(glGetUniformLocation(shader.Program, "dirLight.ambient"),
        0.28f, 0.32f, 0.46f);
    glUniform3f(glGetUniformLocation(shader.Program, "dirLight.diffuse"),
        0.50f, 0.56f, 0.78f);
    glUniform3f(glGetUniformLocation(shader.Program, "dirLight.specular"),
        0.55f, 0.60f, 0.80f);

    // 4 PointLights con baja atenuacion para cubrir todas las islas
    for (int i = 0; i < 4; ++i) {
        std::string base = "pointLights[" + std::to_string(i) + "].";
        float flicker = 0.88f + 0.12f * sin(sceneTime * (4.5f + i) + i * 1.7f);

        glm::vec3 color = nightLights ? candleColors[i] * flicker
            : glm::vec3(0.05f);

        glUniform3f(glGetUniformLocation(shader.Program, (base + "position").c_str()),
            candlePositions[i].x, candlePositions[i].y, candlePositions[i].z);
        glUniform3f(glGetUniformLocation(shader.Program, (base + "ambient").c_str()),
            color.r * 0.28f, color.g * 0.28f, color.b * 0.28f);
        glUniform3f(glGetUniformLocation(shader.Program, (base + "diffuse").c_str()),
            color.r, color.g, color.b);
        glUniform3f(glGetUniformLocation(shader.Program, (base + "specular").c_str()),
            color.r, color.g, color.b);
        glUniform1f(glGetUniformLocation(shader.Program, (base + "constant").c_str()), 1.0f);
        glUniform1f(glGetUniformLocation(shader.Program, (base + "linear").c_str()), 0.045f);
        glUniform1f(glGetUniformLocation(shader.Program, (base + "quadratic").c_str()), 0.0075f);
    }

    // Linterna de camara de largo alcance
    glUniform3f(glGetUniformLocation(shader.Program, "spotLight.position"),
        viewPosition.x, viewPosition.y, viewPosition.z);
    glUniform3f(glGetUniformLocation(shader.Program, "spotLight.direction"),
        camera.GetFront().x, camera.GetFront().y, camera.GetFront().z);
    glUniform3f(glGetUniformLocation(shader.Program, "spotLight.ambient"),
        0.08f, 0.08f, 0.10f);
    glUniform3f(glGetUniformLocation(shader.Program, "spotLight.diffuse"),
        0.85f, 0.80f, 0.70f);
    glUniform3f(glGetUniformLocation(shader.Program, "spotLight.specular"),
        0.90f, 0.90f, 0.95f);
    glUniform1f(glGetUniformLocation(shader.Program, "spotLight.constant"), 1.0f);
    glUniform1f(glGetUniformLocation(shader.Program, "spotLight.linear"), 0.022f);
    glUniform1f(glGetUniformLocation(shader.Program, "spotLight.quadratic"), 0.0019f);
    glUniform1f(glGetUniformLocation(shader.Program, "spotLight.cutOff"),
        glm::cos(glm::radians(15.0f)));
    glUniform1f(glGetUniformLocation(shader.Program, "spotLight.outerCutOff"),
        glm::cos(glm::radians(24.0f)));

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
        if (stateTimer >= 0.5f) {
            sceneState = SCENE_BOATS_MOVING;
            stateTimer = 0.0f;
        }
    }
    else if (sceneState == SCENE_BOATS_MOVING) {
        boatFrameTime += dt;

        float duration = boatPath[boatFrameIndex].duration;
        float t = glm::clamp(boatFrameTime / duration, 0.0f, 1.0f);

        // --- TRAJINERA 1 ---
        int nextIndex = (boatFrameIndex + 1) % BOAT_FRAME_COUNT;
        boatPosition = glm::mix(boatPath[boatFrameIndex].position,
            boatPath[nextIndex].position, t);
        boatRotation = LerpAngleDegrees(boatPath[boatFrameIndex].rotationY,
            boatPath[nextIndex].rotationY, t);

        // --- TRAJINERA 2 (Desfasada 3 keyframes = lado opuesto del canal, nunca chocan) ---
        int idx2 = (boatFrameIndex + 3) % BOAT_FRAME_COUNT;
        int nextIdx2 = (idx2 + 1) % BOAT_FRAME_COUNT;
        boat2Position = glm::mix(boatPath[idx2].position,
            boatPath[nextIdx2].position, t);
        boat2Rotation = LerpAngleDegrees(boatPath[idx2].rotationY,
            boatPath[nextIdx2].rotationY, t);

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
        if (stateTimer >= 1.0f) {
            sceneState = SCENE_BOATS_MOVING;
            stateTimer = 0.0f;
        }
    }
}

void UpdateScene(float dt) {
    sceneTime += dt;
    waterOffset += dt * 0.8f;
    papelPicadoAngle = 6.0f * sin(sceneTime * 2.5f);
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
            boat2Position = boatPath[3].position;
            boat2Rotation = boatPath[3].rotationY;
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

    // El agua activa GL_BLEND solo durante su dibujo; el resto de la escena
    // permanece opaca para no mezclar indebidamente islas ni modelos.
    glDisable(GL_BLEND);

    Shader lightingShader("Shader/lighting.vs", "Shader/lighting.frag");
    Shader lampShader("Shader/lamp.vs", "Shader/lamp.frag");

    // Carga de modelos
    Model Water((char*)"Models/agua.obj");
    Model Island((char*)"Models/isla.obj");
    Model IslandEdge((char*)"Models/borde_isla.obj");
    Model Bridge((char*)"Models/puente_madera.obj");

    Model Tree((char*)"Models/arbol.obj");
    Model HangingDoll((char*)"Models/muneca_colgante.obj");
    Model Cabin((char*)"Models/cabana.obj");
    Model Reeds((char*)"Models/tule.obj");

    Model Altar((char*)"Models/ofrenda.obj");
    Model Cempasuchil((char*)"Models/cempasuchil.obj");
    Model Candle((char*)"Models/veladora.obj");
    Model PapelPicado((char*)"Models/papel_picado.obj");
    Model PhotoFrame((char*)"Models/marco_fotografia.obj");
    Model Skull((char*)"Models/calavera.obj");

    Model Stage((char*)"Models/escenario_musica.obj");
    Model Musician((char*)"Models/musico.obj");
    Model Table((char*)"Models/mesa.obj");
    Model Chair((char*)"Models/silla.obj");
    Model Person((char*)"Models/persona.obj");

    Model Trajinera((char*)"Models/trajinera.obj");
    Model Moon((char*)"Models/luna.obj");
    Model Rock((char*)"Models/piedra.obj");

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

    camera.ProcessMouseMovement(0.0f, -75.0f);

    glm::mat4 projection = glm::perspective(
        camera.GetZoom(),
        (float)SCREEN_WIDTH / (float)SCREEN_HEIGHT,
        0.1f, 150.0f);

    lightingShader.Use();
    glUniform1i(glGetUniformLocation(lightingShader.Program, "material.diffuse"), 0);
    glUniform1i(glGetUniformLocation(lightingShader.Program, "material.specular"), 1);

    // ------------------------- Ciclo principal --------------------

    while (!glfwWindowShouldClose(window)) {
        GLfloat currentFrame = (GLfloat)glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        if (deltaTime > 0.05f) deltaTime = 0.05f;
        lastFrame = currentFrame;

        glfwPollEvents();
        DoMovement();
        UpdateScene(deltaTime);

        glClearColor(0.04f, 0.06f, 0.14f, 1.0f);
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

        float waveY = -0.22f + 0.02f * sin(waterOffset * 1.5f);
        float waveX = 0.12f * cos(waterOffset * 0.7f);

        // Material de agua de Xochimilco: equivalente al Principled BSDF
        // configurado en Blender. El fragment shader genera el Noise -> Bump.
        glUniform1i(glGetUniformLocation(lightingShader.Program, "isWater"), 1);
        glUniform1f(glGetUniformLocation(lightingShader.Program, "waterTime"), waterOffset);
        glUniform3f(glGetUniformLocation(lightingShader.Program, "waterBaseColor"),
            0.015f, 0.12f, 0.135f); // verde-azulado profundo y turbio
        glUniform1f(glGetUniformLocation(lightingShader.Program, "waterTransmission"), 1.0f);
        glUniform1f(glGetUniformLocation(lightingShader.Program, "waterIor"), 1.333f);
        glUniform1f(glGetUniformLocation(lightingShader.Program, "waterRoughness"), 0.05f);
        glUniform1f(glGetUniformLocation(lightingShader.Program, "waterNoiseScale"), 30.0f);
        glUniform1f(glGetUniformLocation(lightingShader.Program, "waterNoiseDetail"), 5.0f);
        glUniform1f(glGetUniformLocation(lightingShader.Program, "waterBumpStrength"), 0.15f);
        glUniform1f(glGetUniformLocation(lightingShader.Program, "waterBumpDistance"), 0.2f);

        // La transparencia se aplica solo al canal: no escribe profundidad,
        // asi las islas y los objetos se dibujan correctamente encima de ella.
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDepthMask(GL_FALSE);
        DrawObject(lightingShader, Water,
            glm::vec3(waveX, waveY, 0.0f),
            glm::vec3(28.0f, 1.0f, 26.0f));
        glDepthMask(GL_TRUE);
        glDisable(GL_BLEND);
        glUniform1i(glGetUniformLocation(lightingShader.Program, "isWater"), 0);

        // ========================================================
        // ISLA 1: ISLA DE LAS MUNECAS (Izquierda: centro en -5.4, -3.2)
        // ========================================================

        DrawObject(lightingShader, Island,
            glm::vec3(-5.4f, 0.0f, -3.2f),
            glm::vec3(2.5f, 0.75f, 2.4f));
        DrawObject(lightingShader, IslandEdge,
            glm::vec3(-5.4f, -0.05f, -3.2f),
            glm::vec3(2.58f, 0.8f, 2.48f));

        DrawObject(lightingShader, Cabin,
            glm::vec3(-5.6f, 0.45f, -3.4f),
            glm::vec3(0.95f), 20.0f);

        DrawObject(lightingShader, Tree, glm::vec3(-7.1f, 0.30f, -4.3f), glm::vec3(1.20f), 15.0f);
        DrawObject(lightingShader, Tree, glm::vec3(-3.8f, 0.30f, -4.5f), glm::vec3(1.05f), 45.0f);
        DrawObject(lightingShader, Tree, glm::vec3(-6.9f, 0.30f, -1.8f), glm::vec3(1.00f), 90.0f);
        DrawObject(lightingShader, Tree, glm::vec3(-4.0f, 0.30f, -1.7f), glm::vec3(1.10f), 130.0f);
        DrawObject(lightingShader, Tree, glm::vec3(-5.5f, 0.30f, -5.0f), glm::vec3(1.25f), 210.0f);

        float dollSwing = 4.0f * sin(sceneTime * 1.8f);
        DrawObject(lightingShader, HangingDoll, glm::vec3(-6.6f, 0.75f, -3.2f), glm::vec3(0.45f), 15.0f, dollSwing);
        DrawObject(lightingShader, HangingDoll, glm::vec3(-4.7f, 0.80f, -4.2f), glm::vec3(0.42f), -25.0f, -dollSwing);
        DrawObject(lightingShader, HangingDoll, glm::vec3(-4.0f, 0.75f, -2.2f), glm::vec3(0.40f), 40.0f, dollSwing);
        DrawObject(lightingShader, HangingDoll, glm::vec3(-6.2f, 0.75f, -2.0f), glm::vec3(0.42f), -10.0f, -dollSwing);
        DrawObject(lightingShader, HangingDoll, glm::vec3(-5.2f, 0.85f, -2.2f), glm::vec3(0.38f), 0.0f, dollSwing);

        DrawObject(lightingShader, Reeds, glm::vec3(-7.8f, 0.0f, -3.8f), glm::vec3(1.0f));
        DrawObject(lightingShader, Reeds, glm::vec3(-7.4f, 0.0f, -1.6f), glm::vec3(0.95f), 45.0f);

        // ========================================================
        // ISLA 2: ISLA DE LA OFRENDA (Derecha: centro en 5.4, -3.2)
        // ========================================================

        DrawObject(lightingShader, Island,
            glm::vec3(5.4f, 0.0f, -3.2f),
            glm::vec3(2.5f, 0.75f, 2.4f));
        DrawObject(lightingShader, IslandEdge,
            glm::vec3(5.4f, -0.05f, -3.2f),
            glm::vec3(2.58f, 0.8f, 2.48f));

        DrawObject(lightingShader, Altar,
            glm::vec3(5.4f, 0.42f, -3.2f),
            glm::vec3(1.15f));

        DrawObject(lightingShader, Tree, glm::vec3(3.8f, 0.30f, -4.9f), glm::vec3(1.1f));
        DrawObject(lightingShader, Tree, glm::vec3(7.0f, 0.30f, -4.9f), glm::vec3(1.15f), 60.0f);

        for (int i = 0; i < 8; ++i) {
            float angle = i * (2.0f * 3.14159f / 8.0f);
            glm::vec3 flowerPos(5.4f + 1.75f * cos(angle), 0.32f, -3.2f + 1.55f * sin(angle));
            DrawObject(lightingShader, Cempasuchil, flowerPos, glm::vec3(0.60f), glm::degrees(angle));
        }

        DrawObject(lightingShader, PhotoFrame, glm::vec3(5.4f, 1.25f, -3.4f), glm::vec3(0.50f));
        DrawObject(lightingShader, Skull, glm::vec3(4.7f, 0.60f, -2.8f), glm::vec3(0.38f), -20.0f);
        DrawObject(lightingShader, Skull, glm::vec3(6.1f, 0.60f, -2.8f), glm::vec3(0.38f), 20.0f);

        DrawObject(lightingShader, PapelPicado,
            glm::vec3(5.4f, 2.1f, -4.5f),
            glm::vec3(1.25f),
            papelPicadoAngle);

        DrawObject(lightingShader, Reeds, glm::vec3(7.8f, 0.0f, -3.5f), glm::vec3(1.0f));
        DrawObject(lightingShader, Reeds, glm::vec3(7.4f, 0.0f, -1.6f), glm::vec3(0.9f), 30.0f);

        // ========================================================
        // ISLA 3: MUSICA Y CONVIVENCIA (Frontal: centro en 0.0, 5.2)
        // ========================================================

        DrawObject(lightingShader, Island,
            glm::vec3(0.0f, 0.0f, 5.2f),
            glm::vec3(2.8f, 0.75f, 2.5f));
        DrawObject(lightingShader, IslandEdge,
            glm::vec3(0.0f, -0.05f, 5.2f),
            glm::vec3(2.88f, 0.8f, 2.58f));

        DrawObject(lightingShader, Stage,
            glm::vec3(0.0f, 0.40f, 4.0f),
            glm::vec3(1.0f));

        float musicBob = 6.0f * sin(sceneTime * 5.0f);
        DrawObject(lightingShader, Musician, glm::vec3(-1.05f, 0.48f, 4.0f), glm::vec3(0.50f), musicBob);
        DrawObject(lightingShader, Musician, glm::vec3(-0.35f, 0.48f, 4.0f), glm::vec3(0.50f), -musicBob);
        DrawObject(lightingShader, Musician, glm::vec3(0.35f, 0.48f, 4.0f), glm::vec3(0.50f), musicBob);
        DrawObject(lightingShader, Musician, glm::vec3(1.05f, 0.48f, 4.0f), glm::vec3(0.50f), -musicBob);

        DrawObject(lightingShader, Table, glm::vec3(-1.5f, 0.38f, 5.8f), glm::vec3(0.65f));
        DrawObject(lightingShader, Table, glm::vec3(0.0f, 0.38f, 6.3f), glm::vec3(0.65f));
        DrawObject(lightingShader, Table, glm::vec3(1.5f, 0.38f, 5.8f), glm::vec3(0.65f));

        DrawObject(lightingShader, Chair, glm::vec3(-2.0f, 0.30f, 5.8f), glm::vec3(0.45f), 90.0f);
        DrawObject(lightingShader, Chair, glm::vec3(-1.0f, 0.30f, 6.3f), glm::vec3(0.45f), 180.0f);
        DrawObject(lightingShader, Chair, glm::vec3(0.5f, 0.30f, 6.7f), glm::vec3(0.45f), 180.0f);
        DrawObject(lightingShader, Chair, glm::vec3(2.0f, 0.30f, 5.8f), glm::vec3(0.45f), -90.0f);

        DrawObject(lightingShader, Person, glm::vec3(-2.0f, 0.38f, 5.0f), glm::vec3(0.48f), 25.0f);
        DrawObject(lightingShader, Person, glm::vec3(-0.7f, 0.38f, 6.8f), glm::vec3(0.48f), 160.0f);
        DrawObject(lightingShader, Person, glm::vec3(0.7f, 0.38f, 6.8f), glm::vec3(0.48f), -160.0f);
        DrawObject(lightingShader, Person, glm::vec3(2.0f, 0.38f, 5.0f), glm::vec3(0.48f), -25.0f);

        // Arboles colocados a los costados para dejar libre la llegada de los puentes
        DrawObject(lightingShader, Tree, glm::vec3(-2.7f, 0.30f, 5.6f), glm::vec3(0.95f));
        DrawObject(lightingShader, Tree, glm::vec3(2.7f, 0.30f, 5.6f), glm::vec3(0.95f), 90.0f);
        DrawObject(lightingShader, Cempasuchil, glm::vec3(-1.7f, 0.32f, 4.3f), glm::vec3(0.55f));
        DrawObject(lightingShader, Cempasuchil, glm::vec3(1.7f, 0.32f, 4.3f), glm::vec3(0.55f));

        DrawObject(lightingShader, PapelPicado,
            glm::vec3(0.0f, 2.0f, 3.5f),
            glm::vec3(1.15f), -papelPicadoAngle);

        // ========================================================
        // PUENTES ALINEADOS ENTRE LAS ORILLAS DE LAS 3 ISLAS
        // ========================================================

        // 1. Puente superior horizontal: conecta Isla 1 (-5.4, -3.2) con Isla 2 (5.4, -3.2)
        DrawObject(lightingShader, Bridge,
            glm::vec3(0.0f, 0.28f, -3.2f),
            glm::vec3(1.85f, 1.0f, 1.15f), 0.0f);

        // 2. Puente diagonal izquierdo: conecta Isla 1 (-4.2, -0.6) con Isla 3 (-2.1, 3.6) al lado del escenario
        DrawObject(lightingShader, Bridge,
            glm::vec3(-3.15f, 0.28f, 1.5f),
            glm::vec3(1.75f, 1.0f, 1.15f), -63.4f);

        // 3. Puente diagonal derecho: conecta Isla 2 (4.2, -0.6) con Isla 3 (2.1, 3.6) al lado del escenario
        DrawObject(lightingShader, Bridge,
            glm::vec3(3.15f, 0.28f, 1.5f),
            glm::vec3(1.75f, 1.0f, 1.15f), 63.4f);

        // Luna y decoracion exterior (fuera del carril de las trajineras)
        DrawObject(lightingShader, Moon, glm::vec3(0.0f, 10.5f, -11.0f),
            glm::vec3(1.6f));

        DrawObject(lightingShader, Rock, glm::vec3(-7.6f, -0.05f, 0.2f), glm::vec3(0.9f));
        DrawObject(lightingShader, Rock, glm::vec3(7.6f, -0.05f, 0.2f), glm::vec3(0.9f), 45.0f);

        for (int i = 0; i < TOTAL_CANDLES; ++i) {
            DrawObject(lightingShader, Candle, allCandleProps[i], glm::vec3(0.35f));
        }

        // ========================================================
        // TRAJINERAS NAVEGANDO EN CARRUSEL (OPUESTAS 180 GRADOS)
        // ========================================================

        float boatRock1 = 2.0f * sin(sceneTime * 3.0f);
        float boatRock2 = 2.0f * cos(sceneTime * 2.7f);

        DrawObject(lightingShader, Trajinera,
            boatPosition, glm::vec3(0.95f), boatRotation, boatRock1);

        DrawObject(lightingShader, Trajinera,
            boat2Position, glm::vec3(0.95f), boat2Rotation, boatRock2);

        // ========================================================
        // FLAMAS EN VELADORAS Y MARCADORES
        // ========================================================

        if (nightLights) {
            for (int i = 0; i < TOTAL_CANDLES; ++i) {
                float flameFlicker = 0.055f + 0.012f * sin(sceneTime * 8.0f + i);
                glm::vec3 flamePos = allCandleProps[i] + glm::vec3(0.0f, 0.22f, 0.0f);
                DrawLamp(lampShader, lampVAO, flamePos, glm::vec3(1.0f, 0.72f, 0.22f), flameFlicker);
            }
        }

        if (showLightMarkers) {
            for (int i = 0; i < 4; ++i) {
                DrawLamp(lampShader, lampVAO, candlePositions[i], candleColors[i], 0.20f);
            }
        }

        glfwSwapBuffers(window);
    }

    glDeleteVertexArrays(1, &lampVAO);
    glDeleteBuffers(1, &lampVBO);

    glfwTerminate();
    return EXIT_SUCCESS;
}
