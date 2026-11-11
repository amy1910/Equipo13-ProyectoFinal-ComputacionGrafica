#include "SceneManager.h"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <utility>

#include <glm/gtc/constants.hpp>

#include "PathUtils.h"

namespace {
constexpr char kBackgroundMusicAlias[] = "xochimilco_background";

const glm::vec3 kOfrendaCenter(16.2f, 1.26f, -9.6f);
const glm::vec3 kConvivenciaCenter(0.0f, 0.80f, 15.6f);
const glm::vec3 kChinampaPoint(-3.0f, 0.80f, 15.6f);
const glm::vec3 kMunecasCenter(-16.2f, 1.00f, -9.6f);
constexpr float kCulturalInteractionRadius = 7.0f;

const CulturalPOI kEmptyCulturalPOI{
    "RECORRIDO XOCHIMILCO", "Acercate a un punto cultural para explorarlo.", ""};
const CulturalPOI kOfrendaPOI{
    "ISLA DE LA OFRENDA: ALTAR DE DIA DE MUERTOS",
    "Representacion sagrada del sincretismo mesoamericano y catolico para honrar el retorno temporal de los fieles difuntos en noviembre. En Xochimilco, la ofrenda se ilumina con veladoras que guian a las animas a traves de la neblina de los canales. Frente al altar aparecen Finado y Moribunda, dos calaveritas de azucar que celebran la memoria con un salto jugueton.",
    "DATOS CLAVE: Integra los cuatro elementos: fuego en las veladoras como luz y guia; aire en el movimiento del papel picado; tierra en los frutos y el cempasuchil; y agua para mitigar la sed del viaje espiritual. Finado y Moribunda representan el encuentro entre humor, artesania y recuerdo."};
const CulturalPOI kConvivenciaPOI{
    "ISLA DE LA CONVIVENCIA Y TRADICION XOCHIMILCA",
    "Espacio dedicado al encuentro social y festivo entre los canales de Xochimilco, Patrimonio Cultural de la Humanidad (UNESCO, 1987). Aqui convergen las trajineras adornadas con nombres propios y arcos florales, acompanadas por musica tradicional, gastronomia lacustre y el sistema agricola ancestral de las chinampas.",
    "DATOS CLAVE: Las trajineras evolucionaron de antiguas canoas de carga chinampera a embarcaciones festivas que fortalecen el tejido comunitario y la identidad del sur de la Cuenca de Mexico."};
const CulturalPOI kChinampaPOI{
    "SUBPUNTO: CHINAMPA Y AGRICULTURA LACUSTRE",
    "Las chinampas son parcelas de cultivo construidas en los humedales con capas vegetales, lodo y vegetacion de la zona. Este sistema productivo mantiene una relacion viva entre agua, suelo, semillas y trabajo comunitario.",
    "DATOS CLAVE: La agricultura chinampera es una tecnologia ancestral y un patrimonio vivo que sostiene la biodiversidad y la memoria agricola de Xochimilco."};
const CulturalPOI kPapelPicadoPOI{
    "SUBPUNTO: PAPEL PICADO Y EL AIRE",
    "El papel picado recorta figuras y simbolos en hojas de colores para decorar calles, patios y ofrendas. Su movimiento hace visible el aire y convierte la celebracion en una experiencia dinamica, delicada y comunitaria.",
    "DATOS CLAVE: En la ofrenda representa el aire y recuerda que la vida, la memoria y la fiesta siguen moviendose entre quienes estan presentes y quienes regresan."};
const CulturalPOI kCempasuchilPOI{
    "SUBPUNTO: CEMPASUCHIL Y CAMINO DE LAS ALMAS",
    "El cempasuchil aporta color y aroma a la ofrenda. Sus petalos se colocan como senderos para orientar a las animas hacia la luz, los alimentos y la memoria de sus familias.",
    "DATOS CLAVE: Su presencia vincula la tierra, los ciclos de cultivo y la continuidad de la vida con la celebracion de Dia de Muertos."};
const CulturalPOI kMunecasPOI{
    "ISLA DE LAS MUNECAS: LEYENDA Y TRADICION ORAL",
    "Originada a mediados del siglo XX en la chinampa de Don Julian Santana Barrera, quien comenzo a colgar munecas recolectadas en los canales como amuleto protector tras el supuesto ahogamiento de una joven en las aguas cercanas.",
    "DATOS CLAVE: Hoy es uno de los referentes del folclore y misticismo lacustre de Xochimilco. Con el paso del tiempo, el sol, la humedad y el viento transformaron las munecas en un paisaje simbolico entre el respeto a los espiritus del canal y la memoria popular."};

const CulturalPOI& CulturalPOIFor(CulturalInterest interest) {
    switch (interest) {
    case CulturalInterest::Altar: return kOfrendaPOI;
    case CulturalInterest::Trajinera: return kConvivenciaPOI;
    case CulturalInterest::Chinampa: return kChinampaPOI;
    case CulturalInterest::PapelPicado: return kPapelPicadoPOI;
    case CulturalInterest::Cempasuchil: return kCempasuchilPOI;
    case CulturalInterest::Munecas: return kMunecasPOI;
    default: return kEmptyCulturalPOI;
    }
}
// Measured from ConfigInicial/Models/trajinera.obj (X = 2.0677, Z = 4.1969).
constexpr float kBoatHalfLength = 2.09845f;
constexpr float kBoatHalfWidth = 1.03385f;
constexpr float kRouteClearance = 3.5f;
constexpr float kDockedBoatClearance = 2.5f;
constexpr int kRouteSamples = 300;

struct BridgeAudit {
    glm::vec2 min;
    glm::vec2 max;
};

// These are conservative XZ rectangles around every visible bridge. They are
// expanded during the route audit by the full boat safety margin.
const std::array<BridgeAudit, 5> kBridgeAudits{
    BridgeAudit{glm::vec2(-1.85f, -3.75f), glm::vec2(1.85f, -2.65f)},
    BridgeAudit{glm::vec2(-5.00f, -0.35f), glm::vec2(-1.50f, 3.35f)},
    BridgeAudit{glm::vec2(1.50f, -0.35f), glm::vec2(5.00f, 3.35f)},
    BridgeAudit{glm::vec2(-19.20f, 13.20f), glm::vec2(-15.20f, 15.80f)},
    BridgeAudit{glm::vec2(15.20f, 13.20f), glm::vec2(19.20f, 15.80f)}};
}

MariachiActor::MariachiActor(const char* folder, int instrumentType,
    const glm::vec3& position, float rotationY, float scale, bool hasBow)
    : instrumentType(instrumentType), position(position), rotationY(rotationY),
      scale(scale), hasBow(hasBow) {
    const std::string root = std::string("Models/Mariachis/") + folder + "/";
    const auto load = [&root](const char* part) {
        const std::string path = root + part + ".obj";
        return std::make_unique<Model>(path.c_str());
    };

    body = load("cuerpo_sombrero");
    rightArm = load("brazo_derecho");
    leftArm = load("brazo_izquierdo");
    instrument = load("instrumento");
    redBowTie = load("mono_rojo");
    if (hasBow) bow = load("arco");
}

