#include "Renderer.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <sstream>
#include <string>
#include <vector>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#define STB_IMAGE_STATIC
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#undef STB_IMAGE_IMPLEMENTATION
#undef STB_IMAGE_STATIC

namespace {
// OBJ bounds measured from ConfigInicial/Models before worldRoot_:
// agua 2.0000 x 2.0000, isla 7.1895 x 5.1707, arbol 2.0592 x 2.0368,
// trajinera 2.0677 x 4.1969 and puente_madera 3.7000 x 1.1000 (X x Z).
// GenericIsland::radii stores the exact scaled isla half-extents used by the
// tree and boat collision rules below.
struct UiVertex {
    float x;
    float y;
    float r;
    float g;
    float b;
    float a;
};

using Glyph = std::array<unsigned char, 7>;

Glyph GlyphFor(char character) {
    switch (static_cast<char>(std::toupper(static_cast<unsigned char>(character)))) {
    case 'A': return {0x0E,0x11,0x11,0x1F,0x11,0x11,0x11};
    case 'B': return {0x1E,0x11,0x11,0x1E,0x11,0x11,0x1E};
    case 'C': return {0x0F,0x10,0x10,0x10,0x10,0x10,0x0F};
    case 'D': return {0x1E,0x11,0x11,0x11,0x11,0x11,0x1E};
    case 'E': return {0x1F,0x10,0x10,0x1E,0x10,0x10,0x1F};
    case 'F': return {0x1F,0x10,0x10,0x1E,0x10,0x10,0x10};
    case 'G': return {0x0F,0x10,0x10,0x17,0x11,0x11,0x0F};
    case 'H': return {0x11,0x11,0x11,0x1F,0x11,0x11,0x11};
    case 'I': return {0x1F,0x04,0x04,0x04,0x04,0x04,0x1F};
    case 'J': return {0x01,0x01,0x01,0x01,0x11,0x11,0x0E};
    case 'K': return {0x11,0x12,0x14,0x18,0x14,0x12,0x11};
    case 'L': return {0x10,0x10,0x10,0x10,0x10,0x10,0x1F};
    case 'M': return {0x11,0x1B,0x15,0x15,0x11,0x11,0x11};
    case 'N': return {0x11,0x19,0x15,0x13,0x11,0x11,0x11};
    case 'O': return {0x0E,0x11,0x11,0x11,0x11,0x11,0x0E};
    case 'P': return {0x1E,0x11,0x11,0x1E,0x10,0x10,0x10};
    case 'Q': return {0x0E,0x11,0x11,0x11,0x15,0x12,0x0D};
    case 'R': return {0x1E,0x11,0x11,0x1E,0x14,0x12,0x11};
    case 'S': return {0x0F,0x10,0x10,0x0E,0x01,0x01,0x1E};
    case 'T': return {0x1F,0x04,0x04,0x04,0x04,0x04,0x04};
    case 'U': return {0x11,0x11,0x11,0x11,0x11,0x11,0x0E};
    case 'V': return {0x11,0x11,0x11,0x11,0x11,0x0A,0x04};
    case 'W': return {0x11,0x11,0x11,0x15,0x15,0x1B,0x11};
    case 'X': return {0x11,0x11,0x0A,0x04,0x0A,0x11,0x11};
    case 'Y': return {0x11,0x11,0x0A,0x04,0x04,0x04,0x04};
    case 'Z': return {0x1F,0x01,0x02,0x04,0x08,0x10,0x1F};
    case ':': return {0x00,0x04,0x04,0x00,0x04,0x04,0x00};
    case ',': return {0x00,0x00,0x00,0x00,0x00,0x06,0x04};
    case '.': return {0x00,0x00,0x00,0x00,0x00,0x06,0x06};
    case '-': return {0x00,0x00,0x00,0x1F,0x00,0x00,0x00};
    case '[': return {0x0E,0x08,0x08,0x08,0x08,0x08,0x0E};
    case ']': return {0x0E,0x02,0x02,0x02,0x02,0x02,0x0E};
    default: return {0x00,0x00,0x00,0x00,0x00,0x00,0x00};
    }
}

std::vector<std::string> WrapText(const std::string& text, std::size_t maxCharacters) {
    std::istringstream words(text);
    std::vector<std::string> lines;
    std::string word;
    std::string currentLine;
    while (words >> word) {
        if (currentLine.empty()) {
            currentLine = word;
        } else if (currentLine.size() + 1u + word.size() <= maxCharacters) {
            currentLine += " " + word;
        } else {
            lines.push_back(currentLine);
            currentLine = word;
        }
    }
    if (!currentLine.empty()) lines.push_back(currentLine);
    return lines;
}

void AppendRect(std::vector<UiVertex>& vertices, float left, float top,
    float right, float bottom, const std::array<float, 4>& color) {
    const UiVertex a{left, top, color[0], color[1], color[2], color[3]};
    const UiVertex b{right, top, color[0], color[1], color[2], color[3]};
    const UiVertex c{right, bottom, color[0], color[1], color[2], color[3]};
    const UiVertex d{left, bottom, color[0], color[1], color[2], color[3]};
    vertices.insert(vertices.end(), {a, b, c, a, c, d});
}

void AppendTextLines(std::vector<UiVertex>& vertices,
    const std::vector<std::string>& lines, float left, float firstLineTop,
    float pixelWidth, float pixelHeight, float lineAdvance,
    const std::array<float, 4>& titleColor,
    const std::array<float, 4>& textColor) {
    const float characterAdvance = pixelWidth * 6.0f;
    for (std::size_t lineIndex = 0; lineIndex < lines.size(); ++lineIndex) {
        float cursorX = left;
        const float cursorY = firstLineTop - static_cast<float>(lineIndex) * lineAdvance;
        const auto& color = lineIndex == 0 ? titleColor : textColor;
        for (char character : lines[lineIndex]) {
            const Glyph glyph = GlyphFor(character);
            for (int row = 0; row < 7; ++row) {
                for (int column = 0; column < 5; ++column) {
                    if ((glyph[row] & (1u << (4 - column))) != 0) {
                        const float glyphLeft = cursorX + static_cast<float>(column) * pixelWidth;
                        const float glyphTop = cursorY - static_cast<float>(row) * pixelHeight;
                        AppendRect(vertices, glyphLeft, glyphTop,
                            glyphLeft + pixelWidth, glyphTop - pixelHeight, color);
                    }
                }
            }
            cursorX += characterAdvance;
        }
    }
}

glm::mat4 MakeBoatTransform(const glm::vec3& position, const glm::vec3& scale,
    float rotationY, float pitch, float roll) {
    glm::mat4 model(1.0f);
    model = glm::translate(model, position);
    model = glm::rotate(model, glm::radians(rotationY), glm::vec3(0.0f, 1.0f, 0.0f));
    model = glm::rotate(model, glm::radians(pitch), glm::vec3(1.0f, 0.0f, 0.0f));
    model = glm::rotate(model, glm::radians(roll), glm::vec3(0.0f, 0.0f, 1.0f));
    return glm::scale(model, scale);
}

float lampVertices[] = {
    -0.5f,-0.5f,-0.5f,0,0,-1, 0.5f,-0.5f,-0.5f,0,0,-1, 0.5f,0.5f,-0.5f,0,0,-1,
    0.5f,0.5f,-0.5f,0,0,-1, -0.5f,0.5f,-0.5f,0,0,-1, -0.5f,-0.5f,-0.5f,0,0,-1,
    -0.5f,-0.5f,0.5f,0,0,1, 0.5f,-0.5f,0.5f,0,0,1, 0.5f,0.5f,0.5f,0,0,1,
    0.5f,0.5f,0.5f,0,0,1, -0.5f,0.5f,0.5f,0,0,1, -0.5f,-0.5f,0.5f,0,0,1,
    -0.5f,0.5f,0.5f,-1,0,0, -0.5f,0.5f,-0.5f,-1,0,0, -0.5f,-0.5f,-0.5f,-1,0,0,
    -0.5f,-0.5f,-0.5f,-1,0,0, -0.5f,-0.5f,0.5f,-1,0,0, -0.5f,0.5f,0.5f,-1,0,0,
    0.5f,0.5f,0.5f,1,0,0, 0.5f,0.5f,-0.5f,1,0,0, 0.5f,-0.5f,-0.5f,1,0,0,
    0.5f,-0.5f,-0.5f,1,0,0, 0.5f,-0.5f,0.5f,1,0,0, 0.5f,0.5f,0.5f,1,0,0,
    -0.5f,-0.5f,-0.5f,0,-1,0, 0.5f,-0.5f,-0.5f,0,-1,0, 0.5f,-0.5f,0.5f,0,-1,0,
    0.5f,-0.5f,0.5f,0,-1,0, -0.5f,-0.5f,0.5f,0,-1,0, -0.5f,-0.5f,-0.5f,0,-1,0,
    -0.5f,0.5f,-0.5f,0,1,0, 0.5f,0.5f,-0.5f,0,1,0, 0.5f,0.5f,0.5f,0,1,0,
    0.5f,0.5f,0.5f,0,1,0, -0.5f,0.5f,0.5f,0,1,0, -0.5f,0.5f,-0.5f,0,1,0
};

const float skyboxVertices[] = {
    // back (-Z)
    -1.0f,  1.0f, -1.0f,  -1.0f, -1.0f, -1.0f,   1.0f, -1.0f, -1.0f,
     1.0f, -1.0f, -1.0f,   1.0f,  1.0f, -1.0f,  -1.0f,  1.0f, -1.0f,
    // left (-X)
    -1.0f, -1.0f,  1.0f,  -1.0f, -1.0f, -1.0f,  -1.0f,  1.0f, -1.0f,
    -1.0f,  1.0f, -1.0f,  -1.0f,  1.0f,  1.0f,  -1.0f, -1.0f,  1.0f,
    // right (+X)
     1.0f, -1.0f, -1.0f,   1.0f, -1.0f,  1.0f,   1.0f,  1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,   1.0f,  1.0f, -1.0f,   1.0f, -1.0f, -1.0f,
    // bottom (-Y)
    -1.0f, -1.0f, -1.0f,  -1.0f, -1.0f,  1.0f,   1.0f, -1.0f,  1.0f,
     1.0f, -1.0f,  1.0f,   1.0f, -1.0f, -1.0f,  -1.0f, -1.0f, -1.0f,
    // top (+Y)
    -1.0f,  1.0f, -1.0f,   1.0f,  1.0f, -1.0f,   1.0f,  1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,  -1.0f,  1.0f,  1.0f,  -1.0f,  1.0f, -1.0f,
    // front (+Z)
    -1.0f, -1.0f,  1.0f,   1.0f, -1.0f,  1.0f,   1.0f,  1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,  -1.0f,  1.0f,  1.0f,  -1.0f, -1.0f,  1.0f
};
}

