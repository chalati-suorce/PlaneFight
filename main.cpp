/*
 * =====================================================================================
 *
 * Filename:  PlaneFight_Enterprise_Edition.cpp
 *
 * Description:  Space Shooter Game (True 2.5D Corridor Edition)
 * 单文件 raylib + C++ 飞机大战。包含：
 * - 透视走廊 2.5D（全透视映射：输入/运动/碰撞）
 * - 霓虹字效（描边 + 扫描 + 轻摇摆）
 * - 程序合成 8bit 循环音乐（M 键静音）
 *
 * =====================================================================================
 */

#include <raylib.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <ctime>
#include <limits>
#include <list>
#include <string>
#include <unordered_set>
#include <vector>

#include "embedded_assets.h"

using namespace std;

namespace Texts {
const char* const MENU_TITLE = u8"\u57FA\u4E8E\u94FE\u8868\u7684\u98DE\u673A\u5927\u6218\u6E38\u620F";
const char* const BTN_EASY = u8"\u7B80\u5355\u6A21\u5F0F";
const char* const BTN_NORMAL = u8"\u666E\u901A\u6A21\u5F0F";
const char* const BTN_HELL = u8"\u5730\u72F1\u6A21\u5F0F";
const char* const BTN_PAUSE = u8"\u6682\u505C";
const char* const BTN_MENU = u8"\u83DC\u5355";
const char* const BTN_RESUME = u8"\u70B9\u51FB\u7EE7\u7EED";
const char* const DEVELOPERS = u8"\u5F00\u53D1\u8005\uFF1A\u65B9\u6052\u5EB7\u3001\u77F3\u5CA9\u3001\u803F\u4E50\u3001\u738B\u667A\u8F69";
const char* const PAUSED_TITLE = u8"\u6E38\u620F\u6682\u505C";
const char* const PAUSED_HINT = u8"\u6216\u6309 [P] \u952E\u7EE7\u7EED";
const char* const END_HINT = u8"\u6309 [\u7A7A\u683C] \u91CD\u8BD5\uFF0C[ESC] \u8FD4\u56DE\u83DC\u5355";
const char* const MUSIC_ON = u8"\u266A ON";
const char* const MUSIC_OFF = u8"\u266A OFF";
}

class GameConfig {
public:
    static const double SCALE;
    static const int BASE_WIDTH;
    static const int BASE_HEIGHT;

    static const Color COLOR_BG;
    static const Color COLOR_GRID;
    static const Color COLOR_PLAYER;
    static const Color COLOR_ENEMY;
    static const Color COLOR_BULLET;
    static const Color COLOR_TEXT;

    static inline int S(double v) {
        return static_cast<int>(v * SCALE);
    }

    static inline int GetWindowWidth() {
        return S(BASE_WIDTH);
    }

    static inline int GetWindowHeight() {
        return S(BASE_HEIGHT);
    }
};

const double GameConfig::SCALE = 3.4;
const int GameConfig::BASE_WIDTH = 256;
const int GameConfig::BASE_HEIGHT = 256;

const Color GameConfig::COLOR_BG = {30, 30, 35, 255};
const Color GameConfig::COLOR_GRID = {50, 50, 60, 255};
const Color GameConfig::COLOR_PLAYER = {50, 150, 250, 255};
const Color GameConfig::COLOR_ENEMY = {230, 80, 80, 255};
const Color GameConfig::COLOR_BULLET = {255, 200, 50, 255};
const Color GameConfig::COLOR_TEXT = {240, 240, 240, 255};

struct PlayerMotionState {
    Vector2 velocity;
    Vector2 accel;
    float tiltDeg;
    float recoil;

    PlayerMotionState()
        : velocity{0.0f, 0.0f},
          accel{0.0f, 0.0f},
          tiltDeg(0.0f),
          recoil(0.0f) {}
};

struct CameraFXState {
    float trauma;
    float shakeX;
    float shakeY;
    float rollDeg;
    float zoom;

    CameraFXState()
        : trauma(0.0f),
          shakeX(0.0f),
          shakeY(0.0f),
          rollDeg(0.0f),
          zoom(1.0f) {}
};

struct PerspectiveConfig {
    float horizonY;
    float bottomY;
    float laneHalfFar;
    float laneHalfNear;
    float minDepthZ;
    float maxDepthZ;

    PerspectiveConfig()
        : horizonY(0.0f),
          bottomY(0.0f),
          laneHalfFar(0.0f),
          laneHalfNear(0.0f),
          minDepthZ(0.0f),
          maxDepthZ(1.0f) {}
};

struct PerspectivePose {
    float laneX;
    float depthZ;
    Vector2 screenPos;
    float screenScale;
    float screenRadius;

    PerspectivePose()
        : laneX(0.0f),
          depthZ(0.0f),
          screenPos{0.0f, 0.0f},
          screenScale(1.0f),
          screenRadius(1.0f) {}
};

struct Particle {
    bool active;
    Vector2 position;
    Vector2 velocity;
    float life;
    float maxLife;
    float size;
    float rotation;
    float spin;
    Color startColor;
    Color endColor;
    int priority;

    Particle()
        : active(false),
          position{0.0f, 0.0f},
          velocity{0.0f, 0.0f},
          life(0.0f),
          maxLife(0.0f),
          size(1.0f),
          rotation(0.0f),
          spin(0.0f),
          startColor{255, 255, 255, 255},
          endColor{255, 255, 255, 0},
          priority(0) {}
};

struct ParallaxLayer {
    float speed;
    float offsetY;
    float alpha;
    int density;
    float drift;
    Color color;

    ParallaxLayer()
        : speed(0.0f),
          offsetY(0.0f),
          alpha(0.0f),
          density(0),
          drift(0.0f),
          color{255, 255, 255, 255} {}
};

struct ShipVolumeStyle {
    int thicknessLayers;
    float maxThicknessPx;
    float shadowBoost;
    unsigned char highlightAlpha;
    unsigned char rimAlpha;

    ShipVolumeStyle()
        : thicknessLayers(3),
          maxThicknessPx(5.5f),
          shadowBoost(1.0f),
          highlightAlpha(64),
          rimAlpha(52) {}
};

struct UiDriftState {
    float x;
    float v;
    float target;
    float retargetTimer;
    float amp;

    UiDriftState()
        : x(0.0f),
          v(0.0f),
          target(0.0f),
          retargetTimer(0.0f),
          amp(1.0f) {}
};

static const float kPi = 3.14159265358979323846f;
static const float kTau = 6.28318530717958647692f;

static float ClampFloat(float value, float minVal, float maxVal) {
    if (value < minVal) return minVal;
    if (value > maxVal) return maxVal;
    return value;
}

static float LerpFloat(float a, float b, float t) {
    t = ClampFloat(t, 0.0f, 1.0f);
    return a + (b - a) * t;
}

static float SmoothStep(float t) {
    t = ClampFloat(t, 0.0f, 1.0f);
    return t * t * (3.0f - 2.0f * t);
}

static float EaseOutCubic(float t) {
    t = ClampFloat(t, 0.0f, 1.0f);
    float inv = 1.0f - t;
    return 1.0f - inv * inv * inv;
}

static float WrapFloat(float value, float range) {
    if (range <= 0.0f) return 0.0f;
    while (value < 0.0f) value += range;
    while (value >= range) value -= range;
    return value;
}

static float Random01() {
    return (float)rand() / (float)RAND_MAX;
}

static float RandomRange(float minVal, float maxVal) {
    return minVal + (maxVal - minVal) * Random01();
}

static Color LerpColor(const Color& a, const Color& b, float t) {
    t = ClampFloat(t, 0.0f, 1.0f);
    Color out;
    out.r = (unsigned char)(a.r + (b.r - a.r) * t);
    out.g = (unsigned char)(a.g + (b.g - a.g) * t);
    out.b = (unsigned char)(a.b + (b.b - a.b) * t);
    out.a = (unsigned char)(a.a + (b.a - a.a) * t);
    return out;
}

static float DistSq(Vector2 a, Vector2 b) {
    float dx = a.x - b.x;
    float dy = a.y - b.y;
    return dx * dx + dy * dy;
}

class PerspectiveMapper {
private:
    PerspectiveConfig cfg;

public:
    PerspectiveMapper();
    explicit PerspectiveMapper(const PerspectiveConfig& config);

    void setConfig(const PerspectiveConfig& config);
    const PerspectiveConfig& getConfig() const;

    Vector2 projectToScreen(float laneX, float depthZ) const;
    float depthToScale(float depthZ) const;
    float laneHalfWidth(float depthZ) const;
    float depthToScreenY(float depthZ) const;
};

class Vector2D {
private:
    double x;
    double y;

public:
    Vector2D();
    Vector2D(double _x, double _y);
    ~Vector2D();

    double getX() const;
    double getY() const;
    void setX(double _x);
    void setY(double _y);
    void set(double _x, double _y);
    void addX(double dx);
    void addY(double dy);
};

class MathUtils {
public:
    static bool isColliding(int x1, int y1, int w1, int h1, int x2, int y2, int w2, int h2);
    static bool isPointInRect(float px, float py, int rx, int ry, int rw, int rh);
    static int clamp(int value, int minVal, int maxVal);
};

class GraphicsEngine {
private:
    static const Font* uiFont;
    static bool hasUIFont;
    static float fxTime;

public:
    static void setUIFont(const Font* font, bool available);
    static void setFXTime(float t);
    static void drawImageAlpha(int x, int y, const Texture2D* tex);
    static void outTextCenter(int x, int y, const char* str, int fontSize = 20, const char* fontName = "微软雅黑");
    static void drawFxTextCenter(int x, int y, const char* str, int fontSize, float intensity = 1.0f, float driftX = 0.0f, const char* fontName = "微软雅黑");
    static void drawVolumetricSprite(Texture2D tex, Rectangle src, Rectangle dst, Vector2 origin, float rotationDeg, ShipVolumeStyle style, Color tint);
};

class ResourceManager {
private:
    Texture2D imgPlayer;
    Texture2D imgEnemy;
    Texture2D imgBulletPlayer;
    Texture2D imgBulletEnemy;
    Texture2D imgBackground;

    Font uiFont;

    bool hasImgPlayer;
    bool hasImgEnemy;
    bool hasImgBulletP;
    bool hasImgBulletE;
    bool hasImgBackground;
    bool hasUIFont;

    bool loadTextureScaled(Texture2D& target, const char* path, int w, int h, bool& stateFlag);
    bool loadUIFontFromSystem();

public:
    ResourceManager();
    ~ResourceManager();

    void loadAllResources();

    const Texture2D* getPlayerImage();
    const Texture2D* getEnemyImage();
    const Texture2D* getBulletPlayerImage();
    const Texture2D* getBulletEnemyImage();
    const Texture2D* getBackgroundImage();

    bool isPlayerImageValid() const;
    bool isEnemyImageValid() const;
    bool isBulletPlayerImageValid() const;
    bool isBulletEnemyImageValid() const;
    bool isBackgroundImageValid() const;
    bool isUIFontValid() const;
};

class GameObject {
protected:
    Vector2D position;
    int width;
    int height;
    bool isAlive;

    float laneX;
    float depthZ;
    float baseRadius;
    PerspectivePose pose;

public:
    GameObject(double _x, double _y, int _w, int _h);
    virtual ~GameObject();

    virtual void draw() = 0;
    virtual bool move(float dt) = 0;
    virtual void updatePerspective(const PerspectiveConfig& cfg);

    double getX() const;
    double getY() const;
    int getWidth() const;
    int getHeight() const;
    bool getIsAlive() const;
    void setIsAlive(bool state);
    void setPosition(double _x, double _y);

    float getLaneX() const;
    float getDepthZ() const;
    void setLaneX(float v);
    void setDepthZ(float v);
    void setBaseRadius(float v);

    const PerspectivePose& getPose() const;
    float getScreenScale() const;
};

class Bullet : public GameObject {
private:
    ResourceManager* resMgr;
    float speed;

public:
    Bullet(float _laneX, float _depthZ, float _speed, ResourceManager* _resMgr);
    virtual ~Bullet();

    virtual void draw() override;
    virtual bool move(float dt) override;
};

class EnemyBullet : public GameObject {
private:
    ResourceManager* resMgr;
    float speed;

public:
    EnemyBullet(float _laneX, float _depthZ, float _speed, ResourceManager* _resMgr);
    virtual ~EnemyBullet();

    virtual void draw() override;
    virtual bool move(float dt) override;
};

class Enemy : public GameObject {
private:
    ResourceManager* resMgr;
    float advanceSpeed;

public:
    Enemy(float _laneX, float _depthZ, float _speed, ResourceManager* _resMgr);
    virtual ~Enemy();

    virtual void draw() override;
    virtual bool move(float dt) override;
};

class Player : public GameObject {
private:
    ResourceManager* resMgr;
    PlayerMotionState motionState;

public:
    Player(ResourceManager* _resMgr);
    virtual ~Player();

    virtual void draw() override;
    virtual bool move(float dt) override;

    void triggerRecoil(float amount);
    const PlayerMotionState& getMotionState() const;
};

class Button {
private:
    int x;
    int y;
    int w;
    int h;
    string text;
    bool isHover;
    bool isPressedVisual;
    int fontSize;
    Color baseColor;
    string fontName;

public:
    Button();
    Button(int _x, int _y, int _w, int _h, string _text, int _fs, Color _c, string _fn);
    ~Button();

    void draw();
    bool update(float mouseX, float mouseY, bool mousePressed, bool mouseDown);

    void setPosition(int _x, int _y);
    void setSize(int _w, int _h);
    void setText(const string& str);
    bool getHoverState() const;
};

enum GameState {
    MENU,
    PLAYING,
    PAUSED,
    END
};