SceneManager::SceneManager()
    : camera_(glm::vec3(0.0f, 24.0f, 55.5f)),
      lastMouseX_(640.0f), lastMouseY_(400.0f), firstMouse_(true),
      showLightMarkers_(false), nightLights_(true), animationEnabled_(true),
      sceneState_(SceneState::Intro), stateTimer_(0.0f), sceneTime_(0.0f),
      papelPicadoAngle_(0.0f), nearOfrenda_(false),
      boatPath_{
          // The model sits on the water surface (Y is the vertical axis in the project).
          BoatFrame{glm::vec3(-25.0f, -0.02f, -24.0f), 90.0f, 3.8f},
          BoatFrame{glm::vec3(-25.0f, -0.02f, -8.0f), 64.0f, 3.8f},
          BoatFrame{glm::vec3(-20.0f, -0.02f, 8.0f), 28.0f, 3.8f},
          BoatFrame{glm::vec3(-15.0f, -0.02f, 22.0f), 4.0f, 3.8f},
          BoatFrame{glm::vec3(-2.0f, -0.02f, 25.0f), -12.0f, 3.8f},
          BoatFrame{glm::vec3(13.0f, -0.02f, 24.0f), -36.0f, 3.8f},
          BoatFrame{glm::vec3(25.0f, -0.02f, 10.0f), -74.0f, 3.8f},
          BoatFrame{glm::vec3(25.0f, -0.02f, -24.0f), -90.0f, 3.8f}},
      boatPosition_(boatPath_[0].position), boat2Position_(boatPath_[3].position),
      boat3Position_(boatPath_[1].position), boat4Position_(boatPath_[4].position),
      boatRotation_(boatPath_[0].rotationY), boat2Rotation_(boatPath_[3].rotationY),
      boat3Rotation_(boatPath_[1].rotationY), boat4Rotation_(boatPath_[4].rotationY),
      boatFrameIndex_(0), boatFrameTime_(0.0f),
      candleProps_{
          glm::vec3(4.45f, 0.82f, -3.05f), glm::vec3(6.35f, 0.82f, -3.35f),
          glm::vec3(5.4f, 1.15f, -3.2f), glm::vec3(4.95f, 1.15f, -3.2f),
          glm::vec3(5.85f, 1.15f, -3.2f), glm::vec3(-5.2f, 0.08f, -1.8f),
          glm::vec3(-6.5f, 0.08f, -4.2f), glm::vec3(0.0f, 0.08f, 5.0f),
          glm::vec3(1.5f, 0.08f, 5.7f)},
      water_((char*)"Models/agua.obj"),
      island_((char*)"Models/isla_chinampa_corregida.obj"),
      islandEdge_((char*)"Models/borde_isla.obj"), bridge_((char*)"Models/puente_madera.obj"),
      tree_((char*)"Models/arbol.obj"), hangingDoll_((char*)"Models/muneca_colgada.obj"),
      cabin_((char*)"Models/cabana_munecas.obj"), reeds_((char*)"Models/tule.obj"),
      altar_((char*)"Models/ofrenda.obj"), bread_((char*)"Models/bread.obj"),
      plate_((char*)"Models/Plate.obj"), spaghetti_((char*)"Models/espagueti.obj"),
      fire_((char*)"Models/fuego.obj"), objCandle_((char*)"Models/objCandle.obj"),
      cempasuchil_((char*)"Models/cempasuchil.obj"),
      candle_((char*)"Models/veladora.obj"), papelPicado_((char*)"Models/papel_picado.obj"),
      photoFrame_((char*)"Models/marco_fotografia.obj"), skull_((char*)"Models/calavera.obj"),
      stage_((char*)"Models/escenario_musica.obj"), musician_((char*)"Models/musico.obj"),
      table_((char*)"Models/mesa.obj"), chair_((char*)"Models/silla.obj"),
      person_((char*)"Models/persona.obj"),
      convivenciaPerson1_((char*)"Models/persona_convivencia_1.obj"),
      convivenciaPerson2_((char*)"Models/persona_convivencia_2.obj"),
      convivenciaPerson3_((char*)"Models/persona_convivencia_3.obj"),
      trajinera_((char*)"Models/trajinera.obj"),
      moon_((char*)"Models/luna.obj"), rock_((char*)"Models/piedra.obj"),
      moribundaHead_((char*)"Models/FinadoMoribunda/moribunda_cabeza.obj"),
      moribundaBody_((char*)"Models/FinadoMoribunda/moribunda_cuerpo.obj"),
      finadoHead_((char*)"Models/FinadoMoribunda/finado_cabeza.obj"),
      finadoBody_((char*)"Models/FinadoMoribunda/finado_cuerpo.obj") {
    camera_.ProcessMouseMovement(0.0f, -75.0f);
    // Twelve irregular chinampas form a distributed archipelago and leave
    // water corridors between the center and the four map edges.
    const std::array<glm::vec3, 12> islandPositions{
        glm::vec3(5.4f, 0.0f, -3.2f), glm::vec3(0.0f, 0.0f, 5.2f),
        glm::vec3(-5.4f, 0.0f, -3.2f), glm::vec3(-14.0f, 0.0f, -11.0f),
        glm::vec3(14.0f, 0.0f, -11.0f), glm::vec3(-14.0f, 0.0f, 11.0f),
        glm::vec3(14.0f, 0.0f, 11.0f), glm::vec3(-21.0f, 0.0f, -2.0f),
        glm::vec3(21.0f, 0.0f, 2.0f), glm::vec3(-4.0f, 0.0f, -15.0f),
        glm::vec3(4.0f, 0.0f, 15.0f), glm::vec3(20.0f, 0.0f, 19.0f)};
    for (const glm::vec3& position : islandPositions) GenerarIslaGenerica(position);

    // Four independent, long Catmull-Rom routes. Their control points create
    // S-curves, sharp reversals and edge-to-edge crossings instead of a center loop.
    scenicBoatPath_ = {
        BoatFrame{glm::vec3(-12.0f, -0.02f, 24.0f), 12.0f, 3.8f},
        BoatFrame{glm::vec3(-7.0f, -0.02f, 13.0f), 42.0f, 3.6f},
        BoatFrame{glm::vec3(-12.0f, -0.02f, 3.0f), 122.0f, 3.4f},
        BoatFrame{glm::vec3(-8.0f, -0.02f, -8.0f), 54.0f, 3.7f},
        BoatFrame{glm::vec3(-2.0f, -0.02f, -24.0f), -8.0f, 3.8f},
        BoatFrame{glm::vec3(8.0f, -0.02f, -20.0f), -48.0f, 3.9f},
        BoatFrame{glm::vec3(12.0f, -0.02f, -8.0f), -118.0f, 3.8f},
        BoatFrame{glm::vec3(9.0f, -0.02f, 12.0f), -34.0f, 3.8f}};
    exteriorBoatPath_ = {
        BoatFrame{glm::vec3(24.0f, -0.02f, -22.0f), 154.0f, 4.8f},
        BoatFrame{glm::vec3(14.0f, -0.02f, -24.0f), 130.0f, 4.8f},
        BoatFrame{glm::vec3(4.0f, -0.02f, -19.0f), 76.0f, 4.6f},
        BoatFrame{glm::vec3(-8.0f, -0.02f, -22.0f), 22.0f, 4.8f},
        BoatFrame{glm::vec3(-24.0f, -0.02f, -17.0f), -24.0f, 4.8f},
        BoatFrame{glm::vec3(-24.0f, -0.02f, -2.0f), 46.0f, 4.6f},
        BoatFrame{glm::vec3(-22.0f, -0.02f, 14.0f), 112.0f, 4.8f},
        BoatFrame{glm::vec3(-12.0f, -0.02f, 22.0f), 162.0f, 4.8f}};
    westernBoatPath_ = {
        BoatFrame{glm::vec3(-24.0f, -0.02f, 18.0f), -38.0f, 4.6f},
        BoatFrame{glm::vec3(-16.0f, -0.02f, 14.0f), -74.0f, 4.7f},
        BoatFrame{glm::vec3(-10.0f, -0.02f, 6.0f), -132.0f, 4.4f},
        BoatFrame{glm::vec3(-17.0f, -0.02f, -1.0f), 176.0f, 4.5f},
        BoatFrame{glm::vec3(-21.0f, -0.02f, -10.0f), 132.0f, 4.6f},
        BoatFrame{glm::vec3(-12.0f, -0.02f, -17.0f), 70.0f, 4.7f},
        BoatFrame{glm::vec3(-1.0f, -0.02f, -20.0f), 24.0f, 4.4f},
        BoatFrame{glm::vec3(10.0f, -0.02f, -15.0f), -26.0f, 4.5f}};
    movingBoatRouteIndex_ = {0, 1, 2, 3};
    // Slow ceremonial speeds, expressed in route-control segments per second.
    movingBoatSpeeds_ = {0.115f, 0.105f, 0.095f, 0.110f};
    movingBoatPhases_ = {0.0f, 2.1f, 4.2f, 5.4f};
    movingBoatClocks_ = movingBoatPhases_;
    movingBoatPauseFactors_.fill(1.0f);
    movingBoatBanks_.fill(0.0f);

    // Docked boats use the measured island radii and the measured trajinera
    // half-width: distance = max(RX,RZ) + halfWidth + 0.8.
    const std::array<std::size_t, kDockedBoatCount> dockedIslandIndices{0, 1, 4, 5, 7, 10};
    const std::array<float, kDockedBoatCount> dockedAngles{0.0f, 90.0f, 206.0f, 28.0f, 180.0f, 300.0f};
    for (std::size_t boatIndex = 0; boatIndex < kDockedBoatCount; ++boatIndex) {
        const GenericIsland& island = genericIslands_[dockedIslandIndices[boatIndex]];
        const float angle = glm::radians(dockedAngles[boatIndex]);
        const glm::vec2 radial(std::cos(angle), std::sin(angle));
        const float dockDistance = std::max(island.radii.x, island.radii.y)
            + kBoatHalfWidth * 0.88f + 0.8f;
        dockedBoatPositions_[boatIndex] = glm::vec3(
            island.position.x + radial.x * dockDistance, -0.02f,
            island.position.z + radial.y * dockDistance);
        dockedBoatRotations_[boatIndex] = dockedAngles[boatIndex] + 90.0f;
    }
    AuditAndRepairBoatRoutes();
    movingBoatPositions_ = {
        boatPath_[0].position, scenicBoatPath_[0].position,
        exteriorBoatPath_[0].position, westernBoatPath_[0].position};
    movingBoatRotations_ = {
        boatPath_[0].rotationY, scenicBoatPath_[0].rotationY,
        exteriorBoatPath_[0].rotationY, westernBoatPath_[0].rotationY};
    interestPositions_ = {
        kOfrendaCenter,
        kConvivenciaCenter,
        kChinampaPoint,
        glm::vec3(16.2f, 6.2f, -13.5f),
        glm::vec3(13.0f, 1.0f, -9.6f),
        kMunecasCenter};
    keyframes_ = {
        KeyFrame{0.0f, glm::vec3(0.0f, 0.00f, 0.0f), glm::vec3(0.0f, -12.0f, 0.0f)},
        KeyFrame{1.0f, glm::vec3(0.0f, 0.08f, 0.0f), glm::vec3(0.0f, 9.0f, 0.0f)},
        KeyFrame{2.0f, glm::vec3(0.0f, 0.14f, 0.0f), glm::vec3(0.0f, -7.0f, 0.0f)},
        KeyFrame{3.0f, glm::vec3(0.0f, 0.05f, 0.0f), glm::vec3(0.0f, 11.0f, 0.0f)},
        KeyFrame{4.0f, glm::vec3(0.0f, 0.00f, 0.0f), glm::vec3(0.0f, -12.0f, 0.0f)}};
    keyframePosition_ = keyframes_[0].position;
    keyframeRotation_ = keyframes_[0].rotation;
    keyframeSwing_ = keyframeRotation_.y;
    sugarSkullTime_ = 0.0f;
    sugarSkullPose_ = SugarSkullAnimationPose{};

    // The new band is made from articulated OBJ components. All paths are
    // relative to ConfigInicial so Debug, Release and the IDE use the same
    // portable asset tree.
    mariachis_[0] = std::make_unique<MariachiActor>(
        "mariachi_1_violin", 0, glm::vec3(-0.80f, 0.13f, -1.18f), 0.0f, 0.30f, true);
    mariachis_[1] = std::make_unique<MariachiActor>(
        "mariachi_2_violin", 0, glm::vec3(-0.40f, 0.13f, -1.18f), 0.0f, 0.30f, true);
    mariachis_[2] = std::make_unique<MariachiActor>(
        "mariachi_3_trumpet", 2, glm::vec3(0.0f, 0.13f, -1.18f), 0.0f, 0.30f, false);
    mariachis_[3] = std::make_unique<MariachiActor>(
        "mariachi_4_violin", 0, glm::vec3(0.40f, 0.13f, -1.18f), 0.0f, 0.30f, true);
    mariachis_[4] = std::make_unique<MariachiActor>(
        "mariachi_5_guitar", 1, glm::vec3(0.80f, 0.13f, -1.18f), 0.0f, 0.30f, false);

    water_.OverrideDiffuseTexture("Models/agua_realista.png");
    island_.OverrideDiffuseTexture("Models/isla_diffuse.png");
    islandEdge_.OverrideDiffuseTexture("Models/borde_isla_diffuse.png");
    bridge_.OverrideDiffuseTexture("Models/puente_madera_diffuse.png");
    tree_.OverrideDiffuseTexture("Models/arbol_diffuse.png");
    // The Muñecas assets use their Blender-exported MTL materials directly:
    // the new cabin and dolls have several distinct wood, rope, dress and eye
    // colors, so overriding them with one legacy texture would flatten them.
    reeds_.OverrideDiffuseTexture("Models/tule_diffuse.png");
    // ofrenda.obj carries multiple MTL materials; keep its per-mesh colors.
    bread_.OverrideDiffuseTexture("Models/bread_diffuse.png");
    plate_.OverrideDiffuseTexture("Models/plate_diffuse.png");
    spaghetti_.OverrideDiffuseTexture("Models/espagueti_diffuse.png");
    fire_.OverrideDiffuseTexture("Models/fuego_diffuse.png");
    objCandle_.OverrideDiffuseTexture("Models/objCandle_diffuse.png");
    // cempasuchil.obj carries separate green foliage and orange flower materials.
    candle_.OverrideDiffuseTexture("Models/veladora_diffuse.png");
    papelPicado_.OverrideDiffuseTexture("Models/papel_picado_diffuse.png");
    photoFrame_.OverrideDiffuseTexture("Models/marco_fotografia_diffuse.png");
    skull_.OverrideDiffuseTexture("Models/calavera_diffuse.png");
    stage_.OverrideDiffuseTexture("Models/escenario_musica_diffuse.png");
    musician_.OverrideDiffuseTexture("Models/musico_diffuse.png");
    table_.OverrideDiffuseTexture("Models/mesa_diffuse.png");
    chair_.OverrideDiffuseTexture("Models/silla_diffuse.png");
    person_.OverrideDiffuseTexture("Models/persona_diffuse.png");
    trajinera_.OverrideDiffuseTexture("Models/trajinera_diffuse.png");
    moon_.OverrideDiffuseTexture("Models/luna_diffuse.png");
    rock_.OverrideDiffuseTexture("Models/piedra_diffuse.png");
    fire_.SetEmissive(glm::vec3(1.0f, 0.055f, 0.005f), 15.0f);
    objCandle_.SetEmissive(glm::vec3(1.0f, 0.16f, 0.02f), 9.0f);
    candle_.SetEmissive(glm::vec3(1.0f, 0.12f, 0.01f), 5.0f);
    moon_.SetEmissive(glm::vec3(0.22f, 0.38f, 1.0f), 7.0f);
    StartBackgroundMusic();
}

