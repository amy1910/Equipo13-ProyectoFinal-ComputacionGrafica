#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <array>
#include <memory>
#include <string>
#include <vector>

#include <windows.h>
#include <mmsystem.h>
#pragma comment(lib, "winmm.lib")

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#include "Camera.h"
#include "MariachiFSM.h"
#include "Model.h"
#include "SceneTypes.h"

struct GenericIsland {
    glm::vec3 position;
    float rotationY;
    glm::vec3 scale;
    glm::vec2 radii;
};

struct CulturalPOI {
    const char* title;
    const char* content;
    const char* keyFact;
};

struct MariachiActor {
    MariachiActor(const char* folder, int instrumentType, const glm::vec3& position,
        float rotationY, float scale, bool hasBow);

    std::unique_ptr<Model> body;
    std::unique_ptr<Model> rightArm;
    std::unique_ptr<Model> leftArm;
    std::unique_ptr<Model> instrument;
    std::unique_ptr<Model> redBowTie;
    std::unique_ptr<Model> bow;
    MariachiFSM fsm;
    int instrumentType;
    glm::vec3 position;
    float rotationY;
    float scale;
    bool hasBow;
};

enum class SugarSkullPhase {
    SideBySide,
    JumpToMoribunda,
    StackedImpact,
    JumpBack
};

struct SugarSkullAnimationPose {
    // Front-left clearing of the ofrenda island, directly visible from the
    // bridge approach.  The pair is intentionally outside the altar footprint.
    glm::vec3 moribundaPosition{-2.82f, 0.10f, 0.18f};
    glm::vec3 finadoPosition{-2.15f, 0.10f, 0.18f};
    glm::vec3 moribundaScale{0.16f};
    glm::vec3 finadoScale{0.16f};
    float moribundaRotationZ = 0.0f;
    float finadoRotationZ = 0.0f;
    SugarSkullPhase phase = SugarSkullPhase::SideBySide;
    float phaseProgress = 0.0f;
};

class SceneManager {
public:
    static constexpr std::size_t kMovingBoatCount = 4;
    static constexpr std::size_t kDockedBoatCount = 6;
    static constexpr std::size_t kMariachiCount = 5;

    SceneManager();
    ~SceneManager();

    void Update(float deltaTime);
    void HandleKey(int key, int action);
    void HandleMouse(double xPosition, double yPosition);
    void GenerarIslaGenerica(glm::vec3 posicion);

    Camera& GetCamera() { return camera_; }
	const Camera& GetCamera() const { return camera_; }
    glm::mat4 ViewMatrix() { return camera_.GetViewMatrix(); }

    float SceneTime() const { return sceneTime_; }
    float PapelPicadoAngle() const { return papelPicadoAngle_; }
    bool NightLights() const { return nightLights_; }
    bool ShowLightMarkers() const { return showLightMarkers_; }
    bool NearOfrenda() const { return nearOfrenda_; }
    bool HasNearbyInterest() const { return nearbyInterest_ != CulturalInterest::None; }
    CulturalInterest NearbyInterest() const { return nearbyInterest_; }
    bool ShowInterestInfo() const { return showInterestInfo_; }
    bool ShowMinimap() const { return showMinimap_; }
    bool ShowHelp() const { return showHelp_; }
    bool AnimationEnabled() const { return animationEnabled_; }
    bool KeyframePlayback() const { return keyframePlayback_; }
    bool MusicEnabled() const { return musicEnabled_; }
    bool MariachiFSMEnabled() const { return mariachiFsmEnabled_; }
    const std::array<glm::vec3, 6>& InterestPositions() const { return interestPositions_; }
    const CulturalPOI& CurrentCulturalPOI() const;
    const glm::vec3& KeyframePosition() const { return keyframePosition_; }
    const glm::vec3& KeyframeRotation() const { return keyframeRotation_; }
    float KeyframeSwing() const { return keyframeSwing_; }
    const SugarSkullAnimationPose& SugarSkulls() const { return sugarSkullPose_; }
    const char* SugarSkullPhaseName() const;
    const std::array<std::unique_ptr<MariachiActor>, kMariachiCount>& Mariachis() const {
        return mariachis_;
    }
    const std::vector<GenericIsland>& GenericIslands() const { return genericIslands_; }

