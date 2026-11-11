#pragma once

#include <array>

#include <glm/glm.hpp>

#include "Shader.h"
#include "Camera.h"

class SceneManager;

class LightManager {
public:
    static constexpr std::size_t kPointLightCount = 14;

    LightManager();

    void Apply(Shader& shader, Camera& camera, float sceneTime,
        bool nightLights, const SceneManager& scene) const;

    const std::array<glm::vec3, kPointLightCount>& PointPositions() const { return pointPositions_; }
    const std::array<glm::vec3, kPointLightCount>& PointColors() const { return pointColors_; }

private:
    std::array<glm::vec3, kPointLightCount> pointPositions_;
    std::array<glm::vec3, kPointLightCount> pointColors_;
};
