#pragma once

#include <string>
#include <vector>

#include <GL/glew.h>
#include <glm/glm.hpp>

#include "LightManager.h"
#include "SceneManager.h"
#include "Shader.h"

class Renderer {
public:
    Renderer(int width, int height);
    ~Renderer();

    void Resize(int width, int height);
    void Draw(SceneManager& scene, const LightManager& lights, float timeSeconds);

private:
    void DrawObject(Shader& shader, Model& object, const glm::vec3& position,
        const glm::vec3& scale, float rotationY = 0.0f, float rotationZ = 0.0f);
    void DrawObjectWithMatrix(Shader& shader, Model& object, const glm::mat4& model);
    void DrawObjectWithParent(Shader& shader, Model& object, const glm::mat4& parent,
        const glm::vec3& localPosition, const glm::vec3& scale,
        float rotationY = 0.0f, float rotationZ = 0.0f);
    void DrawWater(SceneManager& scene, const LightManager& lights, float timeSeconds);
    void DrawLamp(const glm::vec3& position, const glm::vec3& color, float scale);
    void DrawBoatWithCrew(SceneManager& scene, const glm::vec3& position,
        float rotationY, float pitch, float roll, float bank, float scale, float phase);
    void DrawMariachiBand(SceneManager& scene, const glm::mat4& parent);
    void DrawSugarSkulls(SceneManager& scene, const glm::mat4& parent);
    void DrawSpiritParticles(float sceneTime);
    void DrawWorld(SceneManager& scene, const LightManager& lights, float timeSeconds);
    void DrawSkybox(SceneManager& scene, float timeSeconds);
    void DrawGenericIslands(SceneManager& scene);
    void DrawOfrendaOverlay(const SceneManager& scene);
    void DrawControlsOverlay(const SceneManager& scene);
    GLuint LoadCubemap(const std::vector<std::string>& faces);

    int width_;
    int height_;
    glm::mat4 worldRoot_;
    glm::mat4 projection_;
    Shader lightingShader_;
    Shader lampShader_;
    Shader waterShader_;
    Shader uiShader_;
    Shader skyboxShader_;
    GLuint lampVAO_;
    GLuint lampVBO_;
    GLuint uiVAO_;
    GLuint uiVBO_;
    GLuint skyboxVAO_;
    GLuint skyboxVBO_;
    GLuint skyboxTexture_;
};
