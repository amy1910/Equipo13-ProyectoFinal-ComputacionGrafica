#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

struct BoatFrame {
    glm::vec3 position;
    float rotationY;
    float duration;
};

struct KeyFrame {
    float time;
    glm::vec3 position;
    glm::vec3 rotation;
};

enum class CulturalInterest {
    None = -1,
    Altar = 0,
    Trajinera = 1,
    Chinampa = 2,
    PapelPicado = 3,
    Cempasuchil = 4,
    Munecas = 5
};

enum class SceneState {
    Intro,
    BoatsMoving,
    BoatsPaused,
    Finale
};

inline glm::mat4 MakeTransform(const glm::vec3& position,
    const glm::vec3& scale, float rotationY = 0.0f, float rotationZ = 0.0f) {
    glm::mat4 model(1.0f);
    model = glm::translate(model, position);
    model = glm::rotate(model, glm::radians(rotationY), glm::vec3(0.0f, 1.0f, 0.0f));
    if (rotationZ != 0.0f) {
        model = glm::rotate(model, glm::radians(rotationZ), glm::vec3(0.0f, 0.0f, 1.0f));
    }
    return glm::scale(model, scale);
}

inline glm::mat4 MakeTransform3D(const glm::vec3& position,
    const glm::vec3& scale, float rotationX = 0.0f, float rotationY = 0.0f,
    float rotationZ = 0.0f) {
    glm::mat4 model(1.0f);
    model = glm::translate(model, position);
    if (rotationX != 0.0f) {
        model = glm::rotate(model, glm::radians(rotationX), glm::vec3(1.0f, 0.0f, 0.0f));
    }
    if (rotationY != 0.0f) {
        model = glm::rotate(model, glm::radians(rotationY), glm::vec3(0.0f, 1.0f, 0.0f));
    }
    if (rotationZ != 0.0f) {
        model = glm::rotate(model, glm::radians(rotationZ), glm::vec3(0.0f, 0.0f, 1.0f));
    }
    return glm::scale(model, scale);
}