SceneManager::~SceneManager() {
    StopBackgroundMusic();
}

std::string SceneManager::ResolveBackgroundMusicPath() const {
    // Keep the source-tree path as an anchor, then cover both the VS working
    // directory and the three-level hop from bin/Win32/{Debug,Release}.
    const std::string sourceFile(__FILE__);
    const std::size_t sourceSlash = sourceFile.find_last_of("\\/");
    const std::string sourceDirectory = sourceSlash == std::string::npos
        ? std::string() : sourceFile.substr(0, sourceSlash);
    const std::string executable = ExecutableDirectory();
    const std::array<std::string, 6> candidates{
        sourceDirectory + "\\Audio\\xochimilco_fondo.wav",
        sourceDirectory + "\\Audio\\La_Llorona_extraida.mp3",
        executable + "\\..\\..\\..\\ConfigInicial\\Audio\\xochimilco_fondo.wav",
        executable + "\\..\\..\\..\\ConfigInicial\\Audio\\La_Llorona_extraida.mp3",
        ResolveAssetPath("Audio/xochimilco_fondo.wav"),
        ResolveAssetPath("Audio/La_Llorona_extraida.mp3")};
    for (const std::string& candidate : candidates) {
        if (AssetFileExists(candidate)) return candidate;
    }
    return std::string();
}