Renderer::Renderer(int width, int height)
    : width_(width), height_(height),
      // Global world scale: the original scene remains internally coherent,
      // while the displayed environment is three times larger and walkable.
      worldRoot_(MakeTransform(glm::vec3(0.0f), glm::vec3(3.0f))),
      projection_(1.0f),
      lightingShader_(ResolveAssetPath("Shader/lighting.vs").c_str(), ResolveAssetPath("Shader/lighting.frag").c_str()),
      lampShader_(ResolveAssetPath("Shader/lamp.vs").c_str(), ResolveAssetPath("Shader/lamp.frag").c_str()),
      waterShader_(ResolveAssetPath("Shader/water.vs").c_str(), ResolveAssetPath("Shader/water.frag").c_str()),
      uiShader_(ResolveAssetPath("Shader/ui.vs").c_str(), ResolveAssetPath("Shader/ui.frag").c_str()),
      skyboxShader_(ResolveAssetPath("Shader/skybox.vs").c_str(), ResolveAssetPath("Shader/skybox.frag").c_str()),
      lampVAO_(0), lampVBO_(0), uiVAO_(0), uiVBO_(0),
      skyboxVAO_(0), skyboxVBO_(0), skyboxTexture_(0) {
    Resize(width, height);
    glGenVertexArrays(1, &lampVAO_);
    glGenBuffers(1, &lampVBO_);
    glBindVertexArray(lampVAO_);
    glBindBuffer(GL_ARRAY_BUFFER, lampVBO_);
    glBufferData(GL_ARRAY_BUFFER, sizeof(lampVertices), lampVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindVertexArray(0);

    lightingShader_.Use();
    glUniform1i(glGetUniformLocation(lightingShader_.Program, "material.diffuse"), 0);
    glUniform1i(glGetUniformLocation(lightingShader_.Program, "material.specular"), 1);
    waterShader_.Use();
    glUniform1i(glGetUniformLocation(waterShader_.Program, "material.diffuse"), 0);

    glGenVertexArrays(1, &uiVAO_);
    glGenBuffers(1, &uiVBO_);
    glBindVertexArray(uiVAO_);
    glBindBuffer(GL_ARRAY_BUFFER, uiVBO_);
    glBufferData(GL_ARRAY_BUFFER, 0, nullptr, GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(UiVertex), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(UiVertex), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindVertexArray(0);

    glGenVertexArrays(1, &skyboxVAO_);
    glGenBuffers(1, &skyboxVBO_);
    glBindVertexArray(skyboxVAO_);
    glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO_);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), skyboxVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glBindVertexArray(0);

    const std::vector<std::string> skyboxFaces{
        "Models/skybox/right.png", "Models/skybox/left.png",
        "Models/skybox/top.png", "Models/skybox/bottom.png",
        "Models/skybox/front.png", "Models/skybox/back.png"};
    skyboxTexture_ = LoadCubemap(skyboxFaces);
    skyboxShader_.Use();
    glUniform1i(glGetUniformLocation(skyboxShader_.Program, "skybox"), 0);
}

Renderer::~Renderer() {
    glDeleteVertexArrays(1, &lampVAO_);
    glDeleteBuffers(1, &lampVBO_);
    glDeleteVertexArrays(1, &uiVAO_);
    glDeleteBuffers(1, &uiVBO_);
    glDeleteVertexArrays(1, &skyboxVAO_);
    glDeleteBuffers(1, &skyboxVBO_);
    glDeleteTextures(1, &skyboxTexture_);
}

void Renderer::Resize(int width, int height) {
    width_ = width;
    height_ = height > 0 ? height : 1;
    projection_ = glm::perspective(glm::radians(45.0f),
        static_cast<float>(width_) / static_cast<float>(height_), 0.1f, 1000.0f);
    glViewport(0, 0, width_, height_);
}

GLuint Renderer::LoadCubemap(const std::vector<std::string>& faces) {
    GLuint textureID = 0;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);
    stbi_set_flip_vertically_on_load(false);

    for (std::size_t faceIndex = 0; faceIndex < faces.size(); ++faceIndex) {
        const std::string resolvedPath = ResolveAssetPath(faces[faceIndex].c_str());
        int width = 0;
        int height = 0;
        int channels = 0;
        unsigned char* image = stbi_load(resolvedPath.c_str(), &width, &height,
            &channels, STBI_rgb_alpha);
        if (image == nullptr || width <= 0 || height <= 0) {
            std::cerr << "SKYBOX::LOAD_FAILED " << resolvedPath
                << " (" << stbi_failure_reason() << ")" << std::endl;
            if (image != nullptr) stbi_image_free(image);
            continue;
        }
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + static_cast<GLenum>(faceIndex),
            0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
        stbi_image_free(image);
    }
    glGenerateMipmap(GL_TEXTURE_CUBE_MAP);

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
    return textureID;
}

void Renderer::DrawSkybox(SceneManager& scene, float timeSeconds) {
    skyboxShader_.Use();
    // The cubemap follows the camera orientation but never its position.
    // Passing this rotation-only matrix prevents the camera from crossing a
    // finite cube wall and producing a hard vertical split in the viewport.
    const glm::mat4 viewSkybox = glm::mat4(glm::mat3(
        scene.GetCamera().GetViewMatrix()));
    glUniformMatrix4fv(glGetUniformLocation(skyboxShader_.Program, "view"), 1,
        GL_FALSE, glm::value_ptr(viewSkybox));
    glUniformMatrix4fv(glGetUniformLocation(skyboxShader_.Program, "projection"), 1,
        GL_FALSE, glm::value_ptr(projection_));
    glUniform1f(glGetUniformLocation(skyboxShader_.Program, "time"), timeSeconds);

    const GLboolean cullWasEnabled = glIsEnabled(GL_CULL_FACE);
    glDisable(GL_CULL_FACE);
    glDepthFunc(GL_LEQUAL);
    glDepthMask(GL_FALSE);
    glActiveTexture(GL_TEXTURE0);
    glUniform1i(glGetUniformLocation(skyboxShader_.Program, "skybox"), 0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, skyboxTexture_);
    glBindVertexArray(skyboxVAO_);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
    glDepthMask(GL_TRUE);
    glDepthFunc(GL_LESS);
    if (cullWasEnabled) {
        glEnable(GL_CULL_FACE);
    } else {
        glDisable(GL_CULL_FACE);
    }
}