class ParticleSystem {
private:
    static const int MAX_PARTICLES = 550;
    static const int MAX_SPAWN_PER_FRAME = 80;
    array<Particle, MAX_PARTICLES> particles;
    int spawnedThisFrame;

public:
    ParticleSystem();
    void clear();
    void beginFrame();
    bool emit(const Particle& p);
    void update(float dt);
    void draw() const;
    int aliveCount() const;
};

class ChipMusicEngine {
private:
    bool initialized;
    bool muted;
    AudioStream stream;

    int sampleRate;
    int chunkFrames;
    vector<float> buffer;

    float globalTime;
    float phase1;
    float phase2;
    float phase3;
    float lowpassState;

    int stepIndex;
    float stepTime;
    float stepDuration;
    int section;
    int barIndex;
    int stepInBar;
    int arrangementIndex;
    int fillCountdown;

    uint32_t noiseState;

    float midiToFreq(int midiNote) const;
    float squareWave(float phase, float duty) const;
    float triWave(float phase) const;
    float nextNoise();
    void generateChunk();

public:
    ChipMusicEngine();
    ~ChipMusicEngine();

    bool init();
    void update(float dt);
    void toggleMute();
    void shutdown();
    bool isMuted() const;
};

class GameManager {
private:
    ResourceManager resourceManager;
    GameState currentState;

    Player* player;
    list<Bullet*> bullets;
    list<Enemy*> enemies;
    list<EnemyBullet*> enemyBullets;

    int score;
    bool gameOver;
    float deltaTime;
    float uiTime;
    float shootCooldown;
    float pauseCooldown;

    int enemySpawnRate;
    int enemyShootChance;
    float enemySpawnTimer;
    float enemyAdvanceSpeed;
    float enemyBulletSpeed;
    float hitStopTimer;
    float screenFlashAlpha;

    float endScoreAnimTimer;
    int animatedEndScore;
    UiDriftState titleDrift;
    UiDriftState hudDrift;
    UiDriftState microDrift;

    CameraFXState cameraFX;
    ParticleSystem particleSystem;
    ChipMusicEngine chipMusic;

    PerspectiveConfig perspectiveCfg;
    PerspectiveMapper perspectiveMapper;

    array<ParallaxLayer, 2> parallaxLayers;
    vector<Vector2> farStars;
    vector<Vector2> midClouds;

    Vector2 pendingHitPos;
    bool pendingHitFX;

    Button btnEasy;
    Button btnNormal;
    Button btnHell;
    Button btnPause;
    Button btnResume;
    Button btnMenu;

    void initUI();
    void initBackgroundLayers();
    void clearEntities();
    void setDifficulty(int spawnRate, int shootChance);
    void enterEndState();

    void updateMenu();
    void updatePlaying();
    void updatePaused();
    void updateGameOver();

    void processInput(float worldDt);
    void updateGameLogic(float worldDt);
    void renderGameScene();
    void drawBackground();
    void drawShadowEllipse(float x, float y, float rw, float rh, unsigned char alpha) const;

public:
    GameManager();
    ~GameManager();

    void resetGame();
    void run();

    void updateCameraFX(float dt);
    void updateParallax(float dt);
    void updateUiDrift(float dt);
    void spawnMuzzleFX();
    void spawnHitFX();

    void applyRoadBoundaryClamp(GameObject* obj, float extraMarginPx);
    void updatePerspectiveWorld(float dt);
    void resolvePerspectiveCollisions();
    void layoutHUD();

    void drawWorldWithCamera();
    void drawScreenFX();
    void drawUnifiedUI();

    void drawCorridorBackground();
    void drawCorridorGuides();
    void drawMusicIndicator();
};

const Font* GraphicsEngine::uiFont = nullptr;
bool GraphicsEngine::hasUIFont = false;
float GraphicsEngine::fxTime = 0.0f;

PerspectiveMapper::PerspectiveMapper() : cfg() {}
PerspectiveMapper::PerspectiveMapper(const PerspectiveConfig& config) : cfg(config) {}

void PerspectiveMapper::setConfig(const PerspectiveConfig& config) {
    this->cfg = config;
}

const PerspectiveConfig& PerspectiveMapper::getConfig() const {
    return this->cfg;
}

float PerspectiveMapper::laneHalfWidth(float depthZ) const {
    float t = 0.0f;
    if (this->cfg.maxDepthZ > this->cfg.minDepthZ) {
        t = (depthZ - this->cfg.minDepthZ) / (this->cfg.maxDepthZ - this->cfg.minDepthZ);
    }
    float curve = SmoothStep(t);
    return LerpFloat(this->cfg.laneHalfFar, this->cfg.laneHalfNear, curve);
}

float PerspectiveMapper::depthToScreenY(float depthZ) const {
    float t = 0.0f;
    if (this->cfg.maxDepthZ > this->cfg.minDepthZ) {
        t = (depthZ - this->cfg.minDepthZ) / (this->cfg.maxDepthZ - this->cfg.minDepthZ);
    }
    float curve = SmoothStep(t);
    return LerpFloat(this->cfg.horizonY, this->cfg.bottomY, curve);
}

float PerspectiveMapper::depthToScale(float depthZ) const {
    float t = 0.0f;
    if (this->cfg.maxDepthZ > this->cfg.minDepthZ) {
        t = (depthZ - this->cfg.minDepthZ) / (this->cfg.maxDepthZ - this->cfg.minDepthZ);
    }
    float curve = SmoothStep(t);
    return LerpFloat(0.28f, 1.22f, curve);
}

Vector2 PerspectiveMapper::projectToScreen(float laneX, float depthZ) const {
    float centerX = GameConfig::GetWindowWidth() * 0.5f;
    float halfWidth = this->laneHalfWidth(depthZ);
    float y = this->depthToScreenY(depthZ);
    return {centerX + laneX * halfWidth, y};
}

Vector2D::Vector2D() : x(0.0), y(0.0) {}
Vector2D::Vector2D(double _x, double _y) : x(_x), y(_y) {}
Vector2D::~Vector2D() {}

double Vector2D::getX() const { return this->x; }
double Vector2D::getY() const { return this->y; }
void Vector2D::setX(double _x) { this->x = _x; }
void Vector2D::setY(double _y) { this->y = _y; }
void Vector2D::set(double _x, double _y) {
    this->x = _x;
    this->y = _y;
}
void Vector2D::addX(double dx) { this->x += dx; }
void Vector2D::addY(double dy) { this->y += dy; }

bool MathUtils::isColliding(int x1, int y1, int w1, int h1, int x2, int y2, int w2, int h2) {
    return (x1 < x2 + w2 && x1 + w1 > x2 && y1 < y2 + h2 && y1 + h1 > y2);
}

bool MathUtils::isPointInRect(float px, float py, int rx, int ry, int rw, int rh) {
    return (px >= rx && px <= rx + rw && py >= ry && py <= ry + rh);
}

int MathUtils::clamp(int value, int minVal, int maxVal) {
    if (value < minVal) return minVal;
    if (value > maxVal) return maxVal;
    return value;
}
void GraphicsEngine::setUIFont(const Font* font, bool available) {
    uiFont = font;
    hasUIFont = available;
}

void GraphicsEngine::setFXTime(float t) {
    fxTime = t;
}

void GraphicsEngine::drawImageAlpha(int x, int y, const Texture2D* tex) {
    if (tex == nullptr || tex->id == 0) {
        return;
    }
    DrawTextureV(*tex, {(float)x, (float)y}, WHITE);
}

void GraphicsEngine::outTextCenter(int x, int y, const char* str, int fontSize, const char* fontName) {
    (void)fontName;
    if (str == nullptr || str[0] == '\0') {
        return;
    }

    if (hasUIFont && uiFont != nullptr && uiFont->texture.id != 0) {
        Vector2 size = MeasureTextEx(*uiFont, str, (float)fontSize, 1.0f);
        Vector2 pos = {(float)x - size.x * 0.5f, (float)y - size.y * 0.5f};
        DrawTextEx(*uiFont, str, pos, (float)fontSize, 1.0f, GameConfig::COLOR_TEXT);
        return;
    }

    int w = MeasureText(str, fontSize);
    DrawText(str, x - w / 2, y - fontSize / 2, fontSize, GameConfig::COLOR_TEXT);
}

void GraphicsEngine::drawFxTextCenter(int x, int y, const char* str, int fontSize, float intensity, float driftX, const char* fontName) {
    (void)fontName;
    if (str == nullptr || str[0] == '\0') {
        return;
    }

    intensity = ClampFloat(intensity, 0.0f, 1.0f);
    float cx = x + driftX;
    float rotSwing = driftX * 1.85f;
    if (intensity >= 0.62f) {
        rotSwing += std::sin(fxTime * 2.15f + x * 0.008f) * (0.75f + intensity * 0.9f);
    }
    float rotDeg = ClampFloat(rotSwing, -6.0f, 6.0f);

    auto drawAt = [&](float px, float py, Color c) {
        if (hasUIFont && uiFont != nullptr && uiFont->texture.id != 0) {
            Vector2 size = MeasureTextEx(*uiFont, str, (float)fontSize, 1.0f);
            Vector2 pos = {px, py};
            Vector2 origin = {size.x * 0.5f, size.y * 0.5f};
            DrawTextPro(*uiFont, str, pos, origin, rotDeg, (float)fontSize, 1.0f, c);
        } else {
            int w = MeasureText(str, fontSize);
            DrawText(str, (int)(px - w * 0.5f), (int)(py - fontSize * 0.5f), fontSize, c);
        }
    };

    const Color glow = {90, 220, 255, (unsigned char)(150.0f * intensity)};
    drawAt(cx - 2.0f, y, glow);
    drawAt(cx + 2.0f, y, glow);
    drawAt(cx, y - 2.0f, glow);
    drawAt(cx, y + 2.0f, glow);
    drawAt(cx - 1.5f, y - 1.5f, {70, 200, 255, (unsigned char)(95.0f * intensity)});
    drawAt(cx + 1.5f, y + 1.5f, {70, 200, 255, (unsigned char)(95.0f * intensity)});

    drawAt(cx, y, {230, 250, 255, 255});

    if (intensity > 0.45f) {
        float textW = 0.0f;
        float textH = (float)fontSize;
        if (hasUIFont && uiFont != nullptr && uiFont->texture.id != 0) {
            Vector2 size = MeasureTextEx(*uiFont, str, (float)fontSize, 1.0f);
            textW = size.x;
            textH = size.y;
        } else {
            textW = (float)MeasureText(str, fontSize);
        }

        float scanW = std::max(10.0f, fontSize * 0.42f);
        float scanX = cx - textW * 0.5f + WrapFloat(fxTime * 198.0f, textW + scanW * 2.0f) - scanW;

        BeginBlendMode(BLEND_ADDITIVE);
        DrawRectangle((int)scanX,
                      (int)(y - textH * 0.55f),
                      (int)scanW,
                      (int)(textH * 1.15f),
                      {130, 240, 255, (unsigned char)(55.0f * intensity)});
        EndBlendMode();
    }
}

void GraphicsEngine::drawVolumetricSprite(Texture2D tex, Rectangle src, Rectangle dst, Vector2 origin, float rotationDeg, ShipVolumeStyle style, Color tint) {
    if (tex.id == 0 || src.width == 0.0f || src.height == 0.0f) {
        return;
    }

    float baseScale = std::fabs(dst.width / src.width);
    float t = ClampFloat((baseScale - 0.28f) / (1.22f - 0.28f), 0.0f, 1.0f);
    float thicknessPx = LerpFloat(1.0f, style.maxThicknessPx, t);
    int layers = std::max(1, style.thicknessLayers);
    float shadowBoost = ClampFloat(style.shadowBoost, 0.6f, 1.8f);

    for (int i = layers; i >= 1; --i) {
        float lf = (float)i / (float)layers;
        float off = thicknessPx * lf;
        Rectangle depthDst = dst;
        depthDst.x += off;
        depthDst.y += off;

        Color depthTint = {
            (unsigned char)ClampFloat(tint.r * (0.40f / shadowBoost), 0.0f, 255.0f),
            (unsigned char)ClampFloat(tint.g * (0.40f / shadowBoost), 0.0f, 255.0f),
            (unsigned char)ClampFloat(tint.b * (0.44f / shadowBoost), 0.0f, 255.0f),
            (unsigned char)ClampFloat((52.0f + 44.0f * lf) * shadowBoost, 0.0f, 255.0f)
        };
        DrawTexturePro(tex, src, depthDst, origin, rotationDeg, depthTint);
    }

    DrawTexturePro(tex, src, dst, origin, rotationDeg, tint);

    float rad = rotationDeg * (kPi / 180.0f);
    float cr = std::cos(rad);
    float sr = std::sin(rad);
    auto rotPoint = [&](float lx, float ly) {
        return Vector2{dst.x + lx * cr - ly * sr, dst.y + lx * sr + ly * cr};
    };

    float halfW = dst.width * 0.5f;
    float halfH = dst.height * 0.5f;

    BeginBlendMode(BLEND_ADDITIVE);

    Vector2 h0 = rotPoint(-halfW * 0.34f, -halfH * 0.34f);
    Vector2 h1 = rotPoint(halfW * 0.34f, -halfH * 0.34f);
    DrawLineEx(h0, h1, std::max(1.0f, dst.width * 0.08f), {255, 255, 255, style.highlightAlpha});

    Vector2 r0 = rotPoint(-halfW * 0.30f, -halfH * 0.24f);
    Vector2 r1 = rotPoint(-halfW * 0.38f, halfH * 0.30f);
    Vector2 r2 = rotPoint(halfW * 0.30f, -halfH * 0.24f);
    Vector2 r3 = rotPoint(halfW * 0.38f, halfH * 0.30f);
    DrawLineEx(r0, r1, std::max(1.0f, dst.width * 0.05f), {120, 220, 255, style.rimAlpha});
    DrawLineEx(r2, r3, std::max(1.0f, dst.width * 0.05f), {120, 220, 255, style.rimAlpha});

    Vector2 nose = rotPoint(0.0f, -halfH * 0.45f);
    DrawCircleV(nose, std::max(1.0f, dst.width * 0.06f), {255, 255, 255, (unsigned char)(style.highlightAlpha * 0.78f)});

    EndBlendMode();
}