void SceneManager::StopBackgroundMusic() {
    if (musicUsingMci_) {
        mciSendStringA("stop xochimilco_background", nullptr, 0, nullptr);
        mciSendStringA("close xochimilco_background", nullptr, 0, nullptr);
        musicUsingMci_ = false;
    } else {
        mciSendStringA("close xochimilco_background", nullptr, 0, nullptr);
    }
    PlaySoundA(nullptr, nullptr, 0);
}

void SceneManager::StartBackgroundMusic() {
    if (!musicEnabled_) return;
    StopBackgroundMusic();
    musicPath_ = ResolveBackgroundMusicPath();
    if (musicPath_.empty()) {
        std::cerr << "Musica de fondo no encontrada. Se esperaba "
            << "ConfigInicial/Audio/xochimilco_fondo.wav o un MP3 compatible.\n";
        return;
    }

    const std::size_t extensionPosition = musicPath_.find_last_of('.');
    const std::string extension = extensionPosition == std::string::npos
        ? std::string() : musicPath_.substr(extensionPosition);
    if (extension == ".wav" || extension == ".WAV") {
        if (PlaySoundA(musicPath_.c_str(), nullptr,
            SND_FILENAME | SND_ASYNC | SND_LOOP) != FALSE) {
            std::cout << "Musica de fondo: " << musicPath_ << " (loop WAV).\n";
            return;
        }
        std::cerr << "No se pudo iniciar el WAV con PlaySoundA: " << musicPath_ << "\n";
        return;
    }

    const std::string openCommand = "open \"" + musicPath_
        + "\" type mpegvideo alias " + kBackgroundMusicAlias;
    if (mciSendStringA(openCommand.c_str(), nullptr, 0, nullptr) != 0) {
        std::cerr << "No se pudo abrir el audio con MCI: " << musicPath_ << "\n";
        return;
    }
    const std::string playCommand = "play " + std::string(kBackgroundMusicAlias) + " repeat";
    if (mciSendStringA(playCommand.c_str(), nullptr, 0, nullptr) == 0) {
        musicUsingMci_ = true;
        std::cout << "Musica de fondo: " << musicPath_ << " (loop MCI).\n";
    } else {
        mciSendStringA("close xochimilco_background", nullptr, 0, nullptr);
        std::cerr << "No se pudo reproducir el audio con MCI: " << musicPath_ << "\n";
    }
}