void Renderer::DrawObject(Shader& shader, Model& object, const glm::vec3& position,
    const glm::vec3& scale, float rotationY, float rotationZ) {
    DrawObjectWithMatrix(shader, object, MakeTransform(position, scale, rotationY, rotationZ));
}

void Renderer::DrawObjectWithMatrix(Shader& shader, Model& object, const glm::mat4& model) {
    shader.Use();
    const glm::mat4 worldModel = worldRoot_ * model;
    glUniformMatrix4fv(glGetUniformLocation(shader.Program, "model"), 1, GL_FALSE,
        glm::value_ptr(worldModel));
    object.ApplyMaterialUniforms(shader);
    object.Draw(shader);
}

void Renderer::DrawObjectWithParent(Shader& shader, Model& object, const glm::mat4& parent,
    const glm::vec3& localPosition, const glm::vec3& scale, float rotationY, float rotationZ) {
    DrawObjectWithMatrix(shader, object,
        parent * MakeTransform(localPosition, scale, rotationY, rotationZ));
}

void Renderer::DrawLamp(const glm::vec3& position, const glm::vec3& color, float scale) {
    lampShader_.Use();
    glUniform3f(glGetUniformLocation(lampShader_.Program, "lampColor"), color.r, color.g, color.b);
    const glm::mat4 model = worldRoot_ * MakeTransform(position, glm::vec3(scale));
    glUniformMatrix4fv(glGetUniformLocation(lampShader_.Program, "model"), 1, GL_FALSE,
        glm::value_ptr(model));
    glBindVertexArray(lampVAO_);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);
}

void Renderer::DrawBoatWithCrew(SceneManager& scene, const glm::vec3& position,
    float rotationY, float pitch, float roll, float bank, float scale, float phase) {
    Shader& shader = lightingShader_;
    const glm::mat4 boatRoot = MakeBoatTransform(position,
        glm::vec3(scale), rotationY, pitch, roll + bank);
    DrawObjectWithMatrix(shader, scene.Trajinera(), boatRoot);

    // Crew, musician and candle share the boat transform so every passenger
    // travels with its own hull instead of remaining fixed in the world.
    DrawObjectWithMatrix(shader, scene.Person(),
        boatRoot * MakeTransform(glm::vec3(-0.30f, 0.42f, 0.05f), glm::vec3(0.28f), 18.0f));
    DrawObjectWithMatrix(shader, scene.Person(),
        boatRoot * MakeTransform(glm::vec3(0.30f, 0.42f, -0.02f), glm::vec3(0.26f), -18.0f));
    DrawObjectWithMatrix(shader, scene.Musician(),
        boatRoot * MakeTransform(glm::vec3(0.0f, 0.48f, 0.28f), glm::vec3(0.25f), 180.0f,
            4.0f * std::sin(phase)));
    DrawObjectWithMatrix(shader, scene.Candle(),
        boatRoot * MakeTransform(glm::vec3(0.0f, 0.60f, -0.16f), glm::vec3(0.18f), 0.0f,
            2.0f * std::cos(phase * 1.3f)));
}

void Renderer::DrawMariachiBand(SceneManager& scene, const glm::mat4& parent) {
    Shader& shader = lightingShader_;
    // The OBJ parts share a local character space: the body is centered at
    // the origin, the arms pivot near the shoulders, and the instrument points
    // toward +Z. The plaza/audience side of this stage is also +Z. The
    // previous 180-degree root yaw sent the instruments toward the rear;
    // this is the compensating 180-degree turn, so the final front-facing
    // yaw is 0 degrees and every articulated child inherits it.
    const glm::vec3 kRightArmPivot(0.36f, 1.55f, 0.24f);
    const glm::vec3 kLeftArmPivot(-0.36f, 1.55f, 0.24f);
    const glm::vec3 kInstrumentPivot(0.0f, 1.62f, 0.42f);

    for (const std::unique_ptr<MariachiActor>& actorPtr : scene.Mariachis()) {
        if (!actorPtr) continue;
        const MariachiActor& actor = *actorPtr;
        const float rhythm = scene.SceneTime() * 7.0f + actor.instrumentType * 0.85f;
        const float step = std::sin(rhythm);
        const float alternatingStep = std::sin(rhythm
            + (actor.instrumentType == 1 ? 0.8f : 0.0f));
        const float breathing = 0.5f + 0.5f * std::sin(scene.SceneTime() * 2.4f
            + actor.instrumentType * 0.7f);
        const float torsoLean = actor.instrumentType == 2
            ? 5.5f * std::sin(scene.SceneTime() * 5.5f + 0.4f)
            : 2.0f * std::sin(scene.SceneTime() * 3.5f + actor.instrumentType);
        const float headTurn = 3.5f * std::sin(scene.SceneTime() * 1.9f
            + actor.instrumentType * 1.4f);
        const glm::vec3 animatedPosition = actor.position
            + glm::vec3(0.006f * alternatingStep, std::abs(step) * 0.024f, 0.0f);
        const glm::vec3 animatedScale(
            actor.scale * (1.0f - 0.018f * std::abs(step)),
            actor.scale * (1.0f + 0.035f * breathing),
            actor.scale * (1.0f - 0.018f * std::abs(step)));
        constexpr float kStageFrontYaw = 0.0f;
        const glm::mat4 root = parent * MakeTransform3D(animatedPosition,
            animatedScale, torsoLean * 0.15f,
            kStageFrontYaw + actor.rotationY + headTurn, torsoLean * 0.50f);

        DrawObjectWithMatrix(shader, *actor.body, root);
        DrawObjectWithMatrix(shader, *actor.redBowTie, root);

        const float strum = actor.instrumentType == 1
            ? 9.0f * std::sin(scene.SceneTime() * 13.0f + 0.5f)
            : (actor.instrumentType == 2
                ? 4.0f * std::sin(scene.SceneTime() * 8.0f)
                : 2.0f * std::sin(scene.SceneTime() * 6.0f));
        const float rightArmAngle = actor.fsm.anguloBrazo + strum
            + 3.0f * std::sin(scene.SceneTime() * 9.0f + actor.instrumentType);
        const float leftArmAngle = actor.instrumentType == 2
            ? -actor.fsm.anguloBrazo * 0.55f
            : -actor.fsm.anguloBrazo * 0.72f - strum * 0.18f;
        const glm::mat4 rightArm = TransformarMariachi3D(
            root, rightArmAngle, actor.instrumentType == 2 ? -5.0f : 0.0f,
            kRightArmPivot);
        const glm::mat4 leftArm = TransformarMariachi3D(
            root, leftArmAngle, actor.instrumentType == 2 ? 4.0f : 0.0f,
            kLeftArmPivot);
        DrawObjectWithMatrix(shader, *actor.rightArm, rightArm);
        DrawObjectWithMatrix(shader, *actor.leftArm, leftArm);

        const float trumpetPitch = actor.instrumentType == 2
            ? -7.0f * std::max(0.0f, std::sin(scene.SceneTime() * 4.0f)) : 0.0f;
        const glm::mat4 instrument = TransformarMariachi3D(
            root, actor.fsm.anguloInstrumento + 1.8f * std::sin(rhythm * 0.8f),
            trumpetPitch, kInstrumentPivot);
        DrawObjectWithMatrix(shader, *actor.instrument, instrument);
        if (actor.bow) DrawObjectWithMatrix(shader, *actor.bow, instrument);
    }
}