    const glm::vec3& BoatPosition() const { return boatPosition_; }
    const glm::vec3& Boat2Position() const { return boat2Position_; }
    const glm::vec3& Boat3Position() const { return boat3Position_; }
    const glm::vec3& Boat4Position() const { return boat4Position_; }
    float BoatRotation() const { return boatRotation_; }
    float Boat2Rotation() const { return boat2Rotation_; }
    float Boat3Rotation() const { return boat3Rotation_; }
    float Boat4Rotation() const { return boat4Rotation_; }
    const std::array<glm::vec3, kMovingBoatCount>& MovingBoatPositions() const {
        return movingBoatPositions_;
    }
    const std::array<float, kMovingBoatCount>& MovingBoatRotations() const {
        return movingBoatRotations_;
    }
    const std::array<float, kMovingBoatCount>& MovingBoatBanks() const {
        return movingBoatBanks_;
    }
    const std::array<glm::vec3, kDockedBoatCount>& DockedBoatPositions() const {
        return dockedBoatPositions_;
    }
    const std::array<float, kDockedBoatCount>& DockedBoatRotations() const {
        return dockedBoatRotations_;
    }

    const std::array<glm::vec3, 9>& CandleProps() const { return candleProps_; }

    Model& Water() { return water_; }
    Model& Island() { return island_; }
    Model& IslandEdge() { return islandEdge_; }
    Model& Bridge() { return bridge_; }
    Model& Tree() { return tree_; }
    Model& HangingDoll() { return hangingDoll_; }
    Model& Cabin() { return cabin_; }
    Model& Reeds() { return reeds_; }
    Model& Altar() { return altar_; }
    Model& Bread() { return bread_; }
    Model& Plate() { return plate_; }
    Model& Spaghetti() { return spaghetti_; }
    Model& Fire() { return fire_; }
    Model& ObjCandle() { return objCandle_; }
    Model& Cempasuchil() { return cempasuchil_; }
    Model& Candle() { return candle_; }
    Model& PapelPicado() { return papelPicado_; }
    Model& PhotoFrame() { return photoFrame_; }
    Model& Skull() { return skull_; }
    Model& Stage() { return stage_; }
    Model& Musician() { return musician_; }
    Model& Table() { return table_; }
    Model& Chair() { return chair_; }
    Model& Person() { return person_; }
    Model& ConvivenciaPerson1() { return convivenciaPerson1_; }
    Model& ConvivenciaPerson2() { return convivenciaPerson2_; }
    Model& ConvivenciaPerson3() { return convivenciaPerson3_; }
    Model& Trajinera() { return trajinera_; }
    Model& Moon() { return moon_; }
    Model& Rock() { return rock_; }
    Model& MoribundaHead() { return moribundaHead_; }
    Model& MoribundaBody() { return moribundaBody_; }
    Model& FinadoHead() { return finadoHead_; }
    Model& FinadoBody() { return finadoBody_; }

private:
    void DoMovement(float deltaTime);
    void UpdateKeyframes(float deltaTime);
    void UpdateMariachiFSM(float deltaTime);
    void UpdateSugarSkullAnimation(float deltaTime);
    void CheckOfrendaProximity();
    void UpdateCulturalInterest();
    bool IsCameraPositionValid(const glm::vec3& position) const;
    static float SampleKeyframes(const std::array<KeyFrame, 5>& keyframes, float time,
        glm::vec3* position = nullptr, glm::vec3* rotation = nullptr);
    void ResetScene();
    void StartBackgroundMusic();
    void StopBackgroundMusic();
    std::string ResolveBackgroundMusicPath() const;
    static glm::vec3 CatmullRom(const glm::vec3& p0, const glm::vec3& p1,
        const glm::vec3& p2, const glm::vec3& p3, float amount);
    static float CatmullRomScalar(float p0, float p1, float p2, float p3, float amount);
    static float ShortestAngleDelta(float from, float to);
    static float CatmullRomAngle(float a0, float a1, float a2, float a3, float amount);
    void AuditAndRepairBoatRoutes();
    void ResolveMovingBoatConflicts(float deltaTime);