ResourceManager::ResourceManager()
    : imgPlayer(),
      imgEnemy(),
      imgBulletPlayer(),
      imgBulletEnemy(),
      imgBackground(),
      uiFont(),
      hasImgPlayer(false),
      hasImgEnemy(false),
      hasImgBulletP(false),
      hasImgBulletE(false),
      hasImgBackground(false),
      hasUIFont(false) {
}

ResourceManager::~ResourceManager() {
    if (this->imgPlayer.id != 0) UnloadTexture(this->imgPlayer);
    if (this->imgEnemy.id != 0) UnloadTexture(this->imgEnemy);
    if (this->imgBulletPlayer.id != 0) UnloadTexture(this->imgBulletPlayer);
    if (this->imgBulletEnemy.id != 0) UnloadTexture(this->imgBulletEnemy);
    if (this->imgBackground.id != 0) UnloadTexture(this->imgBackground);

    if (this->hasUIFont && this->uiFont.texture.id != 0) {
        UnloadFont(this->uiFont);
    }

    GraphicsEngine::setUIFont(nullptr, false);
}

bool ResourceManager::loadTextureScaled(Texture2D& target, const char* path, int w, int h, bool& stateFlag) {
    stateFlag = false;
    Image img = {0};

    if (FileExists(path)) {
        img = LoadImage(path);
    }

#if defined(_WIN32)
    if (img.data == nullptr) {
        const unsigned char* rawData = nullptr;
        int rawSize = 0;
        if (GetEmbeddedAssetData(path, rawData, rawSize)) {
            img = LoadImageFromMemory(".png", rawData, rawSize);
        }
    }
#endif

    if (img.data == nullptr) {
        return false;
    }

    ImageResizeNN(&img, w, h);
    target = LoadTextureFromImage(img);
    UnloadImage(img);

    stateFlag = (target.id != 0);
    return stateFlag;
}

bool ResourceManager::loadUIFontFromSystem() {
    static const char* fontPaths[] = {
        "C:/Windows/Fonts/simhei.ttf",
        "C:/Windows/Fonts/msyh.ttc",
        "C:/Windows/Fonts/msyhbd.ttc",
        "C:/Windows/Fonts/simsun.ttc"
    };

    const char* glyphSource =
        u8"\u57FA\u4E8E\u94FE\u8868\u7684\u98DE\u673A\u5927\u6218\u6E38\u620F"
        u8"\u7B80\u5355\u6A21\u5F0F\u666E\u901A\u6A21\u5F0F\u5730\u72F1\u6A21\u5F0F"
        u8"\u6682\u505C\u83DC\u5355\u70B9\u51FB\u7EE7\u7EED"
        u8"\u5F00\u53D1\u8005\uFF1A\u65B9\u6052\u5EB7\u3001\u77F3\u5CA9\u3001\u803F\u4E50\u3001\u738B\u667A\u8F69"
        u8"\u6216\u6309 [P] \u952E\u7EE7\u7EED"
        u8"\u6309 [\u7A7A\u683C] \u91CD\u8BD5\uFF0C[ESC] \u8FD4\u56DE\u83DC\u5355"
        u8"SCORE: Final Score GAME OVER MUSIC ON OFF MUTE"
        u8"\u266A"
        u8"0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz[]:,.-_() ";

    int rawCount = 0;
    int* rawCodepoints = LoadCodepoints(glyphSource, &rawCount);
    if (rawCodepoints == nullptr || rawCount <= 0) {
        return false;
    }

    vector<int> uniqueCodepoints;
    uniqueCodepoints.reserve((size_t)rawCount);
    unordered_set<int> seen;

    for (int i = 0; i < rawCount; ++i) {
        int cp = rawCodepoints[i];
        if (seen.insert(cp).second) {
            uniqueCodepoints.push_back(cp);
        }
    }
    UnloadCodepoints(rawCodepoints);

    if (uniqueCodepoints.empty()) {
        return false;
    }

    const int fontBaseSize = GameConfig::S(13);
    for (const char* path : fontPaths) {
        if (!FileExists(path)) {
            continue;
        }

        Font candidate = LoadFontEx(path, fontBaseSize, uniqueCodepoints.data(), (int)uniqueCodepoints.size());
        if (candidate.texture.id != 0) {
            this->uiFont = candidate;
            TraceLog(LOG_INFO, "Loaded UI font from: %s", path);
            return true;
        }
    }

    TraceLog(LOG_WARNING, "No system CJK font loaded. Falling back to default font.");
    return false;
}

void ResourceManager::loadAllResources() {
    this->loadTextureScaled(this->imgBackground, "purple.png", GameConfig::GetWindowWidth(), GameConfig::GetWindowHeight(), this->hasImgBackground);
    this->loadTextureScaled(this->imgPlayer, "playerShip_blue.png", GameConfig::S(32), GameConfig::S(32), this->hasImgPlayer);
    this->loadTextureScaled(this->imgEnemy, "playerShip_red.png", GameConfig::S(40), GameConfig::S(40), this->hasImgEnemy);
    this->loadTextureScaled(this->imgBulletPlayer, "laserBlue.png", GameConfig::S(8), GameConfig::S(24), this->hasImgBulletP);
    this->loadTextureScaled(this->imgBulletEnemy, "laserRed.png", GameConfig::S(8), GameConfig::S(24), this->hasImgBulletE);

    this->hasUIFont = this->loadUIFontFromSystem();
    GraphicsEngine::setUIFont(this->hasUIFont ? &this->uiFont : nullptr, this->hasUIFont);
}

const Texture2D* ResourceManager::getPlayerImage() { return &this->imgPlayer; }
const Texture2D* ResourceManager::getEnemyImage() { return &this->imgEnemy; }
const Texture2D* ResourceManager::getBulletPlayerImage() { return &this->imgBulletPlayer; }
const Texture2D* ResourceManager::getBulletEnemyImage() { return &this->imgBulletEnemy; }
const Texture2D* ResourceManager::getBackgroundImage() { return &this->imgBackground; }

bool ResourceManager::isPlayerImageValid() const { return this->hasImgPlayer; }
bool ResourceManager::isEnemyImageValid() const { return this->hasImgEnemy; }
bool ResourceManager::isBulletPlayerImageValid() const { return this->hasImgBulletP; }
bool ResourceManager::isBulletEnemyImageValid() const { return this->hasImgBulletE; }
bool ResourceManager::isBackgroundImageValid() const { return this->hasImgBackground; }
bool ResourceManager::isUIFontValid() const { return this->hasUIFont; }

GameObject::GameObject(double _x, double _y, int _w, int _h)
    : position(_x, _y),
      width(_w),
      height(_h),
      isAlive(true),
      laneX(0.0f),
      depthZ(0.0f),
      baseRadius((float)std::min(_w, _h) * 0.35f),
      pose() {
}

GameObject::~GameObject() {}

void GameObject::updatePerspective(const PerspectiveConfig& cfg) {
    float t = 0.0f;
    if (cfg.maxDepthZ > cfg.minDepthZ) {
        t = (this->depthZ - cfg.minDepthZ) / (cfg.maxDepthZ - cfg.minDepthZ);
    }

    float curve = SmoothStep(t);
    float halfWidth = LerpFloat(cfg.laneHalfFar, cfg.laneHalfNear, curve);
    float centerX = GameConfig::GetWindowWidth() * 0.5f;

    this->pose.laneX = this->laneX;
    this->pose.depthZ = this->depthZ;
    this->pose.screenPos = {centerX + this->laneX * halfWidth, LerpFloat(cfg.horizonY, cfg.bottomY, curve)};
    this->pose.screenScale = LerpFloat(0.28f, 1.22f, curve);
    this->pose.screenRadius = std::max(1.0f, this->baseRadius * this->pose.screenScale);

    this->position.set(this->pose.screenPos.x - this->width * this->pose.screenScale * 0.5f,
                       this->pose.screenPos.y - this->height * this->pose.screenScale * 0.5f);
}

double GameObject::getX() const { return this->position.getX(); }
double GameObject::getY() const { return this->position.getY(); }
int GameObject::getWidth() const { return this->width; }
int GameObject::getHeight() const { return this->height; }
bool GameObject::getIsAlive() const { return this->isAlive; }
void GameObject::setIsAlive(bool state) { this->isAlive = state; }
void GameObject::setPosition(double _x, double _y) { this->position.set(_x, _y); }

float GameObject::getLaneX() const { return this->laneX; }
float GameObject::getDepthZ() const { return this->depthZ; }
void GameObject::setLaneX(float v) { this->laneX = v; }
void GameObject::setDepthZ(float v) { this->depthZ = v; }
void GameObject::setBaseRadius(float v) { this->baseRadius = v; }

const PerspectivePose& GameObject::getPose() const { return this->pose; }
float GameObject::getScreenScale() const { return this->pose.screenScale; }

Bullet::Bullet(float _laneX, float _depthZ, float _speed, ResourceManager* _resMgr)
    : GameObject(0.0, 0.0, GameConfig::S(8), GameConfig::S(24)), resMgr(_resMgr), speed(_speed) {
    this->laneX = _laneX;
    this->depthZ = _depthZ;
    this->baseRadius = (float)GameConfig::S(3);
}

Bullet::~Bullet() {}

bool Bullet::move(float dt) {
    this->depthZ -= this->speed * dt;
    return this->depthZ > -0.03f;
}

void Bullet::draw() {
    float scale = this->pose.screenScale;
    float bodyW = std::max(1.0f, this->width * scale * 0.55f);
    float bodyH = std::max(2.0f, this->height * scale * 0.85f);
    float x = this->pose.screenPos.x;
    float y = this->pose.screenPos.y;

    float tailLen = std::max(3.0f, bodyH * 0.90f);
    for (int i = 0; i < 4; ++i) {
        float t0 = (float)i / 4.0f;
        float t1 = (float)(i + 1) / 4.0f;
        unsigned char a = (unsigned char)(120.0f * (1.0f - t0));
        float thick = std::max(1.0f, bodyW * (0.75f - t0 * 0.35f));
        DrawLineEx({x, y + t0 * tailLen}, {x, y + t1 * tailLen}, thick, {90, 180, 255, a});
    }

    DrawCircleV({x, y}, std::max(1.0f, bodyW * 0.90f), {80, 180, 255, 85});

    if (this->resMgr->isBulletPlayerImageValid()) {
        Texture2D tex = *this->resMgr->getBulletPlayerImage();
        Rectangle src = {0.0f, 0.0f, (float)tex.width, (float)tex.height};
        Rectangle dst = {x, y, bodyW, bodyH};
        Vector2 org = {bodyW * 0.5f, bodyH * 0.5f};
        DrawTexturePro(tex, src, dst, org, 0.0f, WHITE);
    } else {
        DrawRectangle((int)(x - bodyW * 0.5f), (int)(y - bodyH * 0.5f), (int)bodyW, (int)bodyH, GameConfig::COLOR_BULLET);
    }
}

EnemyBullet::EnemyBullet(float _laneX, float _depthZ, float _speed, ResourceManager* _resMgr)
    : GameObject(0.0, 0.0, GameConfig::S(8), GameConfig::S(24)), resMgr(_resMgr), speed(_speed) {
    this->laneX = _laneX;
    this->depthZ = _depthZ;
    this->baseRadius = (float)GameConfig::S(3);
}

EnemyBullet::~EnemyBullet() {}

bool EnemyBullet::move(float dt) {
    this->depthZ += this->speed * dt;
    return this->depthZ < 1.02f;
}

void EnemyBullet::draw() {
    float scale = this->pose.screenScale;
    float bodyW = std::max(1.0f, this->width * scale * 0.55f);
    float bodyH = std::max(2.0f, this->height * scale * 0.85f);
    float x = this->pose.screenPos.x;
    float y = this->pose.screenPos.y;

    float tailLen = std::max(3.0f, bodyH * 0.90f);
    for (int i = 0; i < 4; ++i) {
        float t0 = (float)i / 4.0f;
        float t1 = (float)(i + 1) / 4.0f;
        unsigned char a = (unsigned char)(115.0f * (1.0f - t0));
        float thick = std::max(1.0f, bodyW * (0.75f - t0 * 0.35f));
        DrawLineEx({x, y - t0 * tailLen}, {x, y - t1 * tailLen}, thick, {255, 110, 110, a});
    }

    DrawCircleV({x, y}, std::max(1.0f, bodyW * 0.90f), {255, 120, 120, 75});

    if (this->resMgr->isBulletEnemyImageValid()) {
        Texture2D tex = *this->resMgr->getBulletEnemyImage();
        Rectangle src = {0.0f, 0.0f, (float)tex.width, (float)tex.height};
        Rectangle dst = {x, y, bodyW, bodyH};
        Vector2 org = {bodyW * 0.5f, bodyH * 0.5f};
        DrawTexturePro(tex, src, dst, org, 0.0f, WHITE);
    } else {
        DrawRectangle((int)(x - bodyW * 0.5f), (int)(y - bodyH * 0.5f), (int)bodyW, (int)bodyH, {255, 100, 100, 255});
    }
}

Enemy::Enemy(float _laneX, float _depthZ, float _speed, ResourceManager* _resMgr)
    : GameObject(0.0, 0.0, GameConfig::S(40), GameConfig::S(40)), resMgr(_resMgr), advanceSpeed(_speed) {
    this->laneX = _laneX;
    this->depthZ = _depthZ;
    this->baseRadius = (float)GameConfig::S(13);
}