void Renderer::DrawSugarSkulls(SceneManager& scene, const glm::mat4& parent) {
    Shader& shader = lightingShader_;
    const SugarSkullAnimationPose& pose = scene.SugarSkulls();
    // The bridge approaches this island from local -X; the exported models
    // face local +Z, so rotate both roots toward the bridge.
    constexpr float kBridgeFacingYaw = -90.0f;
    const glm::mat4 moribundaRoot = parent * MakeTransform3D(
        pose.moribundaPosition, pose.moribundaScale, 0.0f, kBridgeFacingYaw,
        pose.moribundaRotationZ);
    const glm::mat4 finadoRoot = parent * MakeTransform3D(
        pose.finadoPosition, pose.finadoScale, 0.0f, kBridgeFacingYaw,
        pose.finadoRotationZ);

    DrawObjectWithMatrix(shader, scene.MoribundaBody(), moribundaRoot);
    DrawObjectWithMatrix(shader, scene.MoribundaHead(), moribundaRoot);
    DrawObjectWithMatrix(shader, scene.FinadoBody(), finadoRoot);
    DrawObjectWithMatrix(shader, scene.FinadoHead(), finadoRoot);

    // A small warm marker makes the two sugar skulls readable at night and
    // reinforces their cultural-point location without changing their poses.
    if (pose.phase == SugarSkullPhase::StackedImpact) {
        const glm::vec3 impactPoint = glm::vec3(parent
            * glm::vec4(pose.moribundaPosition + glm::vec3(0.0f, 1.25f, 0.30f), 1.0f));
        DrawLamp(impactPoint, glm::vec3(1.20f, 0.42f, 0.08f), 0.055f);
    }
}

void Renderer::DrawSpiritParticles(float sceneTime) {
    const std::array<glm::vec3, 3> centers{
        glm::vec3(5.4f, 0.0f, -3.2f), glm::vec3(0.0f, 0.0f, 5.2f),
        glm::vec3(-5.4f, 0.0f, -3.2f)};
    for (int particleIndex = 0; particleIndex < 60; ++particleIndex) {
        const float index = static_cast<float>(particleIndex);
        const glm::vec3& center = centers[particleIndex % centers.size()];
        const float radius = 0.32f + static_cast<float>(particleIndex % 5) * 0.20f;
        const float angularSpeed = 0.75f + static_cast<float>(particleIndex % 4) * 0.11f;
        const float angle = index * 2.3999632f + sceneTime * angularSpeed;
        const float x = center.x + std::cos(angle) * radius;
        const float z = center.z + std::sin(angle) * radius;
        const float y = 0.35f + std::fmod(index * 0.19f + sceneTime * (0.28f
            + static_cast<float>(particleIndex % 4) * 0.035f), 2.9f)
            + 0.10f * std::sin(angle * 1.7f + sceneTime);
        const float intensity = 0.72f + 0.32f * std::sin(sceneTime * 3.2f + index * 0.4f);
        const glm::vec3 particleColor = (particleIndex % 3 == 0)
            ? glm::vec3(1.25f, 0.42f, 0.06f) * intensity
            : glm::vec3(0.75f, 0.18f, 0.75f) * intensity;
        const float size = 0.022f + static_cast<float>(particleIndex % 3) * 0.009f;
        DrawLamp(glm::vec3(x, y, z), particleColor, size);
    }
}

void Renderer::DrawWater(SceneManager& scene, const LightManager& lights, float timeSeconds) {
    waterShader_.Use();
    // agua.obj is a subdivided unit surface and worldRoot_ scales the scene by
    // 3x. Build the water boundary from the five farthest relevant model
    // points, instead of leaving an oversized ocean plane around the map.
    std::vector<glm::vec2> boundaryPoints;
    boundaryPoints.reserve(scene.GenericIslands().size() +
        scene.DockedBoatPositions().size() + 5);
    const auto appendRadialPoint = [&](const glm::vec2& center, float radius) {
        const float distance = glm::length(center);
        const glm::vec2 direction = distance > 0.001f
            ? center / distance : glm::vec2(1.0f, 0.0f);
        boundaryPoints.push_back(center + direction * radius);
    };
    for (const GenericIsland& island : scene.GenericIslands()) {
        // The diagonal is a conservative outer point for the measured island
        // ellipse, so the fifth-farthest point cannot be clipped by water.
        appendRadialPoint(glm::vec2(island.position.x, island.position.z),
            glm::length(island.radii) + 1.25f);
    }
    for (const glm::vec3& boat : scene.DockedBoatPositions()) {
        appendRadialPoint(glm::vec2(boat.x, boat.z), 3.25f);
    }
    for (const glm::vec2& bridge : std::array<glm::vec2, 5>{
        glm::vec2(0.0f, -3.2f), glm::vec2(-3.25f, 1.5f),
        glm::vec2(3.25f, 1.5f), glm::vec2(-17.2f, 14.5f),
        glm::vec2(17.2f, 14.5f)}) {
        appendRadialPoint(bridge, 2.4f);
    }
    std::sort(boundaryPoints.begin(), boundaryPoints.end(),
        [](const glm::vec2& left, const glm::vec2& right) {
            return glm::dot(left, left) > glm::dot(right, right);
        });
    const std::size_t measuredPointCount = std::min<std::size_t>(5, boundaryPoints.size());
    float farthestFiveRadius = 0.0f;
    for (std::size_t i = 0; i < measuredPointCount; ++i) {
        farthestFiveRadius = std::max(farthestFiveRadius, glm::length(boundaryPoints[i]));
    }
    // The margin is local-space; worldRoot_ multiplies it by 3.0 as well.
    // The user-facing water coverage is intentionally 200% of the measured
    // five-point envelope in X/Z, keeping the archipelago surrounded even
    // from the widest camera angles while remaining centered at the origin.
    constexpr float kWaterXZCoverageMultiplier = 3.0f;
    const float waterHalfExtent = kWaterXZCoverageMultiplier
        * (farthestFiveRadius + 2.0f) / 3.0f;
    const glm::mat4 model = worldRoot_ * MakeTransform(glm::vec3(0.0f, -0.20f, 0.0f),
        glm::vec3(waterHalfExtent, 1.0f, waterHalfExtent));
    const glm::mat4 view = scene.ViewMatrix();
    glUniformMatrix4fv(glGetUniformLocation(waterShader_.Program, "model"), 1, GL_FALSE,
        glm::value_ptr(model));
    glUniformMatrix4fv(glGetUniformLocation(waterShader_.Program, "view"), 1, GL_FALSE,
        glm::value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(waterShader_.Program, "projection"), 1, GL_FALSE,
        glm::value_ptr(projection_));
    glUniform1f(glGetUniformLocation(waterShader_.Program, "time"), timeSeconds);
    const glm::vec3 cameraPosition = scene.GetCamera().GetPosition();
    glUniform3f(glGetUniformLocation(waterShader_.Program, "cameraPosition"),
        cameraPosition.x, cameraPosition.y, cameraPosition.z);
    glUniform3f(glGetUniformLocation(waterShader_.Program, "fogColor"),
        0.055f, 0.018f, 0.115f);
    // Keep the distant water readable; the skybox still provides the final
    // horizon blend, but the channels must not collapse to near-black.
    glUniform1f(glGetUniformLocation(waterShader_.Program, "fogDensity"), 0.0035f);
    // Water is drawn below the corrected chinampa bases. Depth testing now
    // lets the water meet the shoreline geometry naturally instead of using a
    // conservative circular discard mask that created black halos.
    for (std::size_t i = 0; i < lights.PointPositions().size(); ++i) {
        const std::string positionName = "pointLightPositions[" + std::to_string(i) + "]";
        const std::string colorName = "pointLightColors[" + std::to_string(i) + "]";
        glm::vec3 position = lights.PointPositions()[i];
        if (i >= 3 && i < 3 + SceneManager::kMovingBoatCount) {
            position = scene.MovingBoatPositions()[i - 3] + glm::vec3(0.0f, 1.25f, 0.0f);
        }
        position *= 3.0f;
        const glm::vec3 color = lights.PointColors()[i];
        glUniform3f(glGetUniformLocation(waterShader_.Program, positionName.c_str()),
            position.x, position.y, position.z);
        glUniform3f(glGetUniformLocation(waterShader_.Program, colorName.c_str()),
            color.r, color.g, color.b);
    }
    // The water is the opaque base layer of the channels.  Islands and boats
    // are rendered afterward and pass the depth test because they sit above
    // the water surface.
    glDisable(GL_BLEND);
    glDepthMask(GL_TRUE);
    scene.Water().Draw(waterShader_);
}