void SceneManager::AuditAndRepairBoatRoutes() {
    const auto repairPoint = [&](glm::vec2 point) {
        for (const GenericIsland& island : genericIslands_) {
            const glm::vec2 center(island.position.x, island.position.z);
            const float minimumDistance = std::max(island.radii.x, island.radii.y)
                + kBoatHalfLength + kRouteClearance;
            glm::vec2 delta = point - center;
            float distance = glm::length(delta);
            if (distance < minimumDistance) {
                if (distance < 0.001f) delta = glm::vec2(1.0f, 0.0f);
                else delta /= distance;
                point = center + delta * minimumDistance;
            }
        }
        for (const BridgeAudit& bridge : kBridgeAudits) {
            const float expand = kBoatHalfLength + 0.75f;
            const glm::vec2 minPoint = bridge.min - glm::vec2(expand);
            const glm::vec2 maxPoint = bridge.max + glm::vec2(expand);
            if (point.x > minPoint.x && point.x < maxPoint.x &&
                point.y > minPoint.y && point.y < maxPoint.y) {
                const float pushLeft = point.x - minPoint.x;
                const float pushRight = maxPoint.x - point.x;
                const float pushDown = point.y - minPoint.y;
                const float pushUp = maxPoint.y - point.y;
                const float smallest = std::min(std::min(pushLeft, pushRight),
                    std::min(pushDown, pushUp));
                if (smallest == pushLeft) point.x = minPoint.x;
                else if (smallest == pushRight) point.x = maxPoint.x;
                else if (smallest == pushDown) point.y = minPoint.y;
                else point.y = maxPoint.y;
            }
        }
        for (const glm::vec3& docked : dockedBoatPositions_) {
            const glm::vec2 dockedPoint(docked.x, docked.z);
            const float minimumDistance = 2.0f * kBoatHalfLength + kDockedBoatClearance;
            glm::vec2 delta = point - dockedPoint;
            float distance = glm::length(delta);
            if (distance < minimumDistance) {
                if (distance < 0.001f) delta = glm::vec2(0.0f, 1.0f);
                else delta /= distance;
                point = dockedPoint + delta * minimumDistance;
            }
        }
        return point;
    };

    const auto auditRoute = [&](std::array<BoatFrame, kBoatFrameCount>& route) {
        // First clear every control point, then resample all 300 curve samples.
        // Offending samples push the nearest control point and are re-audited.
        for (int pass = 0; pass < 16; ++pass) {
            for (BoatFrame& frame : route) {
                const glm::vec2 repaired = repairPoint(glm::vec2(frame.position.x, frame.position.z));
                frame.position.x = repaired.x;
                frame.position.z = repaired.y;
            }
            bool routeIsSafe = true;
            for (int sample = 0; sample < kRouteSamples; ++sample) {
                const float wrapped = static_cast<float>(sample) / static_cast<float>(kRouteSamples)
                    * static_cast<float>(kBoatFrameCount);
                const int index = static_cast<int>(std::floor(wrapped)) % kBoatFrameCount;
                const float amount = wrapped - static_cast<float>(index);
                const int previous = (index + kBoatFrameCount - 1) % kBoatFrameCount;
                const int next = (index + 1) % kBoatFrameCount;
                const int nextNext = (index + 2) % kBoatFrameCount;
                const glm::vec3 sampled = CatmullRom(route[previous].position, route[index].position,
                    route[next].position, route[nextNext].position, amount);
                const glm::vec2 repaired = repairPoint(glm::vec2(sampled.x, sampled.z));
                const glm::vec2 correction = repaired - glm::vec2(sampled.x, sampled.z);
                if (glm::length(correction) > 0.01f) {
                    routeIsSafe = false;
                    const int controlIndex = (index + 1) % kBoatFrameCount;
                    route[controlIndex].position.x += correction.x * 0.70f;
                    route[controlIndex].position.z += correction.y * 0.70f;
                }
            }
            if (routeIsSafe) break;
        }
    };

    auditRoute(boatPath_);
    auditRoute(scenicBoatPath_);
    auditRoute(exteriorBoatPath_);
    auditRoute(westernBoatPath_);
}

void SceneManager::GenerarIslaGenerica(glm::vec3 posicion) {
    // isla.obj measured bounds: X size 7.1895, Z size 5.1707. Each island
    // gets a distinct asymmetric scale in the requested 1.08x..1.25x range.
    const std::array<glm::vec3, 12> scales{
        glm::vec3(1.16f, 0.91f, 1.10f), glm::vec3(1.22f, 0.88f, 1.08f),
        glm::vec3(1.11f, 0.94f, 1.20f), glm::vec3(1.08f, 0.90f, 1.18f),
        glm::vec3(1.24f, 0.92f, 1.10f), glm::vec3(1.15f, 0.87f, 1.25f),
        glm::vec3(1.20f, 0.95f, 1.09f), glm::vec3(1.09f, 0.89f, 1.22f),
        glm::vec3(1.18f, 0.93f, 1.12f), glm::vec3(1.23f, 0.90f, 1.14f),
        glm::vec3(1.10f, 0.96f, 1.21f), glm::vec3(1.25f, 0.88f, 1.09f)};
    const std::array<float, 12> rotations{18.0f, -24.0f, 37.0f, -12.0f,
        62.0f, -48.0f, 29.0f, 78.0f, -35.0f, 11.0f, -67.0f, 44.0f};
    const std::size_t islandIndex = genericIslands_.size();
    const std::size_t safeIndex = std::min(islandIndex, scales.size() - 1);
    const glm::vec3 scale = scales[safeIndex];
    const glm::vec2 radii(3.59475f * scale.x, 2.58535f * scale.z);
    genericIslands_.push_back(GenericIsland{
        posicion, rotations[safeIndex], scale, radii});
}

void SceneManager::Update(float deltaTime) {
    const float clampedDelta = std::min(deltaTime, 0.05f);
    sceneTime_ += clampedDelta;
    // Two inharmonic waves make the papel picado look like a breeze travelling
    // from one side of the altar to the other instead of a single rigid swing.
    papelPicadoAngle_ = 5.0f * std::sin(sceneTime_ * 2.5f)
        + 1.6f * std::sin(sceneTime_ * 5.3f + 0.75f);
    DoMovement(clampedDelta);
    CheckOfrendaProximity();
    UpdateCulturalInterest();
    UpdateKeyframes(clampedDelta);
    UpdateSugarSkullAnimation(clampedDelta);
    UpdateMariachiFSM(clampedDelta);
}

void SceneManager::DoMovement(float deltaTime) {
    const glm::vec3 previousPosition = camera_.GetPosition();
    if (keys_[GLFW_KEY_W] || keys_[GLFW_KEY_UP]) camera_.ProcessKeyboard(FORWARD, deltaTime);
    if (keys_[GLFW_KEY_S] || keys_[GLFW_KEY_DOWN]) camera_.ProcessKeyboard(BACKWARD, deltaTime);
    if (keys_[GLFW_KEY_A] || keys_[GLFW_KEY_LEFT]) camera_.ProcessKeyboard(LEFT, deltaTime);
    if (keys_[GLFW_KEY_D] || keys_[GLFW_KEY_RIGHT]) camera_.ProcessKeyboard(RIGHT, deltaTime);

    glm::vec3 candidate = camera_.GetPosition();
    candidate.y = glm::clamp(candidate.y, 0.75f, 28.0f);
    if (IsCameraPositionValid(candidate)) {
        camera_.SetPosition(candidate);
    } else {
        camera_.SetPosition(previousPosition);
    }
}

void SceneManager::CheckOfrendaProximity() {
    const float distanceToOfrenda = glm::distance(camera_.GetPosition(), kOfrendaCenter);
    nearOfrenda_ = distanceToOfrenda < kCulturalInteractionRadius;
}