Enemy::~Enemy() {}

bool Enemy::move(float dt) {
    this->depthZ += this->advanceSpeed * dt;
    return this->depthZ < 1.01f;
}

void Enemy::draw() {
    float w = this->width * this->pose.screenScale;
    float h = this->height * this->pose.screenScale;
    float x = this->pose.screenPos.x;
    float y = this->pose.screenPos.y;

    if (this->resMgr->isEnemyImageValid()) {
        Texture2D tex = *this->resMgr->getEnemyImage();
        Rectangle src = {0.0f, 0.0f, (float)tex.width, (float)tex.height};
        Rectangle dst = {x, y, w, h};
        Vector2 org = {w * 0.5f, h * 0.5f};

        ShipVolumeStyle style;
        style.thicknessLayers = 4;
        style.maxThicknessPx = 5.5f;
        style.shadowBoost = 1.10f;
        style.highlightAlpha = 68;
        style.rimAlpha = 52;
        GraphicsEngine::drawVolumetricSprite(tex, src, dst, org, 180.0f, style, WHITE);
    } else {
        DrawTriangle({x - w * 0.5f, y - h * 0.45f}, {x + w * 0.5f, y - h * 0.45f}, {x, y + h * 0.50f}, GameConfig::COLOR_ENEMY);
        BeginBlendMode(BLEND_ADDITIVE);
        DrawLineEx({x - w * 0.20f, y - h * 0.28f}, {x + w * 0.20f, y - h * 0.28f}, std::max(1.0f, w * 0.07f), {255, 230, 190, 70});
        DrawLineEx({x - w * 0.22f, y - h * 0.20f}, {x - w * 0.28f, y + h * 0.24f}, std::max(1.0f, w * 0.04f), {120, 220, 255, 54});
        DrawLineEx({x + w * 0.22f, y - h * 0.20f}, {x + w * 0.28f, y + h * 0.24f}, std::max(1.0f, w * 0.04f), {120, 220, 255, 54});
        EndBlendMode();
    }
}

Player::Player(ResourceManager* _resMgr)
    : GameObject(0.0, 0.0, GameConfig::S(32), GameConfig::S(32)), resMgr(_resMgr), motionState() {
    this->laneX = 0.0f;
    this->depthZ = 0.86f;
    this->baseRadius = (float)GameConfig::S(12);
}

Player::~Player() {}

bool Player::move(float dt) {
    if (dt <= 0.0f) {
        return true;
    }

    float inputX = 0.0f;
    float inputZ = 0.0f;

    if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT)) inputX -= 1.0f;
    if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT)) inputX += 1.0f;
    if (IsKeyDown(KEY_W) || IsKeyDown(KEY_UP)) inputZ -= 1.0f;
    if (IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN)) inputZ += 1.0f;

    const float accelLane = 4.8f;
    const float accelDepth = 2.0f;
    const float dampingLane = 8.0f;
    const float dampingDepth = 8.0f;
    const float maxLaneVel = 1.65f;
    const float maxDepthVel = 0.70f;

    this->motionState.accel = {inputX * accelLane, inputZ * accelDepth};
    this->motionState.velocity.x += this->motionState.accel.x * dt;
    this->motionState.velocity.y += this->motionState.accel.y * dt;

    if (inputX == 0.0f) this->motionState.velocity.x *= std::exp(-dampingLane * dt);
    if (inputZ == 0.0f) this->motionState.velocity.y *= std::exp(-dampingDepth * dt);

    this->motionState.velocity.x = ClampFloat(this->motionState.velocity.x, -maxLaneVel, maxLaneVel);
    this->motionState.velocity.y = ClampFloat(this->motionState.velocity.y, -maxDepthVel, maxDepthVel);

    this->laneX += this->motionState.velocity.x * dt;
    this->depthZ += this->motionState.velocity.y * dt;

    this->laneX = ClampFloat(this->laneX, -1.0f, 1.0f);
    this->depthZ = ClampFloat(this->depthZ, 0.76f, 0.95f);

    float targetTilt = -(this->motionState.velocity.x / maxLaneVel) * 12.0f;
    float blend = 1.0f - std::exp(-14.0f * dt);
    this->motionState.tiltDeg = LerpFloat(this->motionState.tiltDeg, targetTilt, blend);
    this->motionState.recoil *= std::exp(-18.0f * dt);

    if (std::fabs(this->motionState.tiltDeg) < 0.02f) this->motionState.tiltDeg = 0.0f;
    if (std::fabs(this->motionState.recoil) < 0.02f) this->motionState.recoil = 0.0f;

    return true;
}

void Player::draw() {
    float w = this->width * this->pose.screenScale;
    float h = this->height * this->pose.screenScale;
    float x = this->pose.screenPos.x;
    float y = this->pose.screenPos.y - this->motionState.recoil - (std::fabs(this->motionState.tiltDeg) / 12.0f) * 3.0f;

    if (this->resMgr->isPlayerImageValid()) {
        Texture2D tex = *this->resMgr->getPlayerImage();
        Rectangle src = {0.0f, 0.0f, (float)tex.width, (float)tex.height};
        Rectangle dst = {x, y, w, h};
        Vector2 org = {w * 0.5f, h * 0.5f};

        ShipVolumeStyle style;
        style.thicknessLayers = 3;
        style.maxThicknessPx = 5.0f;
        style.shadowBoost = 1.10f;
        style.highlightAlpha = 78;
        style.rimAlpha = 60;
        GraphicsEngine::drawVolumetricSprite(tex, src, dst, org, this->motionState.tiltDeg, style, WHITE);
    } else {
        DrawTriangle({x, y - h * 0.50f}, {x - w * 0.50f, y + h * 0.50f}, {x + w * 0.50f, y + h * 0.50f}, GameConfig::COLOR_PLAYER);
        DrawCircleV({x, y - h * 0.25f}, std::max(1.0f, w * 0.10f), WHITE);
        BeginBlendMode(BLEND_ADDITIVE);
        DrawLineEx({x - w * 0.18f, y - h * 0.28f}, {x + w * 0.18f, y - h * 0.28f}, std::max(1.0f, w * 0.07f), {255, 230, 190, 74});
        DrawLineEx({x - w * 0.18f, y - h * 0.18f}, {x - w * 0.24f, y + h * 0.20f}, std::max(1.0f, w * 0.04f), {120, 220, 255, 64});
        DrawLineEx({x + w * 0.18f, y - h * 0.18f}, {x + w * 0.24f, y + h * 0.20f}, std::max(1.0f, w * 0.04f), {120, 220, 255, 64});
        EndBlendMode();
    }
}

void Player::triggerRecoil(float amount) {
    this->motionState.recoil = ClampFloat(this->motionState.recoil + amount, 0.0f, 5.0f);
}

const PlayerMotionState& Player::getMotionState() const {
    return this->motionState;
}

Button::Button()
    : x(0), y(0), w(0), h(0), text(""), isHover(false), isPressedVisual(false), fontSize(16), baseColor({255, 255, 255, 255}), fontName("微软雅黑") {
}

Button::Button(int _x, int _y, int _w, int _h, string _text, int _fs, Color _c, string _fn)
    : x(_x), y(_y), w(_w), h(_h), text(_text), isHover(false), isPressedVisual(false), fontSize(_fs), baseColor(_c), fontName(_fn) {
}

Button::~Button() {}

void Button::draw() {
    float scale = this->isPressedVisual ? 0.96f : 1.0f;
    float cx = this->x + this->w * 0.5f;
    float cy = this->y + this->h * 0.5f;
    float drawW = this->w * scale;
    float drawH = this->h * scale;

    Rectangle mainRect = {cx - drawW * 0.5f, cy - drawH * 0.5f, drawW, drawH};
    Rectangle shadowRect = {mainRect.x + GameConfig::S(2), mainRect.y + GameConfig::S(2), drawW, drawH};

    float roundness = 0.0f;
    int minSide = std::min((int)drawW, (int)drawH);
    int cornerRadius = GameConfig::S(6);
    if (minSide > 0) {
        roundness = (float)(cornerRadius * 2) / (float)minSide;
        roundness = std::min(1.0f, roundness);
    }

    if (this->isHover) {
        Rectangle glowRect = {mainRect.x - GameConfig::S(3), mainRect.y - GameConfig::S(3), mainRect.width + GameConfig::S(6), mainRect.height + GameConfig::S(6)};
        DrawRectangleRounded(glowRect, roundness, 8, {170, 235, 255, 55});
    }

    DrawRectangleRounded(shadowRect, roundness, 8, {8, 10, 20, 190});

    int r = std::min(255, (int)this->baseColor.r + (this->isHover ? 45 : 0));
    int g = std::min(255, (int)this->baseColor.g + (this->isHover ? 45 : 0));
    int b = std::min(255, (int)this->baseColor.b + (this->isHover ? 45 : 0));
    DrawRectangleRounded(mainRect, roundness, 8, {(unsigned char)r, (unsigned char)g, (unsigned char)b, 255});

    Color borderColor = this->isHover
        ? Color{240, 248, 255, 255}
        : Color{(unsigned char)std::min(255, r + 24), (unsigned char)std::min(255, g + 24), (unsigned char)std::min(255, b + 24), 255};

    DrawRectangleRoundedLinesEx(mainRect, roundness, 8, (float)std::max(2, GameConfig::S(1)), borderColor);
    GraphicsEngine::drawFxTextCenter((int)(mainRect.x + mainRect.width * 0.5f), (int)(mainRect.y + mainRect.height * 0.5f),
                                     this->text.c_str(), this->fontSize, this->isHover ? 0.72f : 0.58f, 0.0f, this->fontName.c_str());
}

bool Button::update(float mouseX, float mouseY, bool mousePressed, bool mouseDown) {
    this->isHover = MathUtils::isPointInRect(mouseX, mouseY, this->x, this->y, this->w, this->h);
    this->isPressedVisual = this->isHover && mouseDown;
    return this->isHover && mousePressed;
}

void Button::setPosition(int _x, int _y) {
    this->x = _x;
    this->y = _y;
}

void Button::setSize(int _w, int _h) {
    this->w = _w;
    this->h = _h;
}

void Button::setText(const string& str) {
    this->text = str;
}

bool Button::getHoverState() const {
    return this->isHover;
}

ParticleSystem::ParticleSystem() : particles(), spawnedThisFrame(0) {
    this->clear();
}

void ParticleSystem::clear() {
    for (size_t i = 0; i < this->particles.size(); ++i) {
        this->particles[i] = Particle();
    }
    this->spawnedThisFrame = 0;
}

void ParticleSystem::beginFrame() {
    this->spawnedThisFrame = 0;
}

bool ParticleSystem::emit(const Particle& p) {
    if (this->spawnedThisFrame >= MAX_SPAWN_PER_FRAME) {
        return false;
    }

    int freeIndex = -1;
    for (int i = 0; i < (int)this->particles.size(); ++i) {
        if (!this->particles[(size_t)i].active) {
            freeIndex = i;
            break;
        }
    }

    int targetIndex = freeIndex;
    if (targetIndex < 0) {
        int lowestPriority = std::numeric_limits<int>::max();
        float lowestLifeRatio = std::numeric_limits<float>::max();
        int replaceIndex = -1;

        for (int i = 0; i < (int)this->particles.size(); ++i) {
            const Particle& cur = this->particles[(size_t)i];
            if (!cur.active) continue;

            float ratio = (cur.maxLife > 0.0f) ? (cur.life / cur.maxLife) : 0.0f;
            if (cur.priority < lowestPriority || (cur.priority == lowestPriority && ratio < lowestLifeRatio)) {
                lowestPriority = cur.priority;
                lowestLifeRatio = ratio;
                replaceIndex = i;
            }
        }

        if (replaceIndex >= 0 && this->particles[(size_t)replaceIndex].priority <= p.priority) {
            targetIndex = replaceIndex;
        }
    }

    if (targetIndex < 0) {
        return false;
    }

    Particle next = p;
    next.active = true;
    if (next.maxLife <= 0.0f) next.maxLife = 0.001f;
    if (next.life <= 0.0f) next.life = next.maxLife;

    this->particles[(size_t)targetIndex] = next;
    this->spawnedThisFrame++;
    return true;
}

void ParticleSystem::update(float dt) {
    if (dt <= 0.0f) return;

    for (size_t i = 0; i < this->particles.size(); ++i) {
        Particle& p = this->particles[i];
        if (!p.active) continue;

        p.life -= dt;
        if (p.life <= 0.0f) {
            p.active = false;
            continue;
        }

        p.position.x += p.velocity.x * dt;
        p.position.y += p.velocity.y * dt;
        p.rotation += p.spin * dt;

        float drag = ClampFloat(1.0f - dt * 2.6f, 0.0f, 1.0f);
        p.velocity.x *= drag;
        p.velocity.y *= drag;
    }
}

void ParticleSystem::draw() const {
    for (size_t i = 0; i < this->particles.size(); ++i) {
        const Particle& p = this->particles[i];
        if (!p.active) continue;

        float t = 1.0f - ClampFloat(p.life / p.maxLife, 0.0f, 1.0f);
        Color c = LerpColor(p.startColor, p.endColor, t);
        float radius = std::max(1.0f, p.size * (1.0f - t * 0.35f));

        DrawCircleV(p.position, radius, c);
        DrawCircleV(p.position, radius * 0.45f, {255, 255, 255, (unsigned char)(c.a * 0.45f)});
    }
}