void Renderer::Draw(SceneManager& scene, const LightManager& lights, float timeSeconds) {
    glClearColor(0.055f, 0.018f, 0.115f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    lightingShader_.Use();
    lights.Apply(lightingShader_, scene.GetCamera(), scene.SceneTime(), scene.NightLights(), scene);
    glUniform3f(glGetUniformLocation(lightingShader_.Program, "fogColor"),
        0.055f, 0.018f, 0.115f);
    glUniform1f(glGetUniformLocation(lightingShader_.Program, "fogDensity"), 0.008f);
    const glm::mat4 view = scene.ViewMatrix();
    glUniformMatrix4fv(glGetUniformLocation(lightingShader_.Program, "view"), 1, GL_FALSE,
        glm::value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(lightingShader_.Program, "projection"), 1, GL_FALSE,
        glm::value_ptr(projection_));
    glUniform1i(glGetUniformLocation(lightingShader_.Program, "transparency"), 0);
    DrawWorld(scene, lights, timeSeconds);
    // Draw after the world so the cube is the far background while the depth
    // test keeps all scene geometry in front of it.  The UI overlay is drawn
    // last and therefore remains legible above the skybox.
    DrawSkybox(scene, timeSeconds);
    if (scene.HasNearbyInterest() || scene.ShowMinimap()) {
        DrawOfrendaOverlay(scene);
    }
    if (scene.ShowHelp() && !scene.ShowInterestInfo()) {
        DrawControlsOverlay(scene);
    }
}

void Renderer::DrawOfrendaOverlay(const SceneManager& scene) {
    const std::array<float, 4> panelColor{0.015f, 0.035f, 0.075f, 0.90f};
    const std::array<float, 4> borderColor{0.95f, 0.55f, 0.18f, 0.95f};
    const std::array<float, 4> textColor{0.96f, 0.93f, 0.82f, 1.0f};
    const std::array<float, 4> titleColor{1.0f, 0.69f, 0.25f, 1.0f};
    std::vector<UiVertex> vertices;
    vertices.reserve(20000);
    const bool infoOpen = scene.ShowInterestInfo();
    const float panelRight = infoOpen ? 0.97f : 0.36f;

    std::vector<std::string> lines;
    const CulturalPOI& poi = scene.CurrentCulturalPOI();
    if (!scene.HasNearbyInterest()) {
        lines = {"RECORRIDO XOCHIMILCO", "ACERCATE A UN PUNTO CULTURAL", "Y PRESIONA [E] PARA EXPLORARLO."};
    } else if (!infoOpen) {
        lines = {"PUNTO CULTURAL", "PRESIONA [E] PARA VER", "INFORMACION DE:", poi.title};
    } else {
        lines.push_back(poi.title);
        lines.push_back("");
        const std::vector<std::string> contentLines = WrapText(poi.content, 68u);
        lines.insert(lines.end(), contentLines.begin(), contentLines.end());
        lines.push_back("");
        const std::vector<std::string> keyFactLines = WrapText(poi.keyFact, 68u);
        lines.insert(lines.end(), keyFactLines.begin(), keyFactLines.end());
    }

    // The prompt is intentionally compact; the full cultural sheet keeps its
    // larger reading panel.  Deriving the compact height from the number of
    // lines avoids a large empty block below the [E] prompt.
    const float compactPanelBottom = lines.size() <= 3u ? 0.60f : 0.50f;
    const float panelBottom = infoOpen ? -0.96f : compactPanelBottom;
    AppendRect(vertices, -0.97f, 0.96f, panelRight, panelBottom, panelColor);
    AppendRect(vertices, -0.97f, 0.96f, panelRight, 0.948f, borderColor);
    AppendRect(vertices, -0.97f, panelBottom + 0.012f, panelRight, panelBottom, borderColor);

    if (infoOpen) {
        AppendTextLines(vertices, lines, -0.90f, 0.90f,
            0.0033f, 0.007f, 0.057f, titleColor, textColor);
    } else {
        AppendTextLines(vertices, lines, -0.90f, 0.895f,
            0.0035f, 0.007f, 0.080f, titleColor, textColor);
    }

    if (scene.ShowMinimap() && !infoOpen) {
        const std::array<float, 4> mapColor{0.01f, 0.10f, 0.16f, 0.92f};
        const std::array<float, 4> mapBorder{0.20f, 0.75f, 0.85f, 0.95f};
        AppendRect(vertices, 0.64f, 0.90f, 0.94f, 0.58f, mapColor);
        AppendRect(vertices, 0.64f, 0.90f, 0.94f, 0.885f, mapBorder);
        AppendRect(vertices, 0.64f, 0.595f, 0.94f, 0.58f, mapBorder);
        const auto& points = scene.InterestPositions();
        const std::array<std::array<float, 4>, 6> pointColors{
            std::array<float,4>{1.0f,0.25f,0.10f,1.0f},
            std::array<float,4>{1.0f,0.80f,0.16f,1.0f},
            std::array<float,4>{0.20f,0.65f,1.0f,1.0f},
            std::array<float,4>{0.95f,0.12f,0.65f,1.0f},
            std::array<float,4>{1.0f,0.42f,0.05f,1.0f},
            std::array<float,4>{0.76f,0.76f,0.82f,1.0f}};
        for (std::size_t i = 0; i < points.size(); ++i) {
            const float x = glm::clamp(0.79f + points[i].x / 100.0f, 0.66f, 0.92f);
            const float y = glm::clamp(0.74f - points[i].z / 100.0f, 0.60f, 0.87f);
            AppendRect(vertices, x - 0.012f, y + 0.012f, x + 0.012f, y - 0.012f, pointColors[i]);
        }
        const glm::vec3 camera = scene.GetCamera().GetPosition();
        const float cameraX = glm::clamp(0.79f + camera.x / 100.0f, 0.66f, 0.92f);
        const float cameraY = glm::clamp(0.74f - camera.z / 100.0f, 0.60f, 0.87f);
        const std::array<float,4> cameraColor{1.0f,1.0f,1.0f,1.0f};
        AppendRect(vertices, cameraX - 0.009f, cameraY + 0.009f,
            cameraX + 0.009f, cameraY - 0.009f, cameraColor);
    }

    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    uiShader_.Use();
    glBindVertexArray(uiVAO_);
    glBindBuffer(GL_ARRAY_BUFFER, uiVBO_);
    glBufferData(GL_ARRAY_BUFFER,
        static_cast<GLsizeiptr>(vertices.size() * sizeof(UiVertex)),
        vertices.data(), GL_DYNAMIC_DRAW);
    glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(vertices.size()));
    glBindVertexArray(0);
    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
}

void Renderer::DrawControlsOverlay(const SceneManager& scene) {
    const std::array<float, 4> panelColor{0.012f, 0.025f, 0.055f, 0.94f};
    const std::array<float, 4> borderColor{0.25f, 0.78f, 0.88f, 0.96f};
    const std::array<float, 4> textColor{0.90f, 0.96f, 0.98f, 1.0f};
    const std::array<float, 4> titleColor{0.98f, 0.72f, 0.26f, 1.0f};
    std::vector<UiVertex> vertices;
    vertices.reserve(16000);

    // Sixteen status/control lines need a taller panel. The compact font keeps
    // the longest FSM label inside the right edge while the bottom border
    // remains visible on 16:10 and 16:9 windows.
    AppendRect(vertices, 0.42f, 0.52f, 0.98f, -0.78f, panelColor);
    AppendRect(vertices, 0.42f, 0.52f, 0.98f, 0.495f, borderColor);
    AppendRect(vertices, 0.42f, -0.755f, 0.98f, -0.78f, borderColor);

    std::vector<std::string> lines{
        "CONTROLES",
        "WASD O FLECHAS: MOVER",
        "MOUSE: CAMARA",
        "E: INFO CULTURAL",
        "M: MAPA",
        "P: ANIMACIONES",
        "L: LUCES",
        "B: MUSICA ON/OFF",
        "ESPACIO: KF PLAY PAUSA",
        "R: REINICIAR KF",
        "K: FSM MARIACHIS ON/OFF",
        "H: OCULTAR AYUDA",
        scene.KeyframePlayback() ? "KF: REPRODUCIENDO" : "KF: EN PAUSA",
        std::string("FSM: ") + (scene.MariachiFSMEnabled() ? "ACTIVA " : "EN PAUSA ")
            + (scene.Mariachis().front() ? scene.Mariachis().front()->fsm.EstadoNombreCorto() : "N/D"),
        scene.NightLights() ? "LUCES: NOCHE" : "LUCES: SUAVE",
        scene.MusicEnabled() ? "MUSICA: ACTIVA" : "MUSICA: SILENCIADA"};

    AppendTextLines(vertices, lines, 0.47f, 0.45f,
        0.0031f, 0.0066f, 0.065f, titleColor, textColor);

    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    uiShader_.Use();
    glBindVertexArray(uiVAO_);
    glBindBuffer(GL_ARRAY_BUFFER, uiVBO_);
    glBufferData(GL_ARRAY_BUFFER,
        static_cast<GLsizeiptr>(vertices.size() * sizeof(UiVertex)),
        vertices.data(), GL_DYNAMIC_DRAW);
    glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(vertices.size()));
    glBindVertexArray(0);
    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
}