void SceneManager::UpdateCulturalInterest() {
    interestPositions_[0] = kOfrendaCenter;
    interestPositions_[1] = kConvivenciaCenter;
    interestPositions_[2] = kChinampaPoint;
    interestPositions_[3] = glm::vec3(16.2f, 6.2f, -13.5f);
    interestPositions_[4] = glm::vec3(13.0f, 1.0f, -9.6f);
    interestPositions_[5] = kMunecasCenter;

    const glm::vec3 cameraPosition = camera_.GetPosition();
    float closestDistance = kCulturalInteractionRadius;
    CulturalInterest closest = CulturalInterest::None;
    for (int index = 0; index < static_cast<int>(interestPositions_.size()); ++index) {
        const float distance = glm::distance(cameraPosition, interestPositions_[index]);
        if (distance < closestDistance) {
            closestDistance = distance;
            closest = static_cast<CulturalInterest>(index);
        }
    }
    if (closest != nearbyInterest_) showInterestInfo_ = false;
    nearbyInterest_ = closest;
    nearOfrenda_ = nearbyInterest_ == CulturalInterest::Altar
        || nearbyInterest_ == CulturalInterest::PapelPicado
        || nearbyInterest_ == CulturalInterest::Cempasuchil;
}

const CulturalPOI& SceneManager::CurrentCulturalPOI() const {
    return CulturalPOIFor(nearbyInterest_);
}

bool SceneManager::IsCameraPositionValid(const glm::vec3& position) const {
    if (std::abs(position.x) > 96.0f || std::abs(position.z) > 96.0f) return false;
    // The intro camera starts above the scene; collision tests begin after it descends.
    if (position.y > 6.0f) return true;

    const glm::vec2 point(position.x, position.z);
    const std::array<std::pair<glm::vec2, float>, 8> obstacles{
        std::make_pair(glm::vec2(16.2f, -9.6f), 2.8f),   // altar
        std::make_pair(glm::vec2(-16.8f, -10.2f), 2.5f), // cabin
        std::make_pair(glm::vec2(11.4f, -14.7f), 1.5f),
        std::make_pair(glm::vec2(22.0f, -14.7f), 1.5f),
        std::make_pair(glm::vec2(-21.3f, -12.9f), 1.4f),
        std::make_pair(glm::vec2(-11.4f, -13.5f), 1.4f),
        std::make_pair(glm::vec2(8.1f, 16.8f), 1.4f),
        std::make_pair(glm::vec2(-8.1f, 16.8f), 1.4f)};
    for (const auto& obstacle : obstacles) {
        if (glm::distance(point, obstacle.first) < obstacle.second) return false;
    }
    return true;
}

glm::vec3 SceneManager::CatmullRom(const glm::vec3& p0, const glm::vec3& p1,
    const glm::vec3& p2, const glm::vec3& p3, float amount) {
    const float t2 = amount * amount;
    const float t3 = t2 * amount;
    return 0.5f * (2.0f * p1 + (-p0 + p2) * amount
        + (2.0f * p0 - 5.0f * p1 + 4.0f * p2 - p3) * t2
        + (-p0 + 3.0f * p1 - 3.0f * p2 + p3) * t3);
}

float SceneManager::CatmullRomScalar(float p0, float p1, float p2, float p3, float amount) {
    const float t2 = amount * amount;
    const float t3 = t2 * amount;
    return 0.5f * (2.0f * p1 + (-p0 + p2) * amount
        + (2.0f * p0 - 5.0f * p1 + 4.0f * p2 - p3) * t2
        + (-p0 + 3.0f * p1 - 3.0f * p2 + p3) * t3);
}

float SceneManager::ShortestAngleDelta(float from, float to) {
    return std::fmod(to - from + 540.0f, 360.0f) - 180.0f;
}

float SceneManager::CatmullRomAngle(float a0, float a1, float a2, float a3, float amount) {
    const float p0 = ShortestAngleDelta(a1, a0);
    const float p1 = 0.0f;
    const float p2 = ShortestAngleDelta(a1, a2);
    const float p3 = ShortestAngleDelta(a1, a3);
    return a1 + CatmullRomScalar(p0, p1, p2, p3, amount);
}

float SceneManager::SampleKeyframes(const std::array<KeyFrame, 5>& keyframes, float time,
    glm::vec3* position, glm::vec3* rotation) {
    const float loopDuration = keyframes.back().time;
    const float wrapped = std::fmod(std::max(time, 0.0f), loopDuration);
    for (std::size_t index = 0; index + 1 < keyframes.size(); ++index) {
        const KeyFrame& left = keyframes[index];
        const KeyFrame& right = keyframes[index + 1];
        if (wrapped <= right.time) {
            const float span = std::max(right.time - left.time, 0.001f);
            const float amount = glm::clamp((wrapped - left.time) / span, 0.0f, 1.0f);
            if (position) *position = glm::mix(left.position, right.position, amount);
            if (rotation) *rotation = glm::mix(left.rotation, right.rotation, amount);
            return amount;
        }
    }
    if (position) *position = keyframes.back().position;
    if (rotation) *rotation = keyframes.back().rotation;
    return 0.0f;
}

void SceneManager::ResolveMovingBoatConflicts(float deltaTime) {
    for (std::size_t boatIndex = 0; boatIndex < kMovingBoatCount; ++boatIndex) {
        movingBoatPauseFactors_[boatIndex] = std::min(
            1.0f, movingBoatPauseFactors_[boatIndex] + deltaTime * 1.8f);
    }

    // If two independent routes meet in a channel, the higher-index boat yields
    // smoothly. Its route clock stops without teleporting or changing direction.
    const float minimumDistance = 1.3f * (2.0f * kBoatHalfLength);
    for (std::size_t first = 0; first < kMovingBoatCount; ++first) {
        for (std::size_t second = first + 1; second < kMovingBoatCount; ++second) {
            const glm::vec2 firstPoint(movingBoatPositions_[first].x,
                movingBoatPositions_[first].z);
            const glm::vec2 secondPoint(movingBoatPositions_[second].x,
                movingBoatPositions_[second].z);
            if (glm::distance(firstPoint, secondPoint) < minimumDistance) {
                movingBoatPauseFactors_[second] = std::max(
                    0.0f, movingBoatPauseFactors_[second] - deltaTime * 5.0f);
            }
        }
    }
}