int ParticleSystem::aliveCount() const {
    int count = 0;
    for (size_t i = 0; i < this->particles.size(); ++i) {
        if (this->particles[i].active) count++;
    }
    return count;
}
ChipMusicEngine::ChipMusicEngine()
    : initialized(false),
      muted(false),
      stream(),
      sampleRate(44100),
      chunkFrames(1024),
      buffer(),
      globalTime(0.0f),
      phase1(0.0f),
      phase2(0.0f),
      phase3(0.0f),
      lowpassState(0.0f),
      stepIndex(0),
      stepTime(0.0f),
      stepDuration(60.0f / 150.0f / 4.0f),
      section(0),
      barIndex(0),
      stepInBar(0),
      arrangementIndex(0),
      fillCountdown(0),
      noiseState(0x12345678u) {
}

ChipMusicEngine::~ChipMusicEngine() {
    this->shutdown();
}

float ChipMusicEngine::midiToFreq(int midiNote) const {
    return 440.0f * std::pow(2.0f, (midiNote - 69) / 12.0f);
}

float ChipMusicEngine::squareWave(float phase, float duty) const {
    return (phase < duty) ? 1.0f : -1.0f;
}

float ChipMusicEngine::triWave(float phase) const {
    return 4.0f * std::fabs(phase - 0.5f) - 1.0f;
}

float ChipMusicEngine::nextNoise() {
    this->noiseState = this->noiseState * 1664525u + 1013904223u;
    uint32_t v = (this->noiseState >> 8) & 0xFFFFu;
    return (float)v / 32767.5f - 1.0f;
}

bool ChipMusicEngine::init() {
    if (this->initialized) {
        return true;
    }

    if (!IsAudioDeviceReady()) {
        InitAudioDevice();
    }

    SetAudioStreamBufferSizeDefault(this->chunkFrames);
    this->stream = LoadAudioStream((unsigned int)this->sampleRate, 32, 1);
    PlayAudioStream(this->stream);

    this->buffer.assign((size_t)this->chunkFrames, 0.0f);
    this->initialized = true;
    return true;
}

void ChipMusicEngine::generateChunk() {
    static const int melodyA[16] = {79, 82, 86, 82, 79, 74, 79, 82, 86, 89, 86, 82, 79, 74, 77, 79};
    static const int melodyAp[16] = {79, 82, 86, 89, 91, 89, 86, 82, 79, 82, 84, 86, 84, 82, 79, 77};
    static const int melodyB[16] = {74, 77, 81, 84, 81, 77, 74, 77, 79, 82, 86, 89, 86, 82, 79, 82};
    static const int melodyBp[16] = {74, 77, 81, 86, 84, 81, 77, 74, 79, 82, 86, 91, 89, 86, 82, 79};
    static const int melodyFill[16] = {91, 89, 86, 84, 82, 81, 79, 77, 76, 77, 79, 81, 82, 84, 86, 89};

    static const int bassA[16] = {43, 0, 43, 0, 38, 0, 38, 0, 41, 0, 41, 0, 36, 0, 38, 0};
    static const int bassB[16] = {38, 0, 38, 0, 41, 0, 41, 0, 43, 0, 43, 0, 36, 0, 38, 0};
    static const int bassFill[16] = {43, 43, 41, 41, 38, 38, 36, 36, 38, 38, 41, 41, 43, 43, 46, 46};

    static const int arpA[16] = {67, 71, 74, 79, 62, 67, 71, 74, 64, 67, 71, 76, 60, 64, 67, 71};
    static const int arpB[16] = {62, 65, 69, 74, 64, 67, 71, 76, 67, 71, 74, 79, 60, 64, 67, 71};
    static const int arpFill[16] = {71, 74, 79, 83, 69, 72, 76, 81, 67, 71, 74, 79, 64, 67, 71, 76};

    static const int kickA[16] = {1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 1, 1, 0, 0, 0};
    static const int snareA[16] = {0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0};
    static const int ghostA[16] = {0, 1, 0, 0, 0, 1, 0, 1, 0, 1, 0, 0, 0, 1, 0, 1};

    static const int kickB[16] = {1, 0, 0, 1, 0, 0, 1, 0, 1, 0, 0, 1, 0, 1, 0, 0};
    static const int snareB[16] = {0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0};
    static const int ghostB[16] = {0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1};

    static const int kickFill[16] = {1, 1, 0, 1, 1, 0, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1};
    static const int snareFill[16] = {0, 0, 1, 0, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 1, 1};
    static const int ghostFill[16] = {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1};

    float dt = 1.0f / (float)this->sampleRate;

    for (int i = 0; i < this->chunkFrames; ++i) {
        this->globalTime += dt;
        this->stepTime += dt;

        while (this->stepTime >= this->stepDuration) {
            this->stepTime -= this->stepDuration;
            this->stepInBar = (this->stepInBar + 1) & 15;
            this->stepIndex = this->stepInBar;

            if (this->fillCountdown > 0) {
                this->fillCountdown--;
            }

            if (this->stepInBar == 0) {
                this->barIndex = (this->barIndex + 1) & 7;
                this->arrangementIndex = (this->arrangementIndex + 1) & 127;
                if (this->barIndex == 6) {
                    this->fillCountdown = 16;
                }
            }

            if (this->barIndex < 2) this->section = 0;       // A
            else if (this->barIndex < 4) this->section = 1;  // A'
            else if (this->barIndex < 6) this->section = 2;  // B
            else if (this->barIndex == 6) this->section = 3; // Fill
            else this->section = 4;                          // B'
        }

        float stepPhase = this->stepTime / this->stepDuration;
        float gate = 1.0f - ClampFloat(stepPhase, 0.0f, 1.0f);
        int s = this->stepInBar & 15;

        const int* melody = melodyA;
        const int* bass = bassA;
        const int* arp = arpA;
        const int* kickPattern = kickA;
        const int* snarePattern = snareA;
        const int* ghostPattern = ghostA;

        if (this->section == 1) {
            melody = melodyAp;
        } else if (this->section == 2) {
            melody = melodyB;
            bass = bassB;
            arp = arpB;
            kickPattern = kickB;
            snarePattern = snareB;
            ghostPattern = ghostB;
        } else if (this->section == 3) {
            melody = melodyFill;
            bass = bassFill;
            arp = arpFill;
            kickPattern = kickFill;
            snarePattern = snareFill;
            ghostPattern = ghostFill;
        } else if (this->section == 4) {
            melody = melodyBp;
            bass = bassB;
            arp = arpB;
            kickPattern = kickB;
            snarePattern = snareB;
            ghostPattern = ghostB;
        }

        int noteMain = melody[s];
        int noteBass = bass[s];
        int arpIdx = (s + ((int)(stepPhase * 4.0f) & 3)) & 15;
        int noteArp = arp[arpIdx];

        float ch1 = 0.0f;
        float ch2 = 0.0f;
        float ch3 = 0.0f;

        float duty1 = ClampFloat(0.25f + 0.03f * (((this->barIndex + this->arrangementIndex) & 1) ? 1.0f : -1.0f), 0.18f, 0.34f);
        float duty2 = ClampFloat(0.50f + 0.02f * (((this->section == 2 || this->section == 4) ? 1.0f : -1.0f)), 0.44f, 0.56f);
        float gateMain = std::pow(gate, (this->section == 3) ? 0.36f : 0.52f);
        float gateBass = std::pow(gate, (this->section == 3) ? 0.56f : 0.72f);
        float gateArp = std::pow(gate, (this->section == 3) ? 0.70f : 0.88f);

        if (noteMain > 0) {
            float f1 = this->midiToFreq(noteMain);
            this->phase1 += f1 * dt;
            if (this->phase1 >= 1.0f) this->phase1 -= std::floor(this->phase1);
            ch1 = this->squareWave(this->phase1, duty1) * gateMain;
        }

        if (noteBass > 0) {
            float f2 = this->midiToFreq(noteBass);
            this->phase2 += f2 * dt;
            if (this->phase2 >= 1.0f) this->phase2 -= std::floor(this->phase2);
            ch2 = this->squareWave(this->phase2, duty2) * gateBass;
        }

        if (noteArp > 0) {
            float f3 = this->midiToFreq(noteArp);
            this->phase3 += f3 * dt;
            if (this->phase3 >= 1.0f) this->phase3 -= std::floor(this->phase3);
            ch3 = this->triWave(this->phase3) * (0.62f * gateArp);
        }

        float kick = 0.0f;
        if (kickPattern[s] && stepPhase < 0.30f) {
            float e = std::exp(-stepPhase * ((this->section == 3) ? 20.0f : 18.0f));
            float freq = 55.0f + ((this->section == 3) ? 120.0f : 90.0f) * (1.0f - stepPhase);
            kick = std::sin(2.0f * kPi * freq * this->globalTime) * e;
        }

        float snare = 0.0f;
        if (snarePattern[s] && stepPhase < 0.24f) {
            float e = std::exp(-stepPhase * 24.0f);
            snare = this->nextNoise() * e;
        }

        float ghost = 0.0f;
        if (ghostPattern[s] && stepPhase < 0.14f) {
            ghost = this->nextNoise() * std::exp(-stepPhase * 32.0f) * 0.20f;
        }

        float mix = ch1 * 0.41f + ch2 * 0.35f + ch3 * 0.20f + kick * 0.64f + snare * 0.40f + ghost;
        if (this->section == 3 || this->fillCountdown > 0) {
            mix *= 1.06f;
        }
        if (this->muted) {
            mix = 0.0f;
        }

        mix *= 0.28f;

        float lpAlpha = 0.14f + 0.02f * ((this->barIndex & 1) ? 1.0f : 0.0f) +
                        0.02f * ((this->section == 2 || this->section == 4) ? 1.0f : 0.0f);
        lpAlpha = ClampFloat(lpAlpha, 0.12f, 0.21f);
        this->lowpassState += (mix - this->lowpassState) * lpAlpha;
        float soft = (float)std::tanh(this->lowpassState * 1.9f) * 0.92f;
        this->buffer[(size_t)i] = soft;
    }
}

void ChipMusicEngine::update(float dt) {
    (void)dt;
    if (!this->initialized) {
        return;
    }

    while (IsAudioStreamProcessed(this->stream)) {
        this->generateChunk();
        UpdateAudioStream(this->stream, this->buffer.data(), this->chunkFrames);
    }
}

void ChipMusicEngine::toggleMute() {
    this->muted = !this->muted;
}

void ChipMusicEngine::shutdown() {
    if (this->initialized) {
        StopAudioStream(this->stream);
        UnloadAudioStream(this->stream);
        this->initialized = false;
    }

    if (IsAudioDeviceReady()) {
        CloseAudioDevice();
    }
}

bool ChipMusicEngine::isMuted() const {
    return this->muted;
}

GameManager::GameManager()
    : resourceManager(),
      currentState(MENU),
      player(nullptr),
      bullets(),
      enemies(),
      enemyBullets(),
      score(0),
      gameOver(false),
      deltaTime(1.0f / 60.0f),
      uiTime(0.0f),
      shootCooldown(0.0f),
      pauseCooldown(0.0f),
      enemySpawnRate(30),
      enemyShootChance(2),
      enemySpawnTimer(0.5f),
      enemyAdvanceSpeed(0.33f),
      enemyBulletSpeed(0.66f),
      hitStopTimer(0.0f),
      screenFlashAlpha(0.0f),
      endScoreAnimTimer(0.0f),
      animatedEndScore(0),
      titleDrift(),
      hudDrift(),
      microDrift(),
      cameraFX(),
      particleSystem(),
      chipMusic(),
      perspectiveCfg(),
      perspectiveMapper(),
      parallaxLayers(),
      farStars(),
      midClouds(),
      pendingHitPos{0.0f, 0.0f},
      pendingHitFX(false),
      btnEasy(),
      btnNormal(),
      btnHell(),
      btnPause(),
      btnResume(),
      btnMenu() {
    srand((unsigned int)time(nullptr));

    SetConfigFlags(FLAG_WINDOW_HIGHDPI | FLAG_MSAA_4X_HINT);
    InitWindow(GameConfig::GetWindowWidth(), GameConfig::GetWindowHeight(), "PlaneFight (raylib)");
    SetTargetFPS(60);
    SetExitKey(KEY_NULL);

    this->resourceManager.loadAllResources();

    this->perspectiveCfg.horizonY = (float)GameConfig::S(54);
    this->perspectiveCfg.bottomY = (float)(GameConfig::GetWindowHeight() - GameConfig::S(10));
    this->perspectiveCfg.laneHalfFar = (float)GameConfig::S(24);
    this->perspectiveCfg.laneHalfNear = (float)GameConfig::S(126);
    this->perspectiveCfg.minDepthZ = 0.0f;
    this->perspectiveCfg.maxDepthZ = 1.0f;
    this->perspectiveMapper.setConfig(this->perspectiveCfg);

    this->initUI();
    this->initBackgroundLayers();
    this->chipMusic.init();

    this->titleDrift.amp = 2.0f;
    this->hudDrift.amp = 1.2f;
    this->microDrift.amp = 0.7f;
    this->titleDrift.retargetTimer = 0.45f;
    this->hudDrift.retargetTimer = 0.35f;
    this->microDrift.retargetTimer = 0.30f;

    this->resetGame();
}

GameManager::~GameManager() {
    this->clearEntities();
    this->chipMusic.shutdown();
    if (!IsWindowReady()) {
        return;
    }
    CloseWindow();
}