void Renderer::DrawWorld(SceneManager& scene, const LightManager& lights, float timeSeconds) {
    DrawWater(scene, lights, timeSeconds);
    lightingShader_.Use();
    Shader& shader = lightingShader_;
    const float sceneTime = scene.SceneTime();

    DrawGenericIslands(scene);
    glUniform1i(glGetUniformLocation(shader.Program, "transparency"), 0);

    DrawObject(shader, scene.Bridge(), glm::vec3(0.0f, 0.08f, -3.2f), glm::vec3(1.0f), 0.0f);
    DrawObject(shader, scene.Bridge(), glm::vec3(-3.25f, 0.08f, 1.5f), glm::vec3(1.0f), -63.4f);
    DrawObject(shader, scene.Bridge(), glm::vec3(3.25f, 0.08f, 1.5f), glm::vec3(1.0f), 63.4f);
    DrawObject(shader, scene.Bridge(), glm::vec3(-17.2f, 0.08f, 14.5f), glm::vec3(1.0f), 25.0f);
    DrawObject(shader, scene.Bridge(), glm::vec3(17.2f, 0.08f, 14.5f), glm::vec3(1.0f), -25.0f);
    DrawObject(shader, scene.Moon(), glm::vec3(0.0f, 12.0f, -9.0f), glm::vec3(2.8f));
    DrawObject(shader, scene.Rock(), glm::vec3(-7.6f, -0.05f, 0.2f), glm::vec3(0.9f));
    DrawObject(shader, scene.Rock(), glm::vec3(7.6f, -0.05f, 0.2f), glm::vec3(0.9f), 45.0f);
    for (std::size_t i = 5; i < scene.CandleProps().size(); ++i) {
        const glm::vec3& candle = scene.CandleProps()[i];
        DrawObject(shader, scene.Candle(), candle, glm::vec3(0.35f));
    }

    const std::array<float, SceneManager::kMovingBoatCount> boatScales{
        0.95f, 0.90f, 0.86f, 0.92f};
    for (std::size_t boatIndex = 0; boatIndex < SceneManager::kMovingBoatCount; ++boatIndex) {
        const float phase = sceneTime * (1.7f + static_cast<float>(boatIndex) * 0.23f)
            + static_cast<float>(boatIndex) * 1.1f;
        const float pitch = 1.25f * std::sin(phase);
        const float roll = 1.65f * std::cos(phase * 0.83f);
        DrawBoatWithCrew(scene, scene.MovingBoatPositions()[boatIndex],
            scene.MovingBoatRotations()[boatIndex], pitch, roll,
            scene.MovingBoatBanks()[boatIndex], boatScales[boatIndex], phase);
    }
    for (std::size_t boatIndex = 0; boatIndex < SceneManager::kDockedBoatCount; ++boatIndex) {
        DrawBoatWithCrew(scene, scene.DockedBoatPositions()[boatIndex],
            scene.DockedBoatRotations()[boatIndex], 0.0f, 0.0f,
            0.0f, 0.88f, static_cast<float>(boatIndex));
    }

    // Small floating offerings follow the same vertical rhythm as the water
    // waves, keeping them just above the canal surface near the altar island.
    for (int offeringIndex = 0; offeringIndex < 8; ++offeringIndex) {
        const float index = static_cast<float>(offeringIndex);
        const float x = 3.2f + static_cast<float>(offeringIndex % 4) * 0.95f;
        const float z = -0.95f - static_cast<float>(offeringIndex / 4) * 1.15f;
        const float bob = 0.055f * std::sin(sceneTime * 1.35f + index * 0.7f);
        DrawObject(shader, scene.Cempasuchil(), glm::vec3(x, 0.04f + bob, z),
            glm::vec3(0.18f + 0.02f * (offeringIndex % 3)), index * 25.0f);
        if (offeringIndex % 2 == 0) {
            DrawObject(shader, scene.Candle(), glm::vec3(x, 0.18f + bob, z),
                glm::vec3(0.16f), -index * 18.0f);
        }
    }

    DrawSpiritParticles(sceneTime);

    if (scene.NightLights()) {
        for (std::size_t i = 0; i < scene.CandleProps().size(); ++i) {
            const float flicker = 0.085f + 0.020f * std::sin(sceneTime * 8.0f + static_cast<float>(i));
            DrawLamp(scene.CandleProps()[i] + glm::vec3(0.0f, 0.22f, 0.0f),
                glm::vec3(1.0f, 0.16f, 0.025f), flicker);
        }
    }
    if (scene.ShowLightMarkers()) {
        for (std::size_t i = 0; i < lights.PointPositions().size(); ++i) {
            DrawLamp(lights.PointPositions()[i], lights.PointColors()[i], 0.20f);
        }
    }
}

