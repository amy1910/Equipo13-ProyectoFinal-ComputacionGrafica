#include "LightManager.h"
#include "SceneManager.h"

#include <cmath>
#include <string>

#include <glm/gtc/constants.hpp>

namespace {
constexpr float kWorldScale = 3.0f;
}

LightManager::LightManager()
    : pointPositions_{
        glm::vec3(5.4f, 1.2f, -3.2f), glm::vec3(4.5f, 1.1f, -3.1f),
        glm::vec3(6.3f, 1.1f, -3.3f),
        glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f),
        glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f),
        glm::vec3(-1.35f, 1.80f, 14.00f), glm::vec3(1.35f, 1.80f, 14.00f),
        glm::vec3(-14.0f, 1.4f, -11.0f), glm::vec3(14.0f, 1.4f, -11.0f),
        // Isla de las Muñecas: cool spectral fill beside the cabin entrance.
        glm::vec3(-5.4f, 1.55f, -2.35f),
        // Isla de la Convivencia: warm front light above the mariachi stage.
        glm::vec3(0.0f, 1.95f, 3.90f),
        // Isla de la ofrenda: warm frontal fill for Finado and Moribunda.
        glm::vec3(2.70f, 1.45f, -4.00f)},
      pointColors_{
        glm::vec3(1.05f, 0.42f, 0.08f), glm::vec3(0.95f, 0.55f, 0.16f),
        glm::vec3(0.95f, 0.55f, 0.16f), glm::vec3(1.00f, 0.52f, 0.14f),
        glm::vec3(1.00f, 0.52f, 0.14f), glm::vec3(1.00f, 0.52f, 0.14f),
        glm::vec3(1.00f, 0.52f, 0.14f), glm::vec3(1.00f, 0.52f, 0.14f),
        glm::vec3(1.00f, 0.52f, 0.14f),
        glm::vec3(0.10f, 0.22f, 0.60f), glm::vec3(0.10f, 0.22f, 0.60f),
        glm::vec3(0.18f, 0.34f, 0.42f),
        glm::vec3(1.20f, 0.62f, 0.18f),
        glm::vec3(1.20f, 0.55f, 0.12f)} {}

void LightManager::Apply(Shader& shader, Camera& camera, float sceneTime,
    bool nightLights, const SceneManager& scene) const {
    shader.Use();

    const glm::vec3 viewPosition = camera.GetPosition();
    glUniform3f(glGetUniformLocation(shader.Program, "viewPos"),
        viewPosition.x, viewPosition.y, viewPosition.z);

    glUniform3f(glGetUniformLocation(shader.Program, "dirLight.direction"),
        -0.25f, -1.0f, -0.3f);
    glUniform3f(glGetUniformLocation(shader.Program, "dirLight.ambient"),
        0.22f, 0.25f, 0.40f);
    glUniform3f(glGetUniformLocation(shader.Program, "dirLight.diffuse"),
        0.46f, 0.52f, 0.78f);
    glUniform3f(glGetUniformLocation(shader.Program, "dirLight.specular"),
        0.38f, 0.46f, 0.74f);

    std::array<glm::vec3, kPointLightCount> dynamicPositions = pointPositions_;
    const auto& boatPositions = scene.MovingBoatPositions();
    for (std::size_t i = 0; i < boatPositions.size(); ++i) {
        dynamicPositions[3 + i] = boatPositions[i] + glm::vec3(0.0f, 1.25f, 0.0f);
    }

    for (std::size_t i = 0; i < dynamicPositions.size(); ++i) {
        const std::string base = "pointLights[" + std::to_string(i) + "].";
        const glm::vec3 worldPosition = dynamicPositions[i] * kWorldScale;
        const float flicker = 0.88f + 0.12f * std::sin(sceneTime * (4.5f + static_cast<float>(i))
            + static_cast<float>(i) * 1.7f);
        const glm::vec3 color = nightLights ? pointColors_[i] * flicker : glm::vec3(0.05f);

        glUniform3f(glGetUniformLocation(shader.Program, (base + "position").c_str()),
            worldPosition.x, worldPosition.y, worldPosition.z);
        glUniform3f(glGetUniformLocation(shader.Program, (base + "ambient").c_str()),
            color.r * 0.24f, color.g * 0.24f, color.b * 0.24f);
        glUniform3f(glGetUniformLocation(shader.Program, (base + "diffuse").c_str()),
            color.r, color.g, color.b);
        glUniform3f(glGetUniformLocation(shader.Program, (base + "specular").c_str()),
            color.r, color.g, color.b);
        glUniform1f(glGetUniformLocation(shader.Program, (base + "constant").c_str()), 1.0f);
        glUniform1f(glGetUniformLocation(shader.Program, (base + "linear").c_str()), 0.028f);
        glUniform1f(glGetUniformLocation(shader.Program, (base + "quadratic").c_str()), 0.0035f);
    }

    const glm::vec3 front = camera.GetFront();
    glUniform3f(glGetUniformLocation(shader.Program, "spotLight.position"),
        viewPosition.x, viewPosition.y, viewPosition.z);
    glUniform3f(glGetUniformLocation(shader.Program, "spotLight.direction"),
        front.x, front.y, front.z);
    glUniform3f(glGetUniformLocation(shader.Program, "spotLight.ambient"),
        0.02f, 0.025f, 0.05f);
    glUniform3f(glGetUniformLocation(shader.Program, "spotLight.diffuse"),
        0.32f, 0.36f, 0.50f);
    glUniform3f(glGetUniformLocation(shader.Program, "spotLight.specular"),
        0.45f, 0.50f, 0.70f);
    glUniform1f(glGetUniformLocation(shader.Program, "spotLight.constant"), 1.0f);
    glUniform1f(glGetUniformLocation(shader.Program, "spotLight.linear"), 0.022f);
    glUniform1f(glGetUniformLocation(shader.Program, "spotLight.quadratic"), 0.0019f);
    glUniform1f(glGetUniformLocation(shader.Program, "spotLight.cutOff"),
        glm::cos(glm::radians(15.0f)));
    glUniform1f(glGetUniformLocation(shader.Program, "spotLight.outerCutOff"),
        glm::cos(glm::radians(24.0f)));
    glUniform1f(glGetUniformLocation(shader.Program, "material.shininess"), 32.0f);
}