void GameManager::initUI() {
    int winW = GameConfig::GetWindowWidth();
    int winH = GameConfig::GetWindowHeight();

    int btnW = GameConfig::S(96);
    int btnH = GameConfig::S(28);
    int btnX = winW / 2 - btnW / 2;

    this->btnEasy = Button(btnX, GameConfig::S(95), btnW, btnH, Texts::BTN_EASY, GameConfig::S(13), {40, 160, 80, 255}, "ui");
    this->btnNormal = Button(btnX, GameConfig::S(135), btnW, btnH, Texts::BTN_NORMAL, GameConfig::S(13), {50, 120, 210, 255}, "ui");
    this->btnHell = Button(btnX, GameConfig::S(175), btnW, btnH, Texts::BTN_HELL, GameConfig::S(13), {180, 50, 50, 255}, "ui");

    this->btnPause = Button(winW - GameConfig::S(45), GameConfig::S(5), GameConfig::S(40), GameConfig::S(20), Texts::BTN_PAUSE, GameConfig::S(12), {50, 120, 210, 255}, "ui");
    this->btnMenu = Button(winW - GameConfig::S(90), GameConfig::S(5), GameConfig::S(40), GameConfig::S(20), Texts::BTN_MENU, GameConfig::S(12), {200, 80, 80, 255}, "ui");
    this->btnResume = Button(winW / 2 - GameConfig::S(45), winH / 2 + GameConfig::S(10), GameConfig::S(90), GameConfig::S(28), Texts::BTN_RESUME, GameConfig::S(14), {50, 120, 210, 255}, "ui");
}

void GameManager::initBackgroundLayers() {
    this->parallaxLayers[0].speed = (float)GameConfig::S(20);
    this->parallaxLayers[0].offsetY = 0.0f;
    this->parallaxLayers[0].alpha = 0.70f;
    this->parallaxLayers[0].density = 120;
    this->parallaxLayers[0].drift = 2.0f;
    this->parallaxLayers[0].color = {220, 232, 255, 120};

    this->parallaxLayers[1].speed = (float)GameConfig::S(32);
    this->parallaxLayers[1].offsetY = 0.0f;
    this->parallaxLayers[1].alpha = 0.30f;
    this->parallaxLayers[1].density = 34;
    this->parallaxLayers[1].drift = 12.0f;
    this->parallaxLayers[1].color = {152, 125, 210, 70};

    int winW = GameConfig::GetWindowWidth();
    int winH = GameConfig::GetWindowHeight();

    this->farStars.clear();
    this->farStars.reserve((size_t)this->parallaxLayers[0].density);
    for (int i = 0; i < this->parallaxLayers[0].density; ++i) {
        this->farStars.push_back({RandomRange(0.0f, (float)winW), RandomRange(0.0f, (float)winH)});
    }

    this->midClouds.clear();
    this->midClouds.reserve((size_t)this->parallaxLayers[1].density);
    for (int i = 0; i < this->parallaxLayers[1].density; ++i) {
        this->midClouds.push_back({RandomRange(0.0f, (float)winW), RandomRange(0.0f, (float)winH)});
    }
}

void GameManager::clearEntities() {
    if (this->player != nullptr) {
        delete this->player;
        this->player = nullptr;
    }

    for (auto b : this->bullets) delete b;
    this->bullets.clear();

    for (auto e : this->enemies) delete e;
    this->enemies.clear();

    for (auto eb : this->enemyBullets) delete eb;
    this->enemyBullets.clear();
}

void GameManager::setDifficulty(int spawnRate, int shootChance) {
    this->enemySpawnRate = std::max(1, spawnRate);
    this->enemyShootChance = std::max(0, shootChance);

    this->enemySpawnTimer = std::max(0.05f, this->enemySpawnRate / 60.0f);
    this->enemyAdvanceSpeed = 0.24f + (60.0f - (float)this->enemySpawnRate) / 140.0f;
    this->enemyBulletSpeed = 0.58f + this->enemyShootChance * 0.032f;
}

void GameManager::resetGame() {
    this->clearEntities();

    this->player = new Player(&this->resourceManager);
    this->player->updatePerspective(this->perspectiveCfg);

    this->score = 0;
    this->gameOver = false;
    this->shootCooldown = 0.0f;
    this->hitStopTimer = 0.0f;
    this->screenFlashAlpha = 0.0f;
    this->cameraFX = CameraFXState();
    this->particleSystem.clear();
    this->pendingHitFX = false;
    this->endScoreAnimTimer = 0.0f;
    this->animatedEndScore = 0;

    this->enemySpawnTimer = std::max(0.05f, this->enemySpawnRate / 60.0f);
}

void GameManager::enterEndState() {
    if (this->currentState == END) {
        return;
    }

    this->currentState = END;
    this->endScoreAnimTimer = 0.0f;
    this->animatedEndScore = 0;
    this->pauseCooldown = 0.20f;
}

void GameManager::spawnMuzzleFX() {
    if (this->player == nullptr) {
        return;
    }

    const PerspectivePose& p = this->player->getPose();
    float gunY = p.screenPos.y - this->player->getHeight() * p.screenScale * 0.33f;
    float wing = this->player->getWidth() * p.screenScale * 0.25f;

    float gunX[2] = {p.screenPos.x - wing, p.screenPos.x + wing};

    for (int g = 0; g < 2; ++g) {
        int count = 6 + rand() % 5;
        for (int i = 0; i < count; ++i) {
            Particle particle;
            particle.active = true;
            particle.position = {gunX[g] + RandomRange(-2.0f, 2.0f), gunY + RandomRange(-2.0f, 2.0f)};
            particle.velocity = {RandomRange(-55.0f, 55.0f), RandomRange(-420.0f, -250.0f)};
            particle.maxLife = RandomRange(0.12f, 0.18f);
            particle.life = particle.maxLife;
            particle.size = RandomRange((float)GameConfig::S(1), (float)GameConfig::S(3));
            particle.rotation = RandomRange(0.0f, 360.0f);
            particle.spin = RandomRange(-120.0f, 120.0f);
            particle.startColor = {120, 220, 255, 230};
            particle.endColor = {80, 160, 255, 0};
            particle.priority = 1;
            this->particleSystem.emit(particle);
        }
    }
}

void GameManager::spawnHitFX() {
    if (!this->pendingHitFX) {
        return;
    }

    int count = 20 + rand() % 9;
    for (int i = 0; i < count; ++i) {
        float angle = RandomRange(0.0f, kTau);
        float speed = RandomRange(130.0f, 360.0f);

        Particle p;
        p.active = true;
        p.position = this->pendingHitPos;
        p.velocity = {std::cos(angle) * speed, std::sin(angle) * speed};
        p.maxLife = RandomRange(0.16f, 0.28f);
        p.life = p.maxLife;
        p.size = RandomRange((float)GameConfig::S(1), (float)GameConfig::S(4));
        p.rotation = RandomRange(0.0f, 360.0f);
        p.spin = RandomRange(-200.0f, 200.0f);
        p.startColor = {255, 220, 150, 240};
        p.endColor = {255, 80, 40, 0};
        p.priority = 3;
        this->particleSystem.emit(p);
    }

    this->screenFlashAlpha = std::max(this->screenFlashAlpha, 34.0f);
    this->cameraFX.trauma = ClampFloat(this->cameraFX.trauma + 0.18f, 0.0f, 1.0f);
    this->hitStopTimer = std::max(this->hitStopTimer, 0.035f);
    this->pendingHitFX = false;
}

void GameManager::updateParallax(float dt) {
    if (dt <= 0.0f) return;

    float speedBoost = 1.0f;
    if (this->player != nullptr && this->currentState == PLAYING) {
        const PlayerMotionState& ms = this->player->getMotionState();
        float laneV = std::fabs(ms.velocity.x);
        float depthV = std::fabs(ms.velocity.y);
        speedBoost += ClampFloat((laneV + depthV) / 2.0f, 0.0f, 1.0f) * 0.4f;
    }

    float h = (float)GameConfig::GetWindowHeight();
    this->parallaxLayers[0].offsetY = WrapFloat(this->parallaxLayers[0].offsetY + this->parallaxLayers[0].speed * dt * 0.7f * speedBoost, h);
    this->parallaxLayers[1].offsetY = WrapFloat(this->parallaxLayers[1].offsetY + this->parallaxLayers[1].speed * dt * 0.9f * speedBoost, h);
}

void GameManager::updateCameraFX(float dt) {
    this->cameraFX.trauma = std::max(0.0f, this->cameraFX.trauma - 2.2f * dt);

    float shakePower = this->cameraFX.trauma * this->cameraFX.trauma;
    float maxShake = 7.0f;
    this->cameraFX.shakeX = RandomRange(-maxShake, maxShake) * shakePower;
    this->cameraFX.shakeY = RandomRange(-maxShake, maxShake) * shakePower;

    float targetZoom = 1.0f + shakePower * 0.02f;
    float blend = 1.0f - std::exp(-10.0f * dt);
    this->cameraFX.zoom = LerpFloat(this->cameraFX.zoom, targetZoom, blend);

    float playerRoll = 0.0f;
    if (this->player != nullptr) {
        playerRoll = this->player->getMotionState().tiltDeg * 0.12f;
    }

    float rollNoise = RandomRange(-1.8f, 1.8f) * shakePower;
    this->cameraFX.rollDeg = playerRoll + rollNoise;
}

void GameManager::updateUiDrift(float dt) {
    if (dt <= 0.0f) {
        return;
    }

    auto tickDrift = [dt](UiDriftState& state, float retargetMin, float retargetMax) {
        state.retargetTimer -= dt;
        if (state.retargetTimer <= 0.0f) {
            state.target = RandomRange(-state.amp, state.amp);
            state.retargetTimer = RandomRange(retargetMin, retargetMax);
        }

        const float spring = 26.0f;
        const float damping = 8.5f;
        float accel = (state.target - state.x) * spring - state.v * damping;
        state.v += accel * dt;
        state.x += state.v * dt;

        float hardLimit = state.amp * 1.35f;
        state.x = ClampFloat(state.x, -hardLimit, hardLimit);
        if (std::fabs(state.x) < 0.004f && std::fabs(state.v) < 0.004f) {
            state.x = 0.0f;
            state.v = 0.0f;
        }
    };

    tickDrift(this->titleDrift, 0.45f, 0.85f);
    tickDrift(this->hudDrift, 0.32f, 0.64f);
    tickDrift(this->microDrift, 0.26f, 0.48f);
}

void GameManager::layoutHUD() {
    int winW = GameConfig::GetWindowWidth();
    int winH = GameConfig::GetWindowHeight();
    int topPad = GameConfig::S(6);
    int sidePad = GameConfig::S(10);
    int gap = GameConfig::S(4);

    int btnW = GameConfig::S(40);
    int btnH = GameConfig::S(20);
    int pauseX = winW - sidePad - btnW;
    int menuX = pauseX - gap - btnW;

    this->btnPause.setSize(btnW, btnH);
    this->btnMenu.setSize(btnW, btnH);
    this->btnPause.setPosition(pauseX, topPad);
    this->btnMenu.setPosition(menuX, topPad);

    this->btnResume.setSize(GameConfig::S(90), GameConfig::S(28));
    this->btnResume.setPosition(winW / 2 - GameConfig::S(45), winH / 2 + GameConfig::S(10));
}

void GameManager::applyRoadBoundaryClamp(GameObject* obj, float extraMarginPx) {
    if (obj == nullptr) {
        return;
    }

    float depth = ClampFloat(obj->getDepthZ(), this->perspectiveCfg.minDepthZ, this->perspectiveCfg.maxDepthZ);
    float halfWidth = this->perspectiveMapper.laneHalfWidth(depth);
    if (halfWidth <= 0.001f) {
        obj->setLaneX(0.0f);
        return;
    }

    float scale = obj->getScreenScale();
    if (scale <= 0.01f) {
        scale = this->perspectiveMapper.depthToScale(depth);
    }

    float safeRadiusPx = obj->getWidth() * scale * 0.50f;
    if (dynamic_cast<Player*>(obj) != nullptr) {
        safeRadiusPx = obj->getWidth() * scale * 0.42f + (float)GameConfig::S(2);
    } else if (dynamic_cast<Enemy*>(obj) != nullptr) {
        safeRadiusPx = obj->getWidth() * scale * 0.42f + (float)GameConfig::S(1);
    } else if (dynamic_cast<Bullet*>(obj) != nullptr || dynamic_cast<EnemyBullet*>(obj) != nullptr) {
        safeRadiusPx = obj->getWidth() * scale * 0.50f;
    }

    safeRadiusPx += std::max(0.0f, extraMarginPx);
    float maxAbsLane = ClampFloat((halfWidth - safeRadiusPx) / halfWidth, 0.0f, 1.0f);
    obj->setLaneX(ClampFloat(obj->getLaneX(), -maxAbsLane, maxAbsLane));
}

void GameManager::processInput(float worldDt) {
    if (this->player == nullptr) {
        return;
    }

    if (IsKeyPressed(KEY_P) && this->pauseCooldown <= 0.0f) {
        this->currentState = PAUSED;
        this->pauseCooldown = 0.20f;
        return;
    }

    if (worldDt <= 0.0f) {
        return;
    }

    this->player->move(worldDt);
    this->applyRoadBoundaryClamp(this->player, 0.0f);

    if (this->shootCooldown > 0.0f) {
        this->shootCooldown = std::max(0.0f, this->shootCooldown - worldDt);
    }

    if (IsKeyDown(KEY_SPACE) && this->shootCooldown <= 0.0f) {
        float leftLane = ClampFloat(this->player->getLaneX() - 0.060f, -1.0f, 1.0f);
        float rightLane = ClampFloat(this->player->getLaneX() + 0.060f, -1.0f, 1.0f);
        float depth = this->player->getDepthZ() - 0.012f;

        this->bullets.push_back(new Bullet(leftLane, depth, 1.45f, &this->resourceManager));
        this->bullets.push_back(new Bullet(rightLane, depth, 1.45f, &this->resourceManager));

        this->player->triggerRecoil(3.0f);
        this->spawnMuzzleFX();
        this->shootCooldown = 0.10f;
    }
}