void Renderer::DrawGenericIslands(SceneManager& scene) {
    Shader& shader = lightingShader_;
    const float sceneTime = scene.SceneTime();
    const glm::vec3& keyframePosition = scene.KeyframePosition();
    const glm::vec3& keyframeRotation = scene.KeyframeRotation();
    const float keyframeSwing = keyframeRotation.y;
    for (std::size_t islandIndex = 0; islandIndex < scene.GenericIslands().size(); ++islandIndex) {
        const GenericIsland& island = scene.GenericIslands()[islandIndex];
        const glm::mat4 islandRoot = MakeTransform(island.position, island.scale, island.rotationY);
        glUniform1i(glGetUniformLocation(shader.Program, "transparency"), 0);

        DrawObjectWithParent(shader, scene.Island(), islandRoot,
            glm::vec3(0.0f, -0.08f, 0.0f), glm::vec3(1.0f));
        DrawObjectWithParent(shader, scene.IslandEdge(), islandRoot,
            glm::vec3(0.0f, -0.12f, 0.0f), glm::vec3(1.03f, 0.95f, 1.03f));

        // Tree offsets are converted back to local coordinates after clamping
        // their real, scaled XZ distance to <=0.56*min(RX,RZ) (and 0.44 for
        // garden islands). The margin leaves roots and trunks inside the
        // measured island perimeter while keeping the altar trees lateral.
        const std::array<glm::vec3, 5> treeOffsets{
            glm::vec3(-1.15f, 0.08f, -0.72f), glm::vec3(1.05f, 0.08f, -0.80f),
            glm::vec3(-0.78f, 0.08f, 0.92f), glm::vec3(0.96f, 0.08f, 0.78f),
            glm::vec3(0.05f, 0.08f, -0.05f)};
        for (std::size_t i = 0; i < treeOffsets.size(); ++i) {
            if (islandIndex == 0 && i >= 2) continue;
            glm::vec3 local = treeOffsets[i];
            if (islandIndex == 0) {
                // The two altar trees sit behind its rear edge and at opposite
                // sides, leaving the ofrenda silhouette fully visible.
                local = i == 0 ? glm::vec3(-1.18f, 0.08f, -0.65f)
                    : glm::vec3(1.18f, 0.08f, -0.65f);
            } else if (islandIndex == 1) {
                // The convivencia island is a plaza. Keep only three smaller
                // trees at the rear and lateral edges; the central view stays
                // open for the stage, tables and visitors.
                if (i == 0) local = glm::vec3(-1.50f, -0.05f, 1.30f);
                else if (i == 1) local = glm::vec3(1.50f, -0.05f, 1.30f);
                else if (i == 2) local = glm::vec3(-1.60f, -0.05f, -1.20f);
                else continue;
            } else if (islandIndex == 2) {
                // The cabin has a clear rear-center footprint. Three reduced
                // trees frame it from the rear and left side; the former
                // front-right tree is intentionally removed because its crown
                // and roots intersected the gable roof.
                if (i == 0) local = glm::vec3(-1.95f, -0.05f, -1.50f);
                else if (i == 1) local = glm::vec3(1.95f, -0.05f, -1.50f);
                else if (i == 2) local = glm::vec3(-1.95f, -0.05f, 0.48f);
                else continue;
            }
            local.y = -0.05f;
            glm::vec2 scaledOffset(local.x * island.scale.x, local.z * island.scale.z);
            const float maximumDistance = (islandIndex == 0 ? 0.56f
                : (islandIndex == 2 ? 0.72f : 0.44f))
                * std::min(island.radii.x, island.radii.y);
            const float distance = glm::length(scaledOffset);
            if (distance > maximumDistance && distance > 0.001f) {
                scaledOffset *= maximumDistance / distance;
                local.x = scaledOffset.x / island.scale.x;
                local.z = scaledOffset.y / island.scale.z;
            }
            const std::size_t globalTreeIndex = islandIndex * treeOffsets.size() + i;
            const float treeScale = islandIndex == 1 ? 0.72f
                : (islandIndex == 2 ? 0.62f : 1.0f);
            const float height = treeScale * (0.75f
                + 0.05f * static_cast<float>(globalTreeIndex % 13));
            const float width = treeScale * (0.82f
                + 0.035f * static_cast<float>((globalTreeIndex * 3) % 11));
            const float depth = treeScale * (0.78f
                + 0.04f * static_cast<float>((globalTreeIndex * 5) % 9));
            DrawObjectWithParent(shader, scene.Tree(), islandRoot, local,
                glm::vec3(width, height, depth),
                17.0f + static_cast<float>(globalTreeIndex * 43));
        }

        // Every chinampa receives a shoreline belt, tule, stones and flowers.
        for (int perimeterIndex = 0; perimeterIndex < 24; ++perimeterIndex) {
            const float angle = static_cast<float>(perimeterIndex) * 0.2617994f
                + static_cast<float>(islandIndex) * 0.17f;
            const float radiusX = 2.72f + 0.10f * std::sin(angle * 3.0f + islandIndex);
            const float radiusZ = 1.92f + 0.12f * std::cos(angle * 2.0f);
            const glm::vec3 shoreline(radiusX * std::cos(angle), 0.02f,
                radiusZ * std::sin(angle));
            if (perimeterIndex % 2 == 0) {
                DrawObjectWithParent(shader, scene.Reeds(), islandRoot, shoreline,
                    glm::vec3(0.54f + 0.07f * static_cast<float>(perimeterIndex % 3)),
                    glm::degrees(angle));
            }
            if (perimeterIndex % 3 == 0) {
                DrawObjectWithParent(shader, scene.Rock(), islandRoot,
                    shoreline - glm::vec3(0.0f, 0.06f, 0.0f),
                    glm::vec3(0.28f + 0.05f * static_cast<float>(perimeterIndex % 4)),
                    glm::degrees(angle + 0.4f));
            }
            if (perimeterIndex % 4 == 0) {
                DrawObjectWithParent(shader, scene.IslandEdge(), islandRoot,
                    glm::vec3(0.0f, -0.03f, 0.0f),
                    glm::vec3(1.06f, 0.98f, 1.06f));
            }
        }

        // Dense cempasuchil beds: stable variations create an organic scatter.
        for (int flowerIndex = 0; flowerIndex < 18; ++flowerIndex) {
            const float angle = static_cast<float>(flowerIndex) * 2.3999632f
                + static_cast<float>(islandIndex) * 0.23f;
            const float radius = 0.75f + static_cast<float>(flowerIndex % 5) * 0.24f;
            const glm::vec3 flowerPosition(
                radius * std::cos(angle), 0.08f + 0.025f * static_cast<float>(flowerIndex % 3),
                radius * 0.82f * std::sin(angle));
            DrawObjectWithParent(shader, scene.Cempasuchil(), islandRoot,
                flowerPosition,
                glm::vec3(0.11f + 0.018f * static_cast<float>(flowerIndex % 4)),
                glm::degrees(angle));
        }

        const auto LocalToWorld = [&](const glm::vec3& local) {
            return glm::vec3(islandRoot * glm::vec4(local, 1.0f));
        };

        if (islandIndex == 0) {
            // Isla de la ofrenda: altar, papel picado, candles and flowers.
            DrawObjectWithParent(shader, scene.Altar(), islandRoot,
                glm::vec3(0.0f, 0.08f, 0.0f), glm::vec3(0.64f));
            // Finado and Moribunda are the featured sugar-skull pair of the
            // altar. Their four-phase jump is evaluated by SceneManager and
            // both separated parts share the same articulated root here.
            DrawSugarSkulls(scene, islandRoot);
            const std::array<glm::vec3, 8> altarFlowerBeds{
                glm::vec3(-1.45f, 0.34f, -1.00f), glm::vec3(-0.72f, 0.34f, -1.35f),
                glm::vec3(0.10f, 0.34f, -1.45f), glm::vec3(0.92f, 0.34f, -1.25f),
                glm::vec3(1.48f, 0.34f, -0.78f), glm::vec3(-1.50f, 0.12f, 0.88f),
                glm::vec3(1.50f, 0.12f, 0.88f), glm::vec3(0.0f, 0.12f, 1.24f)};
            for (std::size_t flowerIndex = 0; flowerIndex < altarFlowerBeds.size(); ++flowerIndex) {
                DrawObjectWithParent(shader, scene.Cempasuchil(), islandRoot,
                    altarFlowerBeds[flowerIndex],
                    glm::vec3(0.12f + 0.015f * static_cast<float>(flowerIndex % 3)),
                    static_cast<float>(flowerIndex * 41));
            }
            glUniform1i(glGetUniformLocation(shader.Program, "transparency"), 1);
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            glDepthMask(GL_FALSE);
            for (int flagIndex = 0; flagIndex < 4; ++flagIndex) {
                const float flagPhase = scene.PapelPicadoAngle()
                    + 2.0f * std::sin(sceneTime * 3.1f + flagIndex * 0.85f)
                    + keyframeSwing;
                const float flagX = -0.72f + static_cast<float>(flagIndex) * 0.48f;
                DrawObjectWithParent(shader, scene.PapelPicado(), islandRoot,
                    glm::vec3(flagX, 2.05f + 0.05f * std::sin(flagPhase), -1.05f)
                        + keyframePosition,
                    glm::vec3(0.22f, 1.25f, 1.0f), 0.0f, flagPhase);
            }
            glDepthMask(GL_TRUE);
            glDisable(GL_BLEND);
            glUniform1i(glGetUniformLocation(shader.Program, "transparency"), 0);
            DrawLamp(LocalToWorld(glm::vec3(0.0f, 1.1f, 0.0f)),
                glm::vec3(1.25f, 0.38f, 0.08f), 0.13f);
        } else if (islandIndex == 1) {
            // Isla de la convivencia: a clear plaza with a small stage,
            // mariachis, tables, food and visitors around the central space.
            // The wider platform gives the complete articulated band a safe
            // margin: hands and instruments stay over the wood instead of
            // clipping the right edge into the water.
            DrawObjectWithParent(shader, scene.Stage(), islandRoot,
                glm::vec3(0.0f, 0.05f, -1.25f), glm::vec3(1.08f, 0.32f, 0.78f));
            // The keyframe track drives the mariachi group as an articulated
            // ensemble. Their torsos and instruments sway together while each
            // performer keeps a slightly different musical counter-motion.
            const glm::mat4 mariachiKeyframeRoot = MakeTransform(
                keyframePosition, glm::vec3(1.0f), 0.0f, keyframeSwing);
            DrawMariachiBand(scene, islandRoot * mariachiKeyframeRoot);
            const float musicSway = 4.5f * std::sin(sceneTime * 4.0f);
            const float conversationTurn = 12.0f * std::sin(sceneTime * 1.6f);
            const float applause = std::max(0.0f, std::sin(sceneTime * 8.0f));

            const std::array<glm::vec3, 3> tablePositions{
                glm::vec3(-1.15f, 0.08f, 0.35f), glm::vec3(0.0f, 0.08f, 0.75f),
                glm::vec3(1.15f, 0.08f, 0.35f)};
            for (std::size_t tableIndex = 0; tableIndex < tablePositions.size(); ++tableIndex) {
                const glm::vec3& tablePosition = tablePositions[tableIndex];
                DrawObjectWithParent(shader, scene.Table(), islandRoot,
                    tablePosition, glm::vec3(0.58f),
                    tableIndex == 1 ? 0.0f : (tableIndex == 0 ? -12.0f : 12.0f));
                DrawObjectWithParent(shader, scene.Plate(), islandRoot,
                    tablePosition + glm::vec3(-0.12f, 0.39f, 0.02f),
                    glm::vec3(0.22f));
                DrawObjectWithParent(shader, scene.Bread(), islandRoot,
                    tablePosition + glm::vec3(0.14f, 0.40f, 0.02f),
                    glm::vec3(0.16f), static_cast<float>(tableIndex * 21));
                DrawObjectWithParent(shader, scene.Candle(), islandRoot,
                    tablePosition + glm::vec3(0.0f, 0.42f, 0.18f),
                    glm::vec3(0.11f));
            }
            DrawObjectWithMatrix(shader, scene.ConvivenciaPerson1(), islandRoot
                * MakeTransform3D(glm::vec3(-1.55f, 0.08f, 0.35f),
                    glm::vec3(0.32f * (1.0f + 0.025f * applause)),
                    2.0f * std::sin(sceneTime * 3.0f),
                    35.0f + conversationTurn, -musicSway * 0.45f));
            DrawObjectWithMatrix(shader, scene.ConvivenciaPerson2(), islandRoot
                * MakeTransform3D(glm::vec3(1.55f, 0.08f, 0.35f),
                    glm::vec3(0.32f * (1.0f + 0.025f * applause)),
                    -2.0f * std::sin(sceneTime * 3.0f + 1.5f),
                    -35.0f - conversationTurn, musicSway * 0.35f));
            DrawObjectWithMatrix(shader, scene.ConvivenciaPerson3(), islandRoot
                * MakeTransform3D(glm::vec3(0.0f, 0.08f + 0.025f * applause, 1.55f),
                    glm::vec3(0.32f), 7.0f * applause, 180.0f,
                    -musicSway * 0.25f));
            DrawObjectWithParent(shader, scene.Chair(), islandRoot,
                glm::vec3(-1.62f, 0.08f, 0.35f), glm::vec3(0.32f), 90.0f);
            DrawObjectWithParent(shader, scene.Chair(), islandRoot,
                glm::vec3(1.62f, 0.08f, 0.35f), glm::vec3(0.32f), -90.0f);
            DrawLamp(LocalToWorld(glm::vec3(-1.45f, 1.20f, 0.20f)),
                glm::vec3(1.15f, 0.38f, 0.08f), 0.16f);
            DrawLamp(LocalToWorld(glm::vec3(1.45f, 1.20f, 0.20f)),
                glm::vec3(1.15f, 0.38f, 0.08f), 0.16f);
            DrawObjectWithParent(shader, scene.PapelPicado(), islandRoot,
                glm::vec3(0.0f, 2.0f, -2.0f), glm::vec3(1.15f),
                -scene.PapelPicadoAngle());
            glUniform1i(glGetUniformLocation(shader.Program, "transparency"), 0);
        } else if (islandIndex == 2) {
            // Isla de las Muñecas: the cabin is centered in a clear footprint,
            // with its +Z facade facing the access bridge.
            DrawObjectWithParent(shader, scene.Cabin(), islandRoot,
                glm::vec3(0.0f, 0.02f, 0.22f), glm::vec3(0.72f), 0.0f);
            const glm::mat4 dollKeyframeRoot = MakeTransform(
                keyframePosition * 0.70f, glm::vec3(1.0f), 0.0f, keyframeSwing * 0.65f);
            // The doll model origin is the top of its rope. The first four
            // anchors sit under low branches; the last two are just below the
            // cabin's front crossbeam, so the rope never pierces the wood.
            const std::array<glm::vec3, 6> dollPositions{
                glm::vec3(-1.72f, 1.50f, -1.48f),
                glm::vec3(1.72f, 1.50f, -1.48f),
                glm::vec3(-1.88f, 1.27f, 0.48f),
                glm::vec3(-1.56f, 1.55f, 0.48f),
                glm::vec3(-0.40f, 1.00f, 1.05f),
                glm::vec3(0.40f, 1.00f, 1.05f)};
            for (std::size_t dollIndex = 0; dollIndex < dollPositions.size(); ++dollIndex) {
                const float phase = static_cast<float>(dollIndex) * 0.65f;
                const float windX = 5.0f * std::sin(scene.SceneTime() * 1.37f + phase)
                    + 2.0f * std::sin(scene.SceneTime() * 2.91f + phase * 1.7f);
                const float windZ = 7.0f * std::sin(scene.SceneTime() * 1.11f + phase * 1.2f)
                    + 2.5f * std::sin(scene.SceneTime() * 3.17f + phase);
                const float twistY = 6.0f * std::sin(scene.SceneTime() * 0.83f + phase);
                const float bob = 0.025f * std::sin(scene.SceneTime() * 2.2f + phase);
                // The generated asset is authored with its eyes toward +Z,
                // which is the visitor/bridge direction. Keeping 0 degrees
                // here corrects the previous backward-facing placement.
                DrawObjectWithMatrix(shader, scene.HangingDoll(),
                    islandRoot * dollKeyframeRoot * MakeTransform3D(
                        dollPositions[dollIndex] + glm::vec3(0.0f, bob, 0.0f),
                        glm::vec3(0.38f), windX, twistY, windZ));
            }
            // A restrained spectral lamp marks the entrance without washing
            // out the silhouettes of the dolls and the cabin roof.
            DrawLamp(LocalToWorld(glm::vec3(-0.62f, 1.12f, 1.02f)),
                glm::vec3(0.24f, 0.46f, 0.52f), 0.15f);
            DrawLamp(LocalToWorld(glm::vec3(0.62f, 1.12f, 1.02f)),
                glm::vec3(0.24f, 0.46f, 0.52f), 0.15f);
        } else {
            // The remaining chinampas are lively neighborhood gardens.
            DrawObjectWithParent(shader, scene.Table(), islandRoot,
                glm::vec3(-0.55f, 0.08f, 0.15f), glm::vec3(0.52f));
            DrawObjectWithParent(shader, scene.Table(), islandRoot,
                glm::vec3(0.65f, 0.08f, 0.32f), glm::vec3(0.48f), 28.0f);
            DrawObjectWithParent(shader, scene.Chair(), islandRoot,
                glm::vec3(-1.00f, 0.08f, 0.15f), glm::vec3(0.34f), 90.0f);
            DrawObjectWithParent(shader, scene.Chair(), islandRoot,
                glm::vec3(1.08f, 0.08f, 0.32f), glm::vec3(0.34f), -90.0f);
            DrawObjectWithParent(shader, scene.Person(), islandRoot,
                glm::vec3(-0.55f, 0.12f, 0.68f), glm::vec3(0.38f), 155.0f);
            DrawObjectWithParent(shader, scene.Person(), islandRoot,
                glm::vec3(0.68f, 0.12f, -0.15f), glm::vec3(0.38f), -25.0f);
            for (int lampIndex = 0; lampIndex < 3; ++lampIndex) {
                const glm::vec3 localLamp(-1.35f + lampIndex * 1.25f,
                    1.05f, -0.95f + 0.25f * (lampIndex % 2));
                DrawLamp(LocalToWorld(localLamp),
                    glm::vec3(1.25f, 0.38f, 0.08f), 0.12f);
            }
        }
    }
}