void SceneManager::UpdateKeyframes(float deltaTime) {
    if (keyframePlayback_) {
        keyframeTime_ = std::fmod(keyframeTime_ + deltaTime, keyframes_.back().time);
        SampleKeyframes(keyframes_, keyframeTime_, &keyframePosition_, &keyframeRotation_);
        keyframeSwing_ = keyframeRotation_.y;
    }
    if (!animationEnabled_) return;

    ResolveMovingBoatConflicts(deltaTime);
    for (std::size_t boatIndex = 0; boatIndex < kMovingBoatCount; ++boatIndex) {
        movingBoatClocks_[boatIndex] = std::fmod(
            movingBoatClocks_[boatIndex] + deltaTime * movingBoatSpeeds_[boatIndex]
                * movingBoatPauseFactors_[boatIndex],
            static_cast<float>(kBoatFrameCount));
    }

    const auto sampleRoute = [&](int routeIndex, float clock,
        glm::vec3& position, float& rotation) {
        const auto& route = routeIndex == 0 ? boatPath_
            : (routeIndex == 1 ? scenicBoatPath_
                : (routeIndex == 2 ? exteriorBoatPath_ : westernBoatPath_));
        const float wrapped = std::fmod(std::max(clock, 0.0f),
            static_cast<float>(kBoatFrameCount));
        const int index = static_cast<int>(std::floor(wrapped)) % kBoatFrameCount;
        const float amount = wrapped - static_cast<float>(index);
        const int previous = (index + kBoatFrameCount - 1) % kBoatFrameCount;
        const int next = (index + 1) % kBoatFrameCount;
        const int nextNext = (index + 2) % kBoatFrameCount;
        position = CatmullRom(route[previous].position, route[index].position,
            route[next].position, route[nextNext].position, amount);
        rotation = CatmullRomAngle(route[previous].rotationY, route[index].rotationY,
            route[next].rotationY, route[nextNext].rotationY, amount);
    };

    for (std::size_t boatIndex = 0; boatIndex < kMovingBoatCount; ++boatIndex) {
        sampleRoute(movingBoatRouteIndex_[boatIndex],
            movingBoatClocks_[boatIndex],
            movingBoatPositions_[boatIndex], movingBoatRotations_[boatIndex]);

        // Estimate local curvature from the route heading immediately before
        // and after the boat.  This produces a restrained bank in turns while
        // preserving the Catmull-Rom path and avoiding visual teleporting.
        glm::vec3 previousPosition;
        glm::vec3 nextPosition;
        float previousRotation = 0.0f;
        float nextRotation = 0.0f;
        sampleRoute(movingBoatRouteIndex_[boatIndex], movingBoatClocks_[boatIndex] - 0.10f,
            previousPosition, previousRotation);
        sampleRoute(movingBoatRouteIndex_[boatIndex], movingBoatClocks_[boatIndex] + 0.10f,
            nextPosition, nextRotation);
        const float headingDelta = ShortestAngleDelta(previousRotation, nextRotation);
        movingBoatBanks_[boatIndex] = glm::clamp(headingDelta * 0.80f, -10.0f, 10.0f);
    }
    boatPosition_ = movingBoatPositions_[0];
    boatRotation_ = movingBoatRotations_[0];
    boat2Position_ = movingBoatPositions_[1];
    boat2Rotation_ = movingBoatRotations_[1];
    boat3Position_ = movingBoatPositions_[2];
    boat3Rotation_ = movingBoatRotations_[2];
    boat4Position_ = movingBoatPositions_[3];
    boat4Rotation_ = movingBoatRotations_[3];
    sceneState_ = SceneState::BoatsMoving;
}

const char* SceneManager::SugarSkullPhaseName() const {
    switch (sugarSkullPose_.phase) {
    case SugarSkullPhase::SideBySide: return "LADO A LADO";
    case SugarSkullPhase::JumpToMoribunda: return "SALTO A MORIBUNDA";
    case SugarSkullPhase::StackedImpact: return "IMPACTO APILADO";
    case SugarSkullPhase::JumpBack: return "SALTO DE REGRESO";
    default: return "CALAVERITAS";
    }
}

void SceneManager::UpdateSugarSkullAnimation(float deltaTime) {
    constexpr float kSideBySideDuration = 1.80f;
    constexpr float kJumpDuration = 1.25f;
    constexpr float kStackedDuration = 0.80f;
    constexpr float kReturnDuration = 1.25f;
    constexpr float kCycleDuration = kSideBySideDuration + kJumpDuration
        + kStackedDuration + kReturnDuration;
    constexpr float kBaseScale = 0.16f;
    // The corrected OBJ bounds put the Moribunda body/head at roughly 3.2
    // local units tall. At kBaseScale this produces a safe, visible stack.
    constexpr float kStackOffset = 0.52f;
    constexpr float kArcHeight = 0.68f;
    const glm::vec3 moribundaBase(-2.82f, 0.10f, 0.18f);
    const glm::vec3 finadoBase(-2.15f, 0.10f, 0.18f);

    if (keyframePlayback_ && animationEnabled_) {
        sugarSkullTime_ = std::fmod(sugarSkullTime_ + deltaTime, kCycleDuration);
    }

    const float breathing = std::sin(sceneTime_ * 2.25f) * 0.018f;
    sugarSkullPose_.moribundaPosition = moribundaBase;
    sugarSkullPose_.finadoPosition = finadoBase;
    sugarSkullPose_.moribundaScale = glm::vec3(
        kBaseScale * (1.0f - breathing * 0.45f),
        kBaseScale * (1.0f + breathing),
        kBaseScale * (1.0f - breathing * 0.45f));
    sugarSkullPose_.finadoScale = sugarSkullPose_.moribundaScale;
    sugarSkullPose_.moribundaRotationZ = 0.0f;
    sugarSkullPose_.finadoRotationZ = 0.0f;

    const auto ease = [](float value) {
        const float t = glm::clamp(value, 0.0f, 1.0f);
        return t * t * (3.0f - 2.0f * t);
    };
    const auto flightScale = [](float base, float amount) {
        return glm::vec3(base * (1.0f - 0.10f * amount),
            base * (1.0f + 0.16f * amount), base * (1.0f - 0.10f * amount));
    };

    if (sugarSkullTime_ < kSideBySideDuration) {
        sugarSkullPose_.phase = SugarSkullPhase::SideBySide;
        sugarSkullPose_.phaseProgress = sugarSkullTime_ / kSideBySideDuration;
        sugarSkullPose_.moribundaRotationZ = 2.5f * std::sin(sceneTime_ * 2.0f);
        sugarSkullPose_.finadoRotationZ = -2.0f * std::sin(sceneTime_ * 2.0f + 0.6f);
        return;
    }

    const float jumpStart = kSideBySideDuration;
    const float stackStart = jumpStart + kJumpDuration;
    const float returnStart = stackStart + kStackedDuration;
    if (sugarSkullTime_ < stackStart) {
        sugarSkullPose_.phase = SugarSkullPhase::JumpToMoribunda;
        const float raw = (sugarSkullTime_ - jumpStart) / kJumpDuration;
        const float t = ease(raw);
        const float flight = std::sin(glm::pi<float>() * raw);
        sugarSkullPose_.phaseProgress = raw;
        sugarSkullPose_.finadoPosition = glm::mix(finadoBase,
            moribundaBase + glm::vec3(0.0f, kStackOffset, 0.0f), t);
        sugarSkullPose_.finadoPosition.y = glm::mix(finadoBase.y,
            moribundaBase.y + kStackOffset, t) + kArcHeight * 4.0f * raw * (1.0f - raw);
        sugarSkullPose_.finadoScale = flightScale(kBaseScale, flight);
        sugarSkullPose_.finadoRotationZ = -15.0f * std::sin(glm::pi<float>() * raw);
        sugarSkullPose_.moribundaRotationZ = 2.0f * std::sin(sceneTime_ * 2.7f);
        return;
    }

    if (sugarSkullTime_ < returnStart) {
        sugarSkullPose_.phase = SugarSkullPhase::StackedImpact;
        const float raw = (sugarSkullTime_ - stackStart) / kStackedDuration;
        const float spring = std::exp(-4.4f * raw) * std::cos(10.0f * raw);
        sugarSkullPose_.phaseProgress = raw;
        sugarSkullPose_.finadoPosition = moribundaBase
            + glm::vec3(0.0f, kStackOffset + 0.06f * std::exp(-5.0f * raw)
                * std::sin(14.0f * raw), 0.0f);
        sugarSkullPose_.finadoRotationZ = 1.6f * spring;
        sugarSkullPose_.moribundaRotationZ = 4.0f * spring;
        sugarSkullPose_.moribundaScale = glm::vec3(
            kBaseScale * (1.0f + 0.055f * std::abs(spring)),
            kBaseScale * (1.0f - 0.13f * std::max(spring, 0.0f)),
            kBaseScale * (1.0f + 0.055f * std::abs(spring)));
        sugarSkullPose_.finadoScale = glm::vec3(kBaseScale * (1.0f + 0.035f * spring),
            kBaseScale * (1.0f - 0.04f * spring), kBaseScale * (1.0f + 0.035f * spring));
        return;
    }

    sugarSkullPose_.phase = SugarSkullPhase::JumpBack;
    const float raw = (sugarSkullTime_ - returnStart) / kReturnDuration;
    const float t = ease(raw);
    const float flight = std::sin(glm::pi<float>() * raw);
    sugarSkullPose_.phaseProgress = raw;
    sugarSkullPose_.finadoPosition = glm::mix(
        moribundaBase + glm::vec3(0.0f, kStackOffset, 0.0f), finadoBase, t);
    sugarSkullPose_.finadoPosition.y = glm::mix(moribundaBase.y + kStackOffset,
        finadoBase.y, t) + kArcHeight * 4.0f * raw * (1.0f - raw);
    sugarSkullPose_.finadoScale = flightScale(kBaseScale, flight);
    if (raw > 0.80f) {
        const float landing = (raw - 0.80f) / 0.20f;
        const float squash = std::sin(glm::pi<float>() * landing);
        sugarSkullPose_.finadoScale = glm::vec3(
            kBaseScale * (1.0f + 0.08f * squash),
            kBaseScale * (1.0f - 0.15f * squash),
            kBaseScale * (1.0f + 0.08f * squash));
    }
    sugarSkullPose_.finadoRotationZ = 15.0f * std::sin(glm::pi<float>() * raw);
    sugarSkullPose_.moribundaRotationZ = 2.5f * std::sin(sceneTime_ * 2.3f + raw);
}