void GameManager::updatePerspectiveWorld(float dt) {
    if (this->player != nullptr) {
        this->applyRoadBoundaryClamp(this->player, 0.0f);
        this->player->updatePerspective(this->perspectiveCfg);
    }

    for (auto it = this->bullets.begin(); it != this->bullets.end();) {
        if (dt > 0.0f && !(*it)->move(dt)) {
            delete *it;
            it = this->bullets.erase(it);
        } else {
            if ((*it)->getDepthZ() < -0.03f) {
                delete *it;
                it = this->bullets.erase(it);
                continue;
            }
            this->applyRoadBoundaryClamp(*it, 0.0f);
            (*it)->updatePerspective(this->perspectiveCfg);
            ++it;
        }
    }

    for (auto it = this->enemyBullets.begin(); it != this->enemyBullets.end();) {
        if (dt > 0.0f && !(*it)->move(dt)) {
            delete *it;
            it = this->enemyBullets.erase(it);
        } else {
            if ((*it)->getDepthZ() > 1.02f) {
                delete *it;
                it = this->enemyBullets.erase(it);
                continue;
            }
            this->applyRoadBoundaryClamp(*it, 0.0f);
            (*it)->updatePerspective(this->perspectiveCfg);
            ++it;
        }
    }

    for (auto it = this->enemies.begin(); it != this->enemies.end();) {
        if (dt > 0.0f && !(*it)->move(dt)) {
            delete *it;
            it = this->enemies.erase(it);
        } else {
            if ((*it)->getDepthZ() > 1.01f) {
                delete *it;
                it = this->enemies.erase(it);
                continue;
            }
            this->applyRoadBoundaryClamp(*it, 0.0f);
            (*it)->updatePerspective(this->perspectiveCfg);
            ++it;
        }
    }

    this->bullets.sort([](const Bullet* a, const Bullet* b) {
        return a->getDepthZ() < b->getDepthZ();
    });
    this->enemyBullets.sort([](const EnemyBullet* a, const EnemyBullet* b) {
        return a->getDepthZ() < b->getDepthZ();
    });
    this->enemies.sort([](const Enemy* a, const Enemy* b) {
        return a->getDepthZ() < b->getDepthZ();
    });
}

void GameManager::resolvePerspectiveCollisions() {
    if (this->player == nullptr) {
        return;
    }

    const PerspectivePose& playerPose = this->player->getPose();
    float playerR = playerPose.screenRadius * 0.82f;

    for (auto it = this->enemyBullets.begin(); it != this->enemyBullets.end();) {
        const PerspectivePose& ep = (*it)->getPose();
        float r = ep.screenRadius + playerR;

        if (DistSq(ep.screenPos, playerPose.screenPos) <= r * r) {
            this->gameOver = true;
            this->screenFlashAlpha = std::max(this->screenFlashAlpha, 92.0f);
            this->cameraFX.trauma = ClampFloat(this->cameraFX.trauma + 0.45f, 0.0f, 1.0f);
            this->hitStopTimer = std::max(this->hitStopTimer, 0.045f);

            delete *it;
            it = this->enemyBullets.erase(it);
            continue;
        }
        ++it;
    }

    for (auto it = this->enemies.begin(); it != this->enemies.end();) {
        const PerspectivePose& ep = (*it)->getPose();
        float r = ep.screenRadius + playerR;

        if (DistSq(ep.screenPos, playerPose.screenPos) <= r * r) {
            this->gameOver = true;
            this->screenFlashAlpha = std::max(this->screenFlashAlpha, 95.0f);
            this->cameraFX.trauma = ClampFloat(this->cameraFX.trauma + 0.45f, 0.0f, 1.0f);
            this->hitStopTimer = std::max(this->hitStopTimer, 0.045f);
            break;
        }
        ++it;
    }

    for (auto eIt = this->enemies.begin(); eIt != this->enemies.end();) {
        bool destroyed = false;
        const PerspectivePose& enemyPose = (*eIt)->getPose();

        for (auto bIt = this->bullets.begin(); bIt != this->bullets.end();) {
            const PerspectivePose& bulletPose = (*bIt)->getPose();
            float r = enemyPose.screenRadius + bulletPose.screenRadius;

            if (DistSq(enemyPose.screenPos, bulletPose.screenPos) <= r * r) {
                this->score += 10;
                destroyed = true;
                this->pendingHitPos = enemyPose.screenPos;
                this->pendingHitFX = true;

                delete *bIt;
                bIt = this->bullets.erase(bIt);
                break;
            } else {
                ++bIt;
            }
        }

        if (destroyed) {
            this->spawnHitFX();
            delete *eIt;
            eIt = this->enemies.erase(eIt);
        } else {
            ++eIt;
        }
    }
}

void GameManager::updateGameLogic(float worldDt) {
    if (this->player == nullptr) {
        return;
    }

    if (worldDt > 0.0f) {
        float spawnInterval = std::max(0.05f, this->enemySpawnRate / 60.0f);
        this->enemySpawnTimer -= worldDt;

        while (this->enemySpawnTimer <= 0.0f) {
            float lane = RandomRange(-0.92f, 0.92f);
            this->enemies.push_back(new Enemy(lane, 0.04f, this->enemyAdvanceSpeed, &this->resourceManager));
            this->enemySpawnTimer += spawnInterval;
        }

        float pFrame = this->enemyShootChance / 100.0f;
        float pScaled = 1.0f - std::pow(1.0f - pFrame, worldDt * 60.0f);
        pScaled = ClampFloat(pScaled, 0.0f, 0.95f);

        for (auto e : this->enemies) {
            if (Random01() < pScaled) {
                this->enemyBullets.push_back(new EnemyBullet(e->getLaneX(), e->getDepthZ() + 0.02f, this->enemyBulletSpeed, &this->resourceManager));
            }
        }
    }

    this->updatePerspectiveWorld(worldDt);
    this->resolvePerspectiveCollisions();

    if (worldDt > 0.0f) {
        this->particleSystem.update(worldDt);
    }

    if (this->gameOver) {
        this->enterEndState();
    }
}

void GameManager::drawShadowEllipse(float x, float y, float rw, float rh, unsigned char alpha) const {
    unsigned char a0 = (unsigned char)(alpha * 0.45f);
    unsigned char a1 = (unsigned char)(alpha * 0.25f);
    unsigned char a2 = (unsigned char)(alpha * 0.12f);

    DrawEllipse((int)x, (int)y, rw, rh, {10, 10, 15, a0});
    DrawEllipse((int)x, (int)y, rw * 1.30f, rh * 1.22f, {10, 10, 15, a1});
    DrawEllipse((int)x, (int)y, rw * 1.60f, rh * 1.45f, {10, 10, 15, a2});
}

void GameManager::drawCorridorGuides() {
    float phase = WrapFloat(this->uiTime * 0.85f, 0.09f);

    float laneMarks[3] = {-0.66f, 0.0f, 0.66f};
    for (float lane : laneMarks) {
        for (float d = phase; d < 1.0f; d += 0.09f) {
            float d2 = d + 0.045f;
            if (d2 > 1.0f) break;

            Vector2 p0 = this->perspectiveMapper.projectToScreen(lane, d);
            Vector2 p1 = this->perspectiveMapper.projectToScreen(lane, d2);
            float nearFactor = ClampFloat((d + d2) * 0.5f, 0.0f, 1.0f);
            float thickness = LerpFloat(1.2f, 4.0f, nearFactor);
            unsigned char a = (unsigned char)(45 + 95 * nearFactor);
            DrawLineEx(p0, p1, thickness, {120, 230, 255, a});
        }
    }

    for (float d = phase; d < 1.0f; d += 0.09f) {
        Vector2 l = this->perspectiveMapper.projectToScreen(-1.0f, d);
        Vector2 r = this->perspectiveMapper.projectToScreen(1.0f, d);
        float nearFactor = ClampFloat(d, 0.0f, 1.0f);
        unsigned char a = (unsigned char)(32 + 84 * nearFactor);
        float thickness = LerpFloat(0.9f, 2.4f, nearFactor);
        DrawLineEx(l, r, thickness, {130, 185, 255, a});
    }

    float topY = this->perspectiveCfg.horizonY;
    float bottomY = this->perspectiveCfg.bottomY;
    float cx = GameConfig::GetWindowWidth() * 0.5f;

    for (int i = 1; i <= 4; ++i) {
        float t = i / 5.0f;
        float xL = LerpFloat(0.0f, cx - this->perspectiveCfg.laneHalfFar, t);
        float xR = LerpFloat((float)GameConfig::GetWindowWidth(), cx + this->perspectiveCfg.laneHalfFar, t);
        float sideThick = LerpFloat(0.8f, 1.8f, 1.0f - t);
        DrawLineEx({xL, bottomY}, {cx - this->perspectiveCfg.laneHalfFar, topY}, sideThick, {75, 100, 150, 52});
        DrawLineEx({xR, bottomY}, {cx + this->perspectiveCfg.laneHalfFar, topY}, sideThick, {75, 100, 150, 52});
    }
}

void GameManager::drawCorridorBackground() {
    int winW = GameConfig::GetWindowWidth();
    int winH = GameConfig::GetWindowHeight();

    if (this->resourceManager.isBackgroundImageValid()) {
        Texture2D tex = *this->resourceManager.getBackgroundImage();
        Rectangle src = {0.0f, 0.0f, (float)tex.width, (float)tex.height};
        Rectangle dst = {0.0f, 0.0f, (float)winW, (float)winH};
        DrawTexturePro(tex, src, dst, {0.0f, 0.0f}, 0.0f, {255, 255, 255, 205});
    } else {
        DrawRectangleGradientV(0, 0, winW, winH, {16, 18, 28, 255}, {38, 28, 52, 255});
    }

    const ParallaxLayer& far = this->parallaxLayers[0];
    for (size_t i = 0; i < this->farStars.size(); ++i) {
        Vector2 p = this->farStars[i];
        float y = WrapFloat(p.y + far.offsetY, (float)winH);
        float x = p.x + std::sin((p.y + this->uiTime * 40.0f) * 0.009f) * far.drift;
        x = WrapFloat(x, (float)winW);

        float twinkle = 0.55f + 0.45f * std::sin((p.x * 0.03f) + (p.y * 0.02f) + this->uiTime * 2.4f);
        unsigned char alpha = (unsigned char)(far.color.a * twinkle * far.alpha);
        DrawCircleV({x, y}, 1.0f + twinkle * 1.25f, {far.color.r, far.color.g, far.color.b, alpha});
    }

    const ParallaxLayer& mid = this->parallaxLayers[1];
    for (size_t i = 0; i < this->midClouds.size(); ++i) {
        Vector2 p = this->midClouds[i];
        float y = WrapFloat(p.y + mid.offsetY, (float)winH);
        float x = p.x + std::sin((p.y + this->uiTime * 30.0f) * 0.007f) * mid.drift;
        x = WrapFloat(x, (float)winW);

        float radius = (float)GameConfig::S(15) + (float)((int)i % 3) * GameConfig::S(6);
        Color c0 = {mid.color.r, mid.color.g, mid.color.b, (unsigned char)(mid.color.a * 0.65f)};
        Color c1 = {mid.color.r, mid.color.g, mid.color.b, 0};
        DrawCircleGradient((int)x, (int)y, radius, c0, c1);
    }

    Vector2 topL = {GameConfig::GetWindowWidth() * 0.5f - this->perspectiveCfg.laneHalfFar, this->perspectiveCfg.horizonY};
    Vector2 topR = {GameConfig::GetWindowWidth() * 0.5f + this->perspectiveCfg.laneHalfFar, this->perspectiveCfg.horizonY};
    Vector2 bottomL = {GameConfig::GetWindowWidth() * 0.5f - this->perspectiveCfg.laneHalfNear, this->perspectiveCfg.bottomY};
    Vector2 bottomR = {GameConfig::GetWindowWidth() * 0.5f + this->perspectiveCfg.laneHalfNear, this->perspectiveCfg.bottomY};

    DrawTriangle(topL, bottomL, bottomR, {23, 29, 45, 220});
    DrawTriangle(topL, topR, bottomR, {28, 34, 52, 220});

    DrawTriangle({0.0f, this->perspectiveCfg.horizonY - GameConfig::S(8)}, topL, bottomL, {18, 20, 30, 185});
    DrawTriangle({0.0f, (float)winH}, {0.0f, this->perspectiveCfg.horizonY - GameConfig::S(8)}, bottomL, {16, 18, 28, 195});

    DrawTriangle(topR, {(float)winW, this->perspectiveCfg.horizonY - GameConfig::S(8)}, bottomR, {18, 20, 30, 185});
    DrawTriangle({(float)winW, this->perspectiveCfg.horizonY - GameConfig::S(8)}, {(float)winW, (float)winH}, bottomR, {16, 18, 28, 195});

    this->drawCorridorGuides();

    DrawLineEx(topL, bottomL, 2.0f, {160, 200, 255, 85});
    DrawLineEx(topR, bottomR, 2.0f, {160, 200, 255, 85});

    DrawRectangleGradientV(0, 0, winW, GameConfig::S(22), {0, 0, 0, 80}, {0, 0, 0, 0});
    DrawRectangleGradientV(0, winH - GameConfig::S(42), winW, GameConfig::S(42), {0, 0, 0, 0}, {0, 0, 0, 100});
}

void GameManager::drawBackground() {
    this->drawCorridorBackground();
}