    Camera camera_;
    std::array<bool, 1024> keys_{};
    float lastMouseX_;
    float lastMouseY_;
    bool firstMouse_;

    bool showLightMarkers_;
    bool nightLights_;
    bool animationEnabled_;
    SceneState sceneState_;
    float stateTimer_;
    float sceneTime_;
    float papelPicadoAngle_;
    bool nearOfrenda_;
    CulturalInterest nearbyInterest_ = CulturalInterest::None;
    bool showInterestInfo_ = false;
    bool showMinimap_ = false;
    bool showHelp_ = true;
    std::array<glm::vec3, 6> interestPositions_{};
    std::array<KeyFrame, 5> keyframes_{};
    float keyframeTime_ = 0.0f;
    float keyframeSwing_ = 0.0f;
    glm::vec3 keyframePosition_{0.0f};
    glm::vec3 keyframeRotation_{0.0f};
    bool keyframePlayback_ = true;
    float sugarSkullTime_ = 0.0f;
    SugarSkullAnimationPose sugarSkullPose_{};
    bool musicEnabled_ = true;
    bool musicUsingMci_ = false;
    bool mariachiFsmEnabled_ = true;
    std::string musicPath_;

    static constexpr int kBoatFrameCount = 8;
    std::array<BoatFrame, kBoatFrameCount> boatPath_;
    std::array<BoatFrame, kBoatFrameCount> scenicBoatPath_{};
    std::array<BoatFrame, kBoatFrameCount> exteriorBoatPath_{};
    std::array<BoatFrame, kBoatFrameCount> westernBoatPath_{};
    std::array<glm::vec3, kMovingBoatCount> movingBoatPositions_{};
    std::array<float, kMovingBoatCount> movingBoatRotations_{};
    std::array<float, kMovingBoatCount> movingBoatSpeeds_{};
    std::array<float, kMovingBoatCount> movingBoatPhases_{};
    std::array<float, kMovingBoatCount> movingBoatClocks_{};
    std::array<float, kMovingBoatCount> movingBoatPauseFactors_{};
    std::array<float, kMovingBoatCount> movingBoatBanks_{};
    std::array<int, kMovingBoatCount> movingBoatRouteIndex_{};
    std::array<glm::vec3, kDockedBoatCount> dockedBoatPositions_{};
    std::array<float, kDockedBoatCount> dockedBoatRotations_{};
    glm::vec3 boatPosition_;
    glm::vec3 boat2Position_;
    glm::vec3 boat3Position_;
    glm::vec3 boat4Position_;
    float boatRotation_;
    float boat2Rotation_;
    float boat3Rotation_;
    float boat4Rotation_;
    int boatFrameIndex_;
    float boatFrameTime_;

    std::array<glm::vec3, 9> candleProps_;
    std::vector<GenericIsland> genericIslands_;
    std::array<std::unique_ptr<MariachiActor>, kMariachiCount> mariachis_{};

    Model water_;
    Model island_;
    Model islandEdge_;
    Model bridge_;
    Model tree_;
    Model hangingDoll_;
    Model cabin_;
    Model reeds_;
    Model altar_;
    Model bread_;
    Model plate_;
    Model spaghetti_;
    Model fire_;
    Model objCandle_;
    Model cempasuchil_;
    Model candle_;
    Model papelPicado_;
    Model photoFrame_;
    Model skull_;
    Model stage_;
    Model musician_;
    Model table_;
    Model chair_;
    Model person_;
    Model convivenciaPerson1_;
    Model convivenciaPerson2_;
    Model convivenciaPerson3_;
    Model trajinera_;
    Model moon_;
    Model rock_;
    Model moribundaHead_;
    Model moribundaBody_;
    Model finadoHead_;
    Model finadoBody_;
};