void SceneManager::UpdateMariachiFSM(float deltaTime) {
    if (!mariachiFsmEnabled_) return;
    for (const std::unique_ptr<MariachiActor>& mariachi : mariachis_) {
        if (mariachi) mariachi->fsm.Update(deltaTime, mariachi->instrumentType);
    }
}

void SceneManager::ResetScene() {
    sceneState_ = SceneState::Intro;
    stateTimer_ = 0.0f;
    boatFrameIndex_ = 0;
    boatFrameTime_ = 0.0f;
    boatPosition_ = boatPath_[0].position;
    boatRotation_ = boatPath_[0].rotationY;
    boat2Position_ = boatPath_[3].position;
    boat2Rotation_ = boatPath_[3].rotationY;
    boat3Position_ = boatPath_[1].position;
    boat3Rotation_ = boatPath_[1].rotationY;
    boat4Position_ = boatPath_[4].position;
    boat4Rotation_ = boatPath_[4].rotationY;
    movingBoatClocks_ = movingBoatPhases_;
    movingBoatPauseFactors_.fill(1.0f);
    movingBoatPositions_ = {
        boatPath_[0].position, scenicBoatPath_[0].position,
        exteriorBoatPath_[0].position, westernBoatPath_[0].position};
    movingBoatRotations_ = {
        boatPath_[0].rotationY, scenicBoatPath_[0].rotationY,
        exteriorBoatPath_[0].rotationY, westernBoatPath_[0].rotationY};
    animationEnabled_ = true;
    keyframePlayback_ = true;
    keyframeTime_ = 0.0f;
    keyframePosition_ = keyframes_[0].position;
    keyframeRotation_ = keyframes_[0].rotation;
    keyframeSwing_ = keyframeRotation_.y;
    mariachiFsmEnabled_ = true;
    for (const std::unique_ptr<MariachiActor>& mariachi : mariachis_) {
        if (mariachi) mariachi->fsm.Reset();
    }
    nearOfrenda_ = false;
    nearbyInterest_ = CulturalInterest::None;
    showInterestInfo_ = false;
}

void SceneManager::HandleKey(int key, int action) {
    if (key >= 0 && key < static_cast<int>(keys_.size())) {
        if (action == GLFW_PRESS) keys_[key] = true;
        if (action == GLFW_RELEASE) keys_[key] = false;
    }
    if (action != GLFW_PRESS) return;
    if (key == GLFW_KEY_SPACE) keyframePlayback_ = !keyframePlayback_;
    if (key == GLFW_KEY_B) {
        musicEnabled_ = !musicEnabled_;
        if (musicEnabled_) {
            StartBackgroundMusic();
            std::cout << "Musica de fondo activada.\n";
        } else {
            StopBackgroundMusic();
            std::cout << "Musica de fondo silenciada.\n";
        }
    }
    if (key == GLFW_KEY_L) nightLights_ = !nightLights_;
    if (key == GLFW_KEY_V) showLightMarkers_ = !showLightMarkers_;
    if (key == GLFW_KEY_M) showMinimap_ = !showMinimap_;
    if (key == GLFW_KEY_K) {
        mariachiFsmEnabled_ = !mariachiFsmEnabled_;
        std::cout << "FSM de mariachis "
            << (mariachiFsmEnabled_ ? "activada" : "en pausa") << ".\n";
    }
    if (key == GLFW_KEY_E && HasNearbyInterest()) {
        showInterestInfo_ = !showInterestInfo_;
        if (showInterestInfo_) {
            const CulturalPOI& poi = CurrentCulturalPOI();
            std::cout << "\n=== " << poi.title << " ===\n"
                << poi.content << "\n"
                << poi.keyFact << "\n"
                << "Presiona E nuevamente para cerrar la ficha.\n" << std::endl;
        }
    }
    if (key == GLFW_KEY_P) {
        animationEnabled_ = !animationEnabled_;
    }
    if (key == GLFW_KEY_H || key == GLFW_KEY_TAB) showHelp_ = !showHelp_;
    if (key == GLFW_KEY_R) ResetScene();
}

void SceneManager::HandleMouse(double xPosition, double yPosition) {
    if (firstMouse_) {
        lastMouseX_ = static_cast<float>(xPosition);
        lastMouseY_ = static_cast<float>(yPosition);
        firstMouse_ = false;
    }
    const float xOffset = static_cast<float>(xPosition) - lastMouseX_;
    const float yOffset = lastMouseY_ - static_cast<float>(yPosition);
    lastMouseX_ = static_cast<float>(xPosition);
    lastMouseY_ = static_cast<float>(yPosition);
    camera_.ProcessMouseMovement(xOffset, yOffset);
}