void GameManager::drawWorldWithCamera() {
    if (this->player == nullptr) {
        return;
    }

    Camera2D cam;
    float winW = (float)GameConfig::GetWindowWidth();
    float winH = (float)GameConfig::GetWindowHeight();
    cam.target = {winW * 0.5f, winH * 0.5f};
    cam.offset = {winW * 0.5f + this->cameraFX.shakeX, winH * 0.5f + this->cameraFX.shakeY};
    cam.rotation = this->cameraFX.rollDeg;
    cam.zoom = this->cameraFX.zoom;

    BeginMode2D(cam);

    vector<pair<float, const GameObject*>> objects;
    objects.reserve(this->enemies.size() + this->bullets.size() + this->enemyBullets.size() + 1);

    for (auto e : this->enemies) objects.push_back({e->getDepthZ(), e});
    for (auto b : this->bullets) objects.push_back({b->getDepthZ(), b});
    for (auto eb : this->enemyBullets) objects.push_back({eb->getDepthZ(), eb});
    objects.push_back({this->player->getDepthZ(), this->player});

    std::sort(objects.begin(), objects.end(), [](const pair<float, const GameObject*>& a, const pair<float, const GameObject*>& b) {
        return a.first < b.first;
    });

    for (size_t i = 0; i < objects.size(); ++i) {
        const GameObject* obj = objects[i].second;
        const PerspectivePose& p = obj->getPose();
        float depthFactor = ClampFloat(p.depthZ, 0.0f, 1.0f);
        float rw = std::max(2.0f, p.screenRadius * 0.80f);
        float rh = std::max(1.0f, p.screenRadius * 0.24f);
        unsigned char a = (unsigned char)(55 + 85 * depthFactor);

        bool isMainShip = (dynamic_cast<const Enemy*>(obj) != nullptr) || (dynamic_cast<const Player*>(obj) != nullptr);
        if (isMainShip) {
            rw *= 1.10f;
            rh *= 1.10f;
            a = (unsigned char)std::min(255, (int)a + 8);
        }

        this->drawShadowEllipse(p.screenPos.x, p.screenPos.y + p.screenRadius * 0.85f, rw, rh, a);
    }

    for (size_t i = 0; i < objects.size(); ++i) {
        const_cast<GameObject*>(objects[i].second)->draw();
    }

    this->particleSystem.draw();

    EndMode2D();
}

void GameManager::drawScreenFX() {
    int winW = GameConfig::GetWindowWidth();
    int winH = GameConfig::GetWindowHeight();

    if (this->screenFlashAlpha > 0.0f) {
        unsigned char alpha = (unsigned char)ClampFloat(this->screenFlashAlpha, 0.0f, 255.0f);
        DrawRectangle(0, 0, winW, winH, {224, 238, 255, alpha});
        this->screenFlashAlpha = std::max(0.0f, this->screenFlashAlpha - 340.0f * this->deltaTime);
    }

    DrawRectangleGradientV(0, 0, winW, GameConfig::S(26), {0, 0, 0, 90}, {0, 0, 0, 0});
    DrawRectangleGradientV(0, winH - GameConfig::S(48), winW, GameConfig::S(48), {0, 0, 0, 0}, {0, 0, 0, 110});
    DrawRectangleGradientH(0, 0, GameConfig::S(26), winH, {0, 0, 0, 70}, {0, 0, 0, 0});
    DrawRectangleGradientH(winW - GameConfig::S(26), 0, GameConfig::S(26), winH, {0, 0, 0, 0}, {0, 0, 0, 70});

    for (int y = 0; y < winH; y += 4) {
        unsigned char a = (unsigned char)(10 + 6 * (0.5f + 0.5f * std::sin(this->uiTime * 42.0f + y * 0.045f)));
        DrawLine(0, y, winW, y, {22, 24, 35, a});
    }
}

void GameManager::drawMusicIndicator() {
    int x = 0;
    int y = 0;
    if (this->currentState == MENU) {
        x = GameConfig::GetWindowWidth() - GameConfig::S(36);
        y = GameConfig::S(9);
    } else if (this->currentState == PLAYING || this->currentState == PAUSED) {
        x = GameConfig::S(58);
        y = GameConfig::S(34);
    } else {
        x = GameConfig::S(58);
        y = GameConfig::S(20);
    }

    float drift = this->microDrift.x * 0.45f;
    const char* text = this->chipMusic.isMuted() ? Texts::MUSIC_OFF : Texts::MUSIC_ON;
    GraphicsEngine::drawFxTextCenter(x, y, text, GameConfig::S(8), 0.40f, drift, "ui");
    GraphicsEngine::drawFxTextCenter(x, y + GameConfig::S(10), "M: TOGGLE", GameConfig::S(7), 0.32f, drift * 0.75f, "ui");
}

void GameManager::drawUnifiedUI() {
    int winW = GameConfig::GetWindowWidth();
    int winH = GameConfig::GetWindowHeight();
    int leftPad = GameConfig::S(10);
    int topPad = GameConfig::S(6);

    this->layoutHUD();

    if (this->currentState == MENU) {
        GraphicsEngine::drawFxTextCenter(winW / 2, GameConfig::S(40), Texts::MENU_TITLE, GameConfig::S(22), 1.0f, this->titleDrift.x, "ui");

        this->btnEasy.draw();
        this->btnNormal.draw();
        this->btnHell.draw();

        GraphicsEngine::drawFxTextCenter(winW / 2, winH - GameConfig::S(16), Texts::DEVELOPERS, GameConfig::S(10), 0.45f, this->microDrift.x * 0.65f, "ui");

        this->drawMusicIndicator();
        return;
    }

    if (this->currentState == PLAYING) {
        string scoreText = "SCORE: " + to_string(this->score);
        GraphicsEngine::drawFxTextCenter(leftPad + GameConfig::S(56), topPad + GameConfig::S(9), scoreText.c_str(), GameConfig::S(16), 0.70f, this->hudDrift.x, "Consolas");

        this->btnPause.draw();
        this->btnMenu.draw();

        this->drawMusicIndicator();
        return;
    }

    if (this->currentState == PAUSED) {
        string scoreText = "SCORE: " + to_string(this->score);
        GraphicsEngine::drawFxTextCenter(leftPad + GameConfig::S(56), topPad + GameConfig::S(9), scoreText.c_str(), GameConfig::S(16), 0.65f, this->hudDrift.x * 0.95f, "Consolas");

        int boxX = winW / 2 - GameConfig::S(84);
        int boxY = winH / 2 - GameConfig::S(52);
        int boxW = GameConfig::S(168);
        int boxH = GameConfig::S(106);

        Rectangle panel = {(float)boxX, (float)boxY, (float)boxW, (float)boxH};
        DrawRectangleRounded(panel, 0.08f, 8, {26, 28, 40, 225});
        DrawRectangleRoundedLinesEx(panel, 0.08f, 8, (float)std::max(2, GameConfig::S(1)), {220, 230, 255, 220});

        GraphicsEngine::drawFxTextCenter(winW / 2, winH / 2 - GameConfig::S(24), Texts::PAUSED_TITLE, GameConfig::S(20), 0.95f, this->titleDrift.x * 0.72f, "ui");
        GraphicsEngine::drawFxTextCenter(winW / 2, winH / 2 - GameConfig::S(4), Texts::PAUSED_HINT, GameConfig::S(12), 0.55f, this->microDrift.x * 0.75f, "ui");

        this->btnResume.draw();
        this->btnPause.draw();
        this->btnMenu.draw();

        this->drawMusicIndicator();
        return;
    }

    if (this->currentState == END) {
        int boxX = winW / 2 - GameConfig::S(114);
        int boxY = winH / 2 - GameConfig::S(74);
        int boxW = GameConfig::S(228);
        int boxH = GameConfig::S(148);

        Rectangle panel = {(float)boxX, (float)boxY, (float)boxW, (float)boxH};
        DrawRectangleRounded(panel, 0.06f, 8, {12, 12, 18, 232});
        DrawRectangleRoundedLinesEx(panel, 0.06f, 8, (float)std::max(2, GameConfig::S(1)), {220, 230, 255, 210});

        GraphicsEngine::drawFxTextCenter(winW / 2, winH / 2 - GameConfig::S(46), "GAME OVER", GameConfig::S(28), 1.0f, this->titleDrift.x, "ui");

        string scoreMsg = "Final Score: " + to_string(this->animatedEndScore);
        GraphicsEngine::drawFxTextCenter(winW / 2, winH / 2 + GameConfig::S(10), scoreMsg.c_str(), GameConfig::S(16), 0.76f, this->hudDrift.x, "ui");
        GraphicsEngine::drawFxTextCenter(winW / 2, winH / 2 + GameConfig::S(42), Texts::END_HINT, GameConfig::S(12), 0.58f, this->microDrift.x * 0.70f, "ui");

        this->drawMusicIndicator();
    }
}

void GameManager::renderGameScene() {
    this->drawWorldWithCamera();
}

void GameManager::updateMenu() {
    this->drawBackground();

    Vector2 mousePos = GetMousePosition();
    bool mousePressed = IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
    bool mouseDown = IsMouseButtonDown(MOUSE_BUTTON_LEFT);

    bool easyClicked = this->btnEasy.update(mousePos.x, mousePos.y, mousePressed, mouseDown);
    bool normalClicked = this->btnNormal.update(mousePos.x, mousePos.y, mousePressed, mouseDown);
    bool hellClicked = this->btnHell.update(mousePos.x, mousePos.y, mousePressed, mouseDown);

    if (easyClicked) {
        this->setDifficulty(60, 1);
        this->resetGame();
        this->currentState = PLAYING;
        return;
    }
    if (normalClicked) {
        this->setDifficulty(30, 2);
        this->resetGame();
        this->currentState = PLAYING;
        return;
    }
    if (hellClicked) {
        this->setDifficulty(10, 6);
        this->resetGame();
        this->currentState = PLAYING;
        return;
    }

    this->drawScreenFX();
    this->drawUnifiedUI();
}

void GameManager::updatePlaying() {
    this->drawBackground();

    Vector2 mousePos = GetMousePosition();
    bool mousePressed = IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
    bool mouseDown = IsMouseButtonDown(MOUSE_BUTTON_LEFT);

    bool pauseClicked = this->btnPause.update(mousePos.x, mousePos.y, mousePressed, mouseDown);
    bool menuClicked = this->btnMenu.update(mousePos.x, mousePos.y, mousePressed, mouseDown);

    if (pauseClicked && this->pauseCooldown <= 0.0f) {
        this->currentState = PAUSED;
        this->pauseCooldown = 0.20f;
        return;
    }
    if (menuClicked) {
        this->currentState = MENU;
        this->resetGame();
        return;
    }

    this->particleSystem.beginFrame();

    float worldDt = this->deltaTime;
    if (this->hitStopTimer > 0.0f) {
        this->hitStopTimer = std::max(0.0f, this->hitStopTimer - this->deltaTime);
        worldDt = 0.0f;
    }

    this->processInput(worldDt);
    if (this->currentState != PLAYING) {
        return;
    }

    this->updateGameLogic(worldDt);
    if (this->currentState != PLAYING) {
        return;
    }

    this->renderGameScene();
    this->drawScreenFX();
    this->drawUnifiedUI();
}

void GameManager::updatePaused() {
    this->drawBackground();

    Vector2 mousePos = GetMousePosition();
    bool mousePressed = IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
    bool mouseDown = IsMouseButtonDown(MOUSE_BUTTON_LEFT);

    bool resumeClicked = this->btnResume.update(mousePos.x, mousePos.y, mousePressed, mouseDown);
    this->btnPause.update(mousePos.x, mousePos.y, false, mouseDown);
    bool menuClicked = this->btnMenu.update(mousePos.x, mousePos.y, mousePressed, mouseDown);

    if (resumeClicked && this->pauseCooldown <= 0.0f) {
        this->currentState = PLAYING;
        this->pauseCooldown = 0.20f;
        return;
    }

    if (menuClicked) {
        this->currentState = MENU;
        this->resetGame();
        return;
    }

    if (IsKeyPressed(KEY_P) && this->pauseCooldown <= 0.0f) {
        this->currentState = PLAYING;
        this->pauseCooldown = 0.20f;
        return;
    }

    this->updatePerspectiveWorld(0.0f);
    this->renderGameScene();
    this->drawScreenFX();
    this->drawUnifiedUI();
}

void GameManager::updateGameOver() {
    this->drawBackground();

    this->updatePerspectiveWorld(0.0f);
    this->endScoreAnimTimer = std::min(0.35f, this->endScoreAnimTimer + this->deltaTime);
    float t = this->endScoreAnimTimer / 0.35f;
    this->animatedEndScore = (int)std::round(this->score * EaseOutCubic(t));

    this->renderGameScene();
    this->drawScreenFX();
    this->drawUnifiedUI();

    if (IsKeyPressed(KEY_SPACE)) {
        this->resetGame();
        this->currentState = PLAYING;
        return;
    }

    if (IsKeyPressed(KEY_ESCAPE)) {
        this->resetGame();
        this->currentState = MENU;
        return;
    }
}

void GameManager::run() {
    while (!WindowShouldClose()) {
        this->deltaTime = ClampFloat(GetFrameTime(), 0.001f, 0.05f);
        this->uiTime += this->deltaTime;

        GraphicsEngine::setFXTime(this->uiTime);
        this->updateUiDrift(this->deltaTime);

        if (IsKeyPressed(KEY_M)) {
            this->chipMusic.toggleMute();
        }
        this->chipMusic.update(this->deltaTime);

        if (this->pauseCooldown > 0.0f) {
            this->pauseCooldown = std::max(0.0f, this->pauseCooldown - this->deltaTime);
        }

        float paraDt = this->deltaTime;
        if (this->currentState == PAUSED) {
            paraDt *= 0.10f;
        }

        this->updateParallax(paraDt);
        this->updateCameraFX(this->deltaTime);
        this->layoutHUD();

        BeginDrawing();
        ClearBackground(BLACK);

        switch (this->currentState) {
            case MENU:
                this->updateMenu();
                break;
            case PLAYING:
                this->updatePlaying();
                break;
            case PAUSED:
                this->updatePaused();
                break;
            case END:
                this->updateGameOver();
                break;
            default:
                break;
        }

        EndDrawing();
    }
}

int main() {
    {
        GameManager game;
        game.run();
    }
    return 0;
}
