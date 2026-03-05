/*
 * =====================================================================================
 *
 * 文件名:  PlaneFight_Enterprise_Edition.cpp
 *
 * 描述:  太空射击游戏（真 2.5D 走廊版）
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

/* ==================== 界面文本常量 ==================== */
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

/* ==================== 游戏全局配置 ==================== */
// 控制窗口大小、缩放比例和主题颜色
class GameConfig {
public:
    static const double SCALE;        // 全局缩放因子
    static const int BASE_WIDTH;      // 基准宽度（像素）
    static const int BASE_HEIGHT;     // 基准高度（像素）

    static const Color COLOR_BG;      // 背景色
    static const Color COLOR_GRID;    // 网格色
    static const Color COLOR_PLAYER;  // 玩家颜色
    static const Color COLOR_ENEMY;   // 敌人颜色
    static const Color COLOR_BULLET;  // 子弹颜色
    static const Color COLOR_TEXT;    // 文字颜色

    // 将值按全局缩放比例取整
    static inline int S(double v) { return static_cast<int>(v * SCALE); }
    static inline int GetWindowWidth() { return S(BASE_WIDTH); }
    static inline int GetWindowHeight() { return S(BASE_HEIGHT); }
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

/* ==================== 轻量数据结构（使用默认成员初始化） ==================== */

// 玩家运动状态：速度、加速度、倾斜角、后坐力
struct PlayerMotionState {
    Vector2 velocity = {0, 0};
    Vector2 accel = {0, 0};
    float tiltDeg = 0;   // 飞船左右倾斜角度
    float recoil = 0;    // 开火后坐力偏移
};

// 摄像机特效状态：屏幕震动、旋转、缩放
struct CameraFXState {
    float trauma = 0;   // 震动强度（0~1）
    float shakeX = 0;   // 水平震动偏移
    float shakeY = 0;   // 垂直震动偏移
    float rollDeg = 0;  // 画面旋转角度
    float zoom = 1;     // 画面缩放
};

// 透视走廊参数：地平线、底部、远近半宽、深度范围
struct PerspectiveConfig {
    float horizonY = 0;       // 地平线 Y 坐标
    float bottomY = 0;        // 底部 Y 坐标
    float laneHalfFar = 0;    // 远处车道半宽
    float laneHalfNear = 0;   // 近处车道半宽
    float minDepthZ = 0;      // 最小深度
    float maxDepthZ = 1;      // 最大深度
};

// 透视映射后的屏幕姿态
struct PerspectivePose {
    float laneX = 0;                // 车道 X（-1~1）
    float depthZ = 0;               // 深度 Z（0~1）
    Vector2 screenPos = {0, 0};     // 屏幕坐标
    float screenScale = 1;          // 缩放系数
    float screenRadius = 1;         // 碰撞半径（屏幕像素）
};

// 粒子：用于爆炸、枪口火焰等特效
struct Particle {
    bool active = false;
    Vector2 position = {0, 0};
    Vector2 velocity = {0, 0};
    float life = 0;
    float maxLife = 0;
    float size = 1;
    float rotation = 0;
    float spin = 0;
    Color startColor = {255, 255, 255, 255};
    Color endColor = {255, 255, 255, 0};
    int priority = 0;  // 优先级：高优先级粒子不会被低优先级覆盖
};

// 视差背景层参数
struct ParallaxLayer {
    float speed = 0;
    float offsetY = 0;
    float alpha = 0;
    int density = 0;
    float drift = 0;
    Color color = {255, 255, 255, 255};
};

// 飞船立体渲染风格参数
struct ShipVolumeStyle {
    int thicknessLayers = 3;           // 厚度层数
    float maxThicknessPx = 5.5f;       // 最大厚度像素
    float shadowBoost = 1.0f;          // 阴影增强
    unsigned char highlightAlpha = 64; // 高光不透明度
    unsigned char rimAlpha = 52;       // 边缘光不透明度
};

// UI 文字漂移动画状态（弹簧运动）
struct UiDriftState {
    float x = 0;               // 当前偏移
    float v = 0;               // 速度
    float target = 0;          // 目标偏移
    float retargetTimer = 0;   // 重新选目标倒计时
    float amp = 1;             // 振幅
};

/* ==================== 数学工具函数 ==================== */

static const float kPi = 3.14159265358979323846f;
static const float kTau = 6.28318530717958647692f;

// 限制值在 [minVal, maxVal] 范围内
static float ClampFloat(float value, float minVal, float maxVal) {
    if (value < minVal) return minVal;
    if (value > maxVal) return maxVal;
    return value;
}

// 带截断的线性插值
static float LerpFloat(float a, float b, float t) {
    return a + (b - a) * ClampFloat(t, 0, 1);
}

// 平滑阶梯函数 3t^2 - 2t^3
static float SmoothStep(float t) {
    t = ClampFloat(t, 0, 1);
    return t * t * (3 - 2 * t);
}

// 缓出三次方
static float EaseOutCubic(float t) {
    t = ClampFloat(t, 0, 1);
    float inv = 1 - t;
    return 1 - inv * inv * inv;
}

// 将值包裹在 [0, range) 区间内
static float WrapFloat(float value, float range) {
    if (range <= 0) return 0;
    while (value < 0) value += range;
    while (value >= range) value -= range;
    return value;
}

// 生成 [0, 1] 随机浮点数
static float Random01() { return (float)rand() / (float)RAND_MAX; }

// 生成 [minVal, maxVal] 随机浮点数
static float RandomRange(float minVal, float maxVal) {
    return minVal + (maxVal - minVal) * Random01();
}

// 颜色线性插值
static Color LerpColor(const Color& a, const Color& b, float t) {
    t = ClampFloat(t, 0, 1);
    return {
        (unsigned char)(a.r + (b.r - a.r) * t),
        (unsigned char)(a.g + (b.g - a.g) * t),
        (unsigned char)(a.b + (b.b - a.b) * t),
        (unsigned char)(a.a + (b.a - a.a) * t)
    };
}

// 两点距离平方（避免开方，用于碰撞检测）
static float DistSq(Vector2 a, Vector2 b) {
    float dx = a.x - b.x, dy = a.y - b.y;
    return dx * dx + dy * dy;
}

// 矩形碰撞检测
static bool IsColliding(int x1, int y1, int w1, int h1, int x2, int y2, int w2, int h2) {
    return x1 < x2 + w2 && x1 + w1 > x2 && y1 < y2 + h2 && y1 + h1 > y2;
}

// 点是否在矩形内
static bool IsPointInRect(float px, float py, int rx, int ry, int rw, int rh) {
    return px >= rx && px <= rx + rw && py >= ry && py <= ry + rh;
}

// 将深度值归一化为 [0,1] 的 t 参数，再做 SmoothStep 曲线
static float DepthToSmooth(float depthZ, const PerspectiveConfig& cfg) {
    float t = 0;
    if (cfg.maxDepthZ > cfg.minDepthZ)
        t = (depthZ - cfg.minDepthZ) / (cfg.maxDepthZ - cfg.minDepthZ);
    return SmoothStep(t);
}

/* ==================== 透视映射器 ==================== */
// 负责将逻辑坐标（车道X + 深度Z）映射到屏幕坐标
class PerspectiveMapper {
    PerspectiveConfig cfg;
public:
    PerspectiveMapper() {}
    explicit PerspectiveMapper(const PerspectiveConfig& c) : cfg(c) {}

    void setConfig(const PerspectiveConfig& c) { cfg = c; }
    const PerspectiveConfig& getConfig() const { return cfg; }

    // 指定深度处的车道半宽（像素）
    float laneHalfWidth(float depthZ) const {
        return LerpFloat(cfg.laneHalfFar, cfg.laneHalfNear, DepthToSmooth(depthZ, cfg));
    }

    // 深度 -> 屏幕 Y 坐标
    float depthToScreenY(float depthZ) const {
        return LerpFloat(cfg.horizonY, cfg.bottomY, DepthToSmooth(depthZ, cfg));
    }

    // 深度 -> 缩放系数
    float depthToScale(float depthZ) const {
        return LerpFloat(0.28f, 1.22f, DepthToSmooth(depthZ, cfg));
    }

    // 逻辑坐标 -> 屏幕坐标
    Vector2 projectToScreen(float laneX, float depthZ) const {
        float halfW = laneHalfWidth(depthZ);
        return {GameConfig::GetWindowWidth() * 0.5f + laneX * halfW, depthToScreenY(depthZ)};
    }
};

/* ==================== 图形引擎（静态工具类） ==================== */
// 封装文字渲染、特效文字、立体精灵绘制
class GraphicsEngine {
    static const Font* uiFont;
    static bool hasUIFont;
    static float fxTime;   // 全局特效时间，用于扫描线动画等

public:
    static void setUIFont(const Font* font, bool available) { uiFont = font; hasUIFont = available; }
    static void setFXTime(float t) { fxTime = t; }

    // 绘制纹理（原始大小）
    static void drawImageAlpha(int x, int y, const Texture2D* tex) {
        if (!tex || tex->id == 0) return;
        DrawTextureV(*tex, {(float)x, (float)y}, WHITE);
    }

    // 居中绘制普通文字
    static void outTextCenter(int x, int y, const char* str, int fontSize = 20, const char* fontName = "") {
        (void)fontName;
        if (!str || !str[0]) return;
        if (hasUIFont && uiFont && uiFont->texture.id != 0) {
            Vector2 size = MeasureTextEx(*uiFont, str, (float)fontSize, 1);
            DrawTextEx(*uiFont, str, {x - size.x * 0.5f, y - size.y * 0.5f}, (float)fontSize, 1, GameConfig::COLOR_TEXT);
        } else {
            int w = MeasureText(str, fontSize);
            DrawText(str, x - w / 2, y - fontSize / 2, fontSize, GameConfig::COLOR_TEXT);
        }
    }

    // 居中绘制霓虹特效文字（带描边、扫描线、轻微旋转）
    static void drawFxTextCenter(int x, int y, const char* str, int fontSize, float intensity = 1.0f, float driftX = 0, const char* fontName = "") {
        (void)fontName;
        if (!str || !str[0]) return;
        intensity = ClampFloat(intensity, 0, 1);
        float cx = x + driftX;

        // 计算摇摆旋转角度
        float rotSwing = driftX * 1.85f;
        if (intensity >= 0.62f)
            rotSwing += std::sin(fxTime * 2.15f + x * 0.008f) * (0.75f + intensity * 0.9f);
        float rotDeg = ClampFloat(rotSwing, -6, 6);

        // 在指定位置用指定颜色绘制文字（带旋转）
        auto drawAt = [&](float px, float py, Color c) {
            if (hasUIFont && uiFont && uiFont->texture.id != 0) {
                Vector2 size = MeasureTextEx(*uiFont, str, (float)fontSize, 1);
                DrawTextPro(*uiFont, str, {px, py}, {size.x * 0.5f, size.y * 0.5f}, rotDeg, (float)fontSize, 1, c);
            } else {
                int w = MeasureText(str, fontSize);
                DrawText(str, (int)(px - w * 0.5f), (int)(py - fontSize * 0.5f), fontSize, c);
            }
        };

        // 霓虹光晕描边（6个方向）
        const Color glow = {90, 220, 255, (unsigned char)(150 * intensity)};
        drawAt(cx - 2, y, glow);
        drawAt(cx + 2, y, glow);
        drawAt(cx, y - 2, glow);
        drawAt(cx, y + 2, glow);
        drawAt(cx - 1.5f, y - 1.5f, {70, 200, 255, (unsigned char)(95 * intensity)});
        drawAt(cx + 1.5f, y + 1.5f, {70, 200, 255, (unsigned char)(95 * intensity)});

        // 主体白色文字
        drawAt(cx, y, {230, 250, 255, 255});

        // 扫描线效果（高强度时显示）
        if (intensity > 0.45f) {
            float textW = 0, textH = (float)fontSize;
            if (hasUIFont && uiFont && uiFont->texture.id != 0) {
                Vector2 size = MeasureTextEx(*uiFont, str, (float)fontSize, 1);
                textW = size.x; textH = size.y;
            } else {
                textW = (float)MeasureText(str, fontSize);
            }
            float scanW = std::max(10.0f, fontSize * 0.42f);
            float scanX = cx - textW * 0.5f + WrapFloat(fxTime * 198, textW + scanW * 2) - scanW;
            BeginBlendMode(BLEND_ADDITIVE);
            DrawRectangle((int)scanX, (int)(y - textH * 0.55f), (int)scanW, (int)(textH * 1.15f),
                          {130, 240, 255, (unsigned char)(55 * intensity)});
            EndBlendMode();
        }
    }

    // 绘制带立体感的精灵（多层阴影 + 高光 + 边缘光）
    static void drawVolumetricSprite(Texture2D tex, Rectangle src, Rectangle dst, Vector2 origin,
                                     float rotationDeg, ShipVolumeStyle style, Color tint) {
        if (tex.id == 0 || src.width == 0 || src.height == 0) return;

        float baseScale = std::fabs(dst.width / src.width);
        float t = ClampFloat((baseScale - 0.28f) / (1.22f - 0.28f), 0, 1);
        float thicknessPx = LerpFloat(1, style.maxThicknessPx, t);
        int layers = std::max(1, style.thicknessLayers);
        float shadowBoost = ClampFloat(style.shadowBoost, 0.6f, 1.8f);

        // 从后往前绘制阴影层
        for (int i = layers; i >= 1; --i) {
            float lf = (float)i / layers;
            Rectangle depthDst = {dst.x + thicknessPx * lf, dst.y + thicknessPx * lf, dst.width, dst.height};
            Color depthTint = {
                (unsigned char)ClampFloat(tint.r * (0.40f / shadowBoost), 0, 255),
                (unsigned char)ClampFloat(tint.g * (0.40f / shadowBoost), 0, 255),
                (unsigned char)ClampFloat(tint.b * (0.44f / shadowBoost), 0, 255),
                (unsigned char)ClampFloat((52 + 44 * lf) * shadowBoost, 0, 255)
            };
            DrawTexturePro(tex, src, depthDst, origin, rotationDeg, depthTint);
        }

        // 主体层
        DrawTexturePro(tex, src, dst, origin, rotationDeg, tint);

        // 高光和边缘光线
        float rad = rotationDeg * (kPi / 180);
        float cr = std::cos(rad), sr = std::sin(rad);
        auto rotPoint = [&](float lx, float ly) -> Vector2 {
            return {dst.x + lx * cr - ly * sr, dst.y + lx * sr + ly * cr};
        };
        float halfW = dst.width * 0.5f, halfH = dst.height * 0.5f;

        BeginBlendMode(BLEND_ADDITIVE);
        // 顶部高光线
        DrawLineEx(rotPoint(-halfW * 0.34f, -halfH * 0.34f), rotPoint(halfW * 0.34f, -halfH * 0.34f),
                   std::max(1.0f, dst.width * 0.08f), {255, 255, 255, style.highlightAlpha});
        // 左右边缘光
        DrawLineEx(rotPoint(-halfW * 0.30f, -halfH * 0.24f), rotPoint(-halfW * 0.38f, halfH * 0.30f),
                   std::max(1.0f, dst.width * 0.05f), {120, 220, 255, style.rimAlpha});
        DrawLineEx(rotPoint(halfW * 0.30f, -halfH * 0.24f), rotPoint(halfW * 0.38f, halfH * 0.30f),
                   std::max(1.0f, dst.width * 0.05f), {120, 220, 255, style.rimAlpha});
        // 机头亮点
        Vector2 nose = rotPoint(0, -halfH * 0.45f);
        DrawCircleV(nose, std::max(1.0f, dst.width * 0.06f), {255, 255, 255, (unsigned char)(style.highlightAlpha * 0.78f)});
        EndBlendMode();
    }
};

const Font* GraphicsEngine::uiFont = nullptr;
bool GraphicsEngine::hasUIFont = false;
float GraphicsEngine::fxTime = 0;

/* ==================== 资源管理器 ==================== */
// 加载和管理所有图片纹理和 UI 字体
class ResourceManager {
    Texture2D imgPlayer = {};
    Texture2D imgEnemy = {};
    Texture2D imgBulletPlayer = {};
    Texture2D imgBulletEnemy = {};
    Texture2D imgBackground = {};
    Font uiFont = {};

    bool hasImgPlayer = false;
    bool hasImgEnemy = false;
    bool hasImgBulletP = false;
    bool hasImgBulletE = false;
    bool hasImgBackground = false;
    bool hasUIFont = false;

    // 加载图片并缩放到指定大小
    bool loadTextureScaled(Texture2D& target, const char* path, int w, int h, bool& flag) {
        flag = false;
        Image img = {0};
        if (FileExists(path)) img = LoadImage(path);
#if defined(_WIN32)
        if (!img.data) {
            const unsigned char* rawData = nullptr;
            int rawSize = 0;
            if (GetEmbeddedAssetData(path, rawData, rawSize))
                img = LoadImageFromMemory(".png", rawData, rawSize);
        }
#endif
        if (!img.data) return false;
        ImageResizeNN(&img, w, h);
        target = LoadTextureFromImage(img);
        UnloadImage(img);
        flag = (target.id != 0);
        return flag;
    }

    // 尝试从系统目录加载中文字体
    bool loadUIFontFromSystem() {
        static const char* fontPaths[] = {
            "C:/Windows/Fonts/simhei.ttf",
            "C:/Windows/Fonts/msyh.ttc",
            "C:/Windows/Fonts/msyhbd.ttc",
            "C:/Windows/Fonts/simsun.ttc"
        };
        // 收集所有需要渲染的字符码点
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
        if (!rawCodepoints || rawCount <= 0) return false;

        // 去重码点
        vector<int> uniqueCp;
        uniqueCp.reserve(rawCount);
        unordered_set<int> seen;
        for (int i = 0; i < rawCount; ++i)
            if (seen.insert(rawCodepoints[i]).second)
                uniqueCp.push_back(rawCodepoints[i]);
        UnloadCodepoints(rawCodepoints);
        if (uniqueCp.empty()) return false;

        const int fontBaseSize = GameConfig::S(13);
        for (const char* path : fontPaths) {
            if (!FileExists(path)) continue;
            Font candidate = LoadFontEx(path, fontBaseSize, uniqueCp.data(), (int)uniqueCp.size());
            if (candidate.texture.id != 0) {
                uiFont = candidate;
                TraceLog(LOG_INFO, "Loaded UI font from: %s", path);
                return true;
            }
        }
        TraceLog(LOG_WARNING, "No system CJK font loaded. Falling back to default font.");
        return false;
    }

public:
    ResourceManager() {}

    ~ResourceManager() {
        if (imgPlayer.id) UnloadTexture(imgPlayer);
        if (imgEnemy.id) UnloadTexture(imgEnemy);
        if (imgBulletPlayer.id) UnloadTexture(imgBulletPlayer);
        if (imgBulletEnemy.id) UnloadTexture(imgBulletEnemy);
        if (imgBackground.id) UnloadTexture(imgBackground);
        if (hasUIFont && uiFont.texture.id) UnloadFont(uiFont);
        GraphicsEngine::setUIFont(nullptr, false);
    }

    // 加载全部资源
    void loadAllResources() {
        loadTextureScaled(imgBackground, "purple.png", GameConfig::GetWindowWidth(), GameConfig::GetWindowHeight(), hasImgBackground);
        loadTextureScaled(imgPlayer, "playerShip_blue.png", GameConfig::S(32), GameConfig::S(32), hasImgPlayer);
        loadTextureScaled(imgEnemy, "playerShip_red.png", GameConfig::S(40), GameConfig::S(40), hasImgEnemy);
        loadTextureScaled(imgBulletPlayer, "laserBlue.png", GameConfig::S(8), GameConfig::S(24), hasImgBulletP);
        loadTextureScaled(imgBulletEnemy, "laserRed.png", GameConfig::S(8), GameConfig::S(24), hasImgBulletE);
        hasUIFont = loadUIFontFromSystem();
        GraphicsEngine::setUIFont(hasUIFont ? &uiFont : nullptr, hasUIFont);
    }

    // 纹理获取接口
    const Texture2D* getPlayerImage() { return &imgPlayer; }
    const Texture2D* getEnemyImage() { return &imgEnemy; }
    const Texture2D* getBulletPlayerImage() { return &imgBulletPlayer; }
    const Texture2D* getBulletEnemyImage() { return &imgBulletEnemy; }
    const Texture2D* getBackgroundImage() { return &imgBackground; }

    bool isPlayerImageValid() const { return hasImgPlayer; }
    bool isEnemyImageValid() const { return hasImgEnemy; }
    bool isBulletPlayerImageValid() const { return hasImgBulletP; }
    bool isBulletEnemyImageValid() const { return hasImgBulletE; }
    bool isBackgroundImageValid() const { return hasImgBackground; }
    bool isUIFontValid() const { return hasUIFont; }
};

/* ==================== 游戏对象基类 ==================== */
// 所有可显示对象的基类：管理位置、大小、存活状态、透视映射
class GameObject {
protected:
    Vector2 position = {0, 0};  // 屏幕坐标位置
    int width, height;
    bool isAlive = true;

    float laneX = 0;            // 车道横坐标（-1~1）
    float depthZ = 0;           // 纵深坐标（0=远处, 1=近处）
    float baseRadius;           // 基础碰撞半径
    PerspectivePose pose;

public:
    GameObject(int _w, int _h)
        : width(_w), height(_h), baseRadius(std::min(_w, _h) * 0.35f) {}
    virtual ~GameObject() {}

    virtual void draw() = 0;
    virtual bool move(float dt) = 0;

    // 根据当前车道和深度，计算屏幕位置和缩放
    virtual void updatePerspective(const PerspectiveConfig& cfg) {
        float curve = DepthToSmooth(depthZ, cfg);
        float halfWidth = LerpFloat(cfg.laneHalfFar, cfg.laneHalfNear, curve);
        float centerX = GameConfig::GetWindowWidth() * 0.5f;

        pose.laneX = laneX;
        pose.depthZ = depthZ;
        pose.screenPos = {centerX + laneX * halfWidth, LerpFloat(cfg.horizonY, cfg.bottomY, curve)};
        pose.screenScale = LerpFloat(0.28f, 1.22f, curve);
        pose.screenRadius = std::max(1.0f, baseRadius * pose.screenScale);

        position = {pose.screenPos.x - width * pose.screenScale * 0.5f,
                    pose.screenPos.y - height * pose.screenScale * 0.5f};
    }

    // 常用属性访问
    float getX() const { return position.x; }
    float getY() const { return position.y; }
    int getWidth() const { return width; }
    int getHeight() const { return height; }
    bool getIsAlive() const { return isAlive; }
    void setIsAlive(bool s) { isAlive = s; }
    void setPosition(float _x, float _y) { position = {_x, _y}; }

    float getLaneX() const { return laneX; }
    float getDepthZ() const { return depthZ; }
    void setLaneX(float v) { laneX = v; }
    void setDepthZ(float v) { depthZ = v; }
    void setBaseRadius(float v) { baseRadius = v; }

    const PerspectivePose& getPose() const { return pose; }
    float getScreenScale() const { return pose.screenScale; }
};

/* ==================== 子弹类（玩家和敌人共用） ==================== */
// direction < 0 表示向远处飞（玩家子弹），> 0 表示向近处飞（敌人子弹）
class Bullet : public GameObject {
    ResourceManager* resMgr;
    float speed;
    int direction;  // -1=玩家子弹（向远处）, +1=敌人子弹（向近处）

public:
    Bullet(float _laneX, float _depthZ, float _speed, int _dir, ResourceManager* _rm)
        : GameObject(GameConfig::S(8), GameConfig::S(24)), resMgr(_rm), speed(_speed), direction(_dir) {
        laneX = _laneX;
        depthZ = _depthZ;
        baseRadius = (float)GameConfig::S(3);
    }

    bool move(float dt) override {
        depthZ += direction * speed * dt;
        // 玩家子弹超出远端 / 敌人子弹超出近端时销毁
        return direction < 0 ? (depthZ > -0.03f) : (depthZ < 1.02f);
    }

    bool isPlayerBullet() const { return direction < 0; }

    void draw() override {
        float scale = pose.screenScale;
        float bodyW = std::max(1.0f, width * scale * 0.55f);
        float bodyH = std::max(2.0f, height * scale * 0.85f);
        float x = pose.screenPos.x, y = pose.screenPos.y;

        // 绘制拖尾效果
        float tailLen = std::max(3.0f, bodyH * 0.90f);
        float tailDir = (direction < 0) ? 1.0f : -1.0f;  // 玩家子弹尾巴朝下，敌人朝上
        Color tailColor = (direction < 0) ? Color{90, 180, 255, 0} : Color{255, 110, 110, 0};
        unsigned char tailBaseA = (direction < 0) ? 120 : 115;
        for (int i = 0; i < 4; ++i) {
            float t0 = i / 4.0f, t1 = (i + 1) / 4.0f;
            unsigned char a = (unsigned char)(tailBaseA * (1 - t0));
            float thick = std::max(1.0f, bodyW * (0.75f - t0 * 0.35f));
            Color c = {tailColor.r, tailColor.g, tailColor.b, a};
            DrawLineEx({x, y + tailDir * t0 * tailLen}, {x, y + tailDir * t1 * tailLen}, thick, c);
        }

        // 光晕
        Color glowColor = (direction < 0) ? Color{80, 180, 255, 85} : Color{255, 120, 120, 75};
        DrawCircleV({x, y}, std::max(1.0f, bodyW * 0.90f), glowColor);

        // 精灵纹理 / 回退矩形
        bool hasImg = (direction < 0) ? resMgr->isBulletPlayerImageValid() : resMgr->isBulletEnemyImageValid();
        const Texture2D* tex = (direction < 0) ? resMgr->getBulletPlayerImage() : resMgr->getBulletEnemyImage();
        if (hasImg) {
            Rectangle src = {0, 0, (float)tex->width, (float)tex->height};
            Rectangle dst = {x, y, bodyW, bodyH};
            DrawTexturePro(*tex, src, dst, {bodyW * 0.5f, bodyH * 0.5f}, 0, WHITE);
        } else {
            Color fallback = (direction < 0) ? GameConfig::COLOR_BULLET : Color{255, 100, 100, 255};
            DrawRectangle((int)(x - bodyW * 0.5f), (int)(y - bodyH * 0.5f), (int)bodyW, (int)bodyH, fallback);
        }
    }
};

/* ==================== 敌机类 ==================== */
class Enemy : public GameObject {
    ResourceManager* resMgr;
    float advanceSpeed;  // 前进速度

public:
    Enemy(float _laneX, float _depthZ, float _speed, ResourceManager* _rm)
        : GameObject(GameConfig::S(40), GameConfig::S(40)), resMgr(_rm), advanceSpeed(_speed) {
        laneX = _laneX;
        depthZ = _depthZ;
        baseRadius = (float)GameConfig::S(13);
    }

    // 敌人从远处向玩家推进
    bool move(float dt) override {
        depthZ += advanceSpeed * dt;
        return depthZ < 1.01f;
    }

    void draw() override {
        float w = width * pose.screenScale, h = height * pose.screenScale;
        float x = pose.screenPos.x, y = pose.screenPos.y;

        if (resMgr->isEnemyImageValid()) {
            Texture2D tex = *resMgr->getEnemyImage();
            Rectangle src = {0, 0, (float)tex.width, (float)tex.height};
            Rectangle dst = {x, y, w, h};
            ShipVolumeStyle style;
            style.thicknessLayers = 4;
            style.shadowBoost = 1.10f;
            style.highlightAlpha = 68;
            // 倒转 180 度（敌机朝下）
            GraphicsEngine::drawVolumetricSprite(tex, src, dst, {w * 0.5f, h * 0.5f}, 180, style, WHITE);
        } else {
            // 无纹理回退：绘制三角形 + 高光线
            DrawTriangle({x - w * 0.5f, y - h * 0.45f}, {x + w * 0.5f, y - h * 0.45f}, {x, y + h * 0.50f}, GameConfig::COLOR_ENEMY);
            BeginBlendMode(BLEND_ADDITIVE);
            DrawLineEx({x - w * 0.20f, y - h * 0.28f}, {x + w * 0.20f, y - h * 0.28f}, std::max(1.0f, w * 0.07f), {255, 230, 190, 70});
            DrawLineEx({x - w * 0.22f, y - h * 0.20f}, {x - w * 0.28f, y + h * 0.24f}, std::max(1.0f, w * 0.04f), {120, 220, 255, 54});
            DrawLineEx({x + w * 0.22f, y - h * 0.20f}, {x + w * 0.28f, y + h * 0.24f}, std::max(1.0f, w * 0.04f), {120, 220, 255, 54});
            EndBlendMode();
        }
    }
};

/* ==================== 玩家飞机类 ==================== */
class Player : public GameObject {
    ResourceManager* resMgr;
    PlayerMotionState motionState;

public:
    Player(ResourceManager* _rm)
        : GameObject(GameConfig::S(32), GameConfig::S(32)), resMgr(_rm) {
        laneX = 0;
        depthZ = 0.86f;
        baseRadius = (float)GameConfig::S(12);
    }

    // 处理键盘输入，更新物理运动
    bool move(float dt) override {
        if (dt <= 0) return true;

        // 读取输入方向
        float inputX = 0, inputZ = 0;
        if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT))  inputX -= 1;
        if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT)) inputX += 1;
        if (IsKeyDown(KEY_W) || IsKeyDown(KEY_UP))    inputZ -= 1;
        if (IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN))  inputZ += 1;

        // 运动参数
        const float accelLane = 4.8f, accelDepth = 2.0f;
        const float dampLane = 8.0f, dampDepth = 8.0f;
        const float maxLaneV = 1.65f, maxDepthV = 0.70f;

        // 加速 + 阻尼
        motionState.accel = {inputX * accelLane, inputZ * accelDepth};
        motionState.velocity.x += motionState.accel.x * dt;
        motionState.velocity.y += motionState.accel.y * dt;
        if (inputX == 0) motionState.velocity.x *= std::exp(-dampLane * dt);
        if (inputZ == 0) motionState.velocity.y *= std::exp(-dampDepth * dt);
        motionState.velocity.x = ClampFloat(motionState.velocity.x, -maxLaneV, maxLaneV);
        motionState.velocity.y = ClampFloat(motionState.velocity.y, -maxDepthV, maxDepthV);

        // 更新位置
        laneX += motionState.velocity.x * dt;
        depthZ += motionState.velocity.y * dt;
        laneX = ClampFloat(laneX, -1, 1);
        depthZ = ClampFloat(depthZ, 0.76f, 0.95f);

        // 倾斜角跟随速度
        float targetTilt = -(motionState.velocity.x / maxLaneV) * 12;
        float blend = 1 - std::exp(-14 * dt);
        motionState.tiltDeg = LerpFloat(motionState.tiltDeg, targetTilt, blend);
        motionState.recoil *= std::exp(-18 * dt);

        // 消除微小残留
        if (std::fabs(motionState.tiltDeg) < 0.02f) motionState.tiltDeg = 0;
        if (std::fabs(motionState.recoil) < 0.02f) motionState.recoil = 0;
        return true;
    }

    void draw() override {
        float w = width * pose.screenScale, h = height * pose.screenScale;
        float x = pose.screenPos.x;
        float y = pose.screenPos.y - motionState.recoil - (std::fabs(motionState.tiltDeg) / 12) * 3;

        if (resMgr->isPlayerImageValid()) {
            Texture2D tex = *resMgr->getPlayerImage();
            Rectangle src = {0, 0, (float)tex.width, (float)tex.height};
            Rectangle dst = {x, y, w, h};
            ShipVolumeStyle style;
            style.maxThicknessPx = 5;
            style.shadowBoost = 1.10f;
            style.highlightAlpha = 78;
            style.rimAlpha = 60;
            GraphicsEngine::drawVolumetricSprite(tex, src, dst, {w * 0.5f, h * 0.5f}, motionState.tiltDeg, style, WHITE);
        } else {
            // 无纹理回退
            DrawTriangle({x, y - h * 0.50f}, {x - w * 0.50f, y + h * 0.50f}, {x + w * 0.50f, y + h * 0.50f}, GameConfig::COLOR_PLAYER);
            DrawCircleV({x, y - h * 0.25f}, std::max(1.0f, w * 0.10f), WHITE);
            BeginBlendMode(BLEND_ADDITIVE);
            DrawLineEx({x - w * 0.18f, y - h * 0.28f}, {x + w * 0.18f, y - h * 0.28f}, std::max(1.0f, w * 0.07f), {255, 230, 190, 74});
            DrawLineEx({x - w * 0.18f, y - h * 0.18f}, {x - w * 0.24f, y + h * 0.20f}, std::max(1.0f, w * 0.04f), {120, 220, 255, 64});
            DrawLineEx({x + w * 0.18f, y - h * 0.18f}, {x + w * 0.24f, y + h * 0.20f}, std::max(1.0f, w * 0.04f), {120, 220, 255, 64});
            EndBlendMode();
        }
    }

    // 触发后坐力
    void triggerRecoil(float amount) { motionState.recoil = ClampFloat(motionState.recoil + amount, 0, 5); }
    const PlayerMotionState& getMotionState() const { return motionState; }
};

/* ==================== UI 按钮类 ==================== */
class Button {
    int x = 0, y = 0, w = 0, h = 0;
    string text;
    bool isHover = false;
    bool isPressedVisual = false;
    int fontSize = 16;
    Color baseColor = {255, 255, 255, 255};
    string fontName = "ui";

public:
    Button() {}
    Button(int _x, int _y, int _w, int _h, string _text, int _fs, Color _c, string _fn)
        : x(_x), y(_y), w(_w), h(_h), text(_text), fontSize(_fs), baseColor(_c), fontName(_fn) {}

    // 绘制按钮（圆角矩形 + 阴影 + 霓虹文字）
    void draw() {
        float scale = isPressedVisual ? 0.96f : 1.0f;
        float cx = x + w * 0.5f, cy = y + h * 0.5f;
        float drawW = w * scale, drawH = h * scale;

        Rectangle mainRect = {cx - drawW * 0.5f, cy - drawH * 0.5f, drawW, drawH};
        Rectangle shadowRect = {mainRect.x + GameConfig::S(2), mainRect.y + GameConfig::S(2), drawW, drawH};

        int minSide = std::min((int)drawW, (int)drawH);
        float roundness = minSide > 0 ? std::min(1.0f, (float)(GameConfig::S(6) * 2) / minSide) : 0;

        // 悬停光晕
        if (isHover) {
            Rectangle glowRect = {mainRect.x - GameConfig::S(3), mainRect.y - GameConfig::S(3),
                                  mainRect.width + GameConfig::S(6), mainRect.height + GameConfig::S(6)};
            DrawRectangleRounded(glowRect, roundness, 8, {170, 235, 255, 55});
        }

        // 阴影
        DrawRectangleRounded(shadowRect, roundness, 8, {8, 10, 20, 190});

        // 主体（悬停时变亮）
        int r = std::min(255, (int)baseColor.r + (isHover ? 45 : 0));
        int g = std::min(255, (int)baseColor.g + (isHover ? 45 : 0));
        int b = std::min(255, (int)baseColor.b + (isHover ? 45 : 0));
        DrawRectangleRounded(mainRect, roundness, 8, {(unsigned char)r, (unsigned char)g, (unsigned char)b, 255});

        // 边框
        Color border = isHover
            ? Color{240, 248, 255, 255}
            : Color{(unsigned char)std::min(255, r + 24), (unsigned char)std::min(255, g + 24), (unsigned char)std::min(255, b + 24), 255};
        DrawRectangleRoundedLinesEx(mainRect, roundness, 8, (float)std::max(2, GameConfig::S(1)), border);

        // 按钮文字
        GraphicsEngine::drawFxTextCenter((int)(mainRect.x + mainRect.width * 0.5f),
            (int)(mainRect.y + mainRect.height * 0.5f), text.c_str(), fontSize, isHover ? 0.72f : 0.58f, 0, fontName.c_str());
    }

    // 更新悬停和点击状态，返回是否被点击
    bool update(float mx, float my, bool pressed, bool down) {
        isHover = IsPointInRect(mx, my, x, y, w, h);
        isPressedVisual = isHover && down;
        return isHover && pressed;
    }

    void setPosition(int _x, int _y) { x = _x; y = _y; }
    void setSize(int _w, int _h) { w = _w; h = _h; }
    void setText(const string& s) { text = s; }
    bool getHoverState() const { return isHover; }
};

/* ==================== 游戏状态枚举 ==================== */
enum GameState { MENU, PLAYING, PAUSED, END };

/* ==================== 粒子系统 ==================== */
// 管理爆炸、枪焰等粒子特效的生命周期和渲染
class ParticleSystem {
    static const int MAX_PARTICLES = 550;
    static const int MAX_SPAWN_PER_FRAME = 80;
    array<Particle, MAX_PARTICLES> particles = {};
    int spawnedThisFrame = 0;

public:
    ParticleSystem() { clear(); }

    void clear() {
        for (auto& p : particles) p = Particle();
        spawnedThisFrame = 0;
    }

    void beginFrame() { spawnedThisFrame = 0; }

    // 发射一个粒子，如果空间不足则尝试替换低优先级粒子
    bool emit(const Particle& p) {
        if (spawnedThisFrame >= MAX_SPAWN_PER_FRAME) return false;

        // 寻找空闲槽位
        int idx = -1;
        for (int i = 0; i < (int)particles.size(); ++i) {
            if (!particles[i].active) { idx = i; break; }
        }

        // 无空闲则替换最低优先级+最短剩余寿命的粒子
        if (idx < 0) {
            int lowestPri = numeric_limits<int>::max();
            float lowestRatio = numeric_limits<float>::max();
            int replaceIdx = -1;
            for (int i = 0; i < (int)particles.size(); ++i) {
                const Particle& cur = particles[i];
                if (!cur.active) continue;
                float ratio = cur.maxLife > 0 ? cur.life / cur.maxLife : 0;
                if (cur.priority < lowestPri || (cur.priority == lowestPri && ratio < lowestRatio)) {
                    lowestPri = cur.priority;
                    lowestRatio = ratio;
                    replaceIdx = i;
                }
            }
            if (replaceIdx >= 0 && particles[replaceIdx].priority <= p.priority)
                idx = replaceIdx;
        }
        if (idx < 0) return false;

        Particle next = p;
        next.active = true;
        if (next.maxLife <= 0) next.maxLife = 0.001f;
        if (next.life <= 0) next.life = next.maxLife;
        particles[idx] = next;
        spawnedThisFrame++;
        return true;
    }

    // 每帧更新所有活跃粒子
    void update(float dt) {
        if (dt <= 0) return;
        for (auto& p : particles) {
            if (!p.active) continue;
            p.life -= dt;
            if (p.life <= 0) { p.active = false; continue; }
            p.position.x += p.velocity.x * dt;
            p.position.y += p.velocity.y * dt;
            p.rotation += p.spin * dt;
            float drag = ClampFloat(1 - dt * 2.6f, 0, 1);
            p.velocity.x *= drag;
            p.velocity.y *= drag;
        }
    }

    // 绘制所有活跃粒子
    void draw() const {
        for (const auto& p : particles) {
            if (!p.active) continue;
            float t = 1 - ClampFloat(p.life / p.maxLife, 0, 1);
            Color c = LerpColor(p.startColor, p.endColor, t);
            float radius = std::max(1.0f, p.size * (1 - t * 0.35f));
            DrawCircleV(p.position, radius, c);
            DrawCircleV(p.position, radius * 0.45f, {255, 255, 255, (unsigned char)(c.a * 0.45f)});
        }
    }

    int aliveCount() const {
        int n = 0;
        for (const auto& p : particles) if (p.active) n++;
        return n;
    }
};

/* ==================== 8bit 芯片音乐引擎 ==================== */
// 程序化合成循环 BGM：方波旋律 + 方波低音 + 三角波琶音 + 鼓组
class ChipMusicEngine {
    bool initialized = false;
    bool muted = false;
    AudioStream stream = {};

    int sampleRate = 44100;
    int chunkFrames = 1024;
    vector<float> buffer;

    float globalTime = 0;
    float phase1 = 0, phase2 = 0, phase3 = 0;  // 三个振荡器相位
    float lowpassState = 0;

    int stepIndex = 0;
    float stepTime = 0;
    float stepDuration = 60.0f / 150.0f / 4.0f;  // BPM=150, 16分音符
    int section = 0, barIndex = 0, stepInBar = 0;
    int arrangementIndex = 0, fillCountdown = 0;
    uint32_t noiseState = 0x12345678u;  // 噪声发生器状态

    // MIDI 音符号 -> 频率
    float midiToFreq(int note) const { return 440 * std::pow(2.0f, (note - 69) / 12.0f); }
    // 方波（可调占空比）
    float squareWave(float ph, float duty) const { return ph < duty ? 1.0f : -1.0f; }
    // 三角波
    float triWave(float ph) const { return 4 * std::fabs(ph - 0.5f) - 1; }
    // 伪随机噪声
    float nextNoise() {
        noiseState = noiseState * 1664525u + 1013904223u;
        return (float)((noiseState >> 8) & 0xFFFFu) / 32767.5f - 1;
    }

    // 生成一块音频数据
    void generateChunk() {
        // 旋律/低音/琶音/鼓组的模式序列
        static const int melodyA[16] = {79,82,86,82,79,74,79,82,86,89,86,82,79,74,77,79};
        static const int melodyAp[16] = {79,82,86,89,91,89,86,82,79,82,84,86,84,82,79,77};
        static const int melodyB[16] = {74,77,81,84,81,77,74,77,79,82,86,89,86,82,79,82};
        static const int melodyBp[16] = {74,77,81,86,84,81,77,74,79,82,86,91,89,86,82,79};
        static const int melodyFill[16] = {91,89,86,84,82,81,79,77,76,77,79,81,82,84,86,89};

        static const int bassA[16] = {43,0,43,0,38,0,38,0,41,0,41,0,36,0,38,0};
        static const int bassB[16] = {38,0,38,0,41,0,41,0,43,0,43,0,36,0,38,0};
        static const int bassFill[16] = {43,43,41,41,38,38,36,36,38,38,41,41,43,43,46,46};

        static const int arpA[16] = {67,71,74,79,62,67,71,74,64,67,71,76,60,64,67,71};
        static const int arpB[16] = {62,65,69,74,64,67,71,76,67,71,74,79,60,64,67,71};
        static const int arpFill[16] = {71,74,79,83,69,72,76,81,67,71,74,79,64,67,71,76};

        static const int kickA[16]  = {1,0,0,0,1,0,0,0,1,0,0,1,1,0,0,0};
        static const int snareA[16] = {0,0,1,0,0,0,1,0,0,0,1,0,0,0,1,0};
        static const int ghostA[16] = {0,1,0,0,0,1,0,1,0,1,0,0,0,1,0,1};

        static const int kickB[16]  = {1,0,0,1,0,0,1,0,1,0,0,1,0,1,0,0};
        static const int snareB[16] = {0,0,1,0,0,1,0,0,0,0,1,0,0,1,0,0};
        static const int ghostB[16] = {0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1};

        static const int kickFill[16]  = {1,1,0,1,1,0,1,1,1,0,1,1,1,1,1,1};
        static const int snareFill[16] = {0,0,1,0,0,1,0,1,0,1,0,1,0,1,1,1};
        static const int ghostFill[16] = {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1};

        float dt = 1.0f / sampleRate;

        for (int i = 0; i < chunkFrames; ++i) {
            globalTime += dt;
            stepTime += dt;

            // 步进序列器
            while (stepTime >= stepDuration) {
                stepTime -= stepDuration;
                stepInBar = (stepInBar + 1) & 15;
                stepIndex = stepInBar;
                if (fillCountdown > 0) fillCountdown--;
                if (stepInBar == 0) {
                    barIndex = (barIndex + 1) & 7;
                    arrangementIndex = (arrangementIndex + 1) & 127;
                    if (barIndex == 6) fillCountdown = 16;
                }
                // 根据小节确定段落：A / A' / B / Fill / B'
                if (barIndex < 2) section = 0;
                else if (barIndex < 4) section = 1;
                else if (barIndex < 6) section = 2;
                else if (barIndex == 6) section = 3;
                else section = 4;
            }

            float stepPhase = stepTime / stepDuration;
            float gate = 1 - ClampFloat(stepPhase, 0, 1);
            int s = stepInBar & 15;

            // 选择当前段落的模式
            const int* melody = melodyA, *bass = bassA, *arp = arpA;
            const int* kickPat = kickA, *snarePat = snareA, *ghostPat = ghostA;
            if (section == 1) { melody = melodyAp; }
            else if (section == 2) { melody = melodyB; bass = bassB; arp = arpB; kickPat = kickB; snarePat = snareB; ghostPat = ghostB; }
            else if (section == 3) { melody = melodyFill; bass = bassFill; arp = arpFill; kickPat = kickFill; snarePat = snareFill; ghostPat = ghostFill; }
            else if (section == 4) { melody = melodyBp; bass = bassB; arp = arpB; kickPat = kickB; snarePat = snareB; ghostPat = ghostB; }

            int noteMain = melody[s], noteBass = bass[s];
            int noteArp = arp[(s + ((int)(stepPhase * 4) & 3)) & 15];

            // 调制占空比和门限
            float duty1 = ClampFloat(0.25f + 0.03f * (((barIndex + arrangementIndex) & 1) ? 1.0f : -1.0f), 0.18f, 0.34f);
            float duty2 = ClampFloat(0.50f + 0.02f * ((section == 2 || section == 4) ? 1.0f : -1.0f), 0.44f, 0.56f);
            bool isFill = (section == 3);
            float gateMain = std::pow(gate, isFill ? 0.36f : 0.52f);
            float gateBass = std::pow(gate, isFill ? 0.56f : 0.72f);
            float gateArp  = std::pow(gate, isFill ? 0.70f : 0.88f);

            // 三个声道合成
            float ch1 = 0, ch2 = 0, ch3 = 0;
            if (noteMain > 0) {
                phase1 += midiToFreq(noteMain) * dt;
                if (phase1 >= 1) phase1 -= std::floor(phase1);
                ch1 = squareWave(phase1, duty1) * gateMain;
            }
            if (noteBass > 0) {
                phase2 += midiToFreq(noteBass) * dt;
                if (phase2 >= 1) phase2 -= std::floor(phase2);
                ch2 = squareWave(phase2, duty2) * gateBass;
            }
            if (noteArp > 0) {
                phase3 += midiToFreq(noteArp) * dt;
                if (phase3 >= 1) phase3 -= std::floor(phase3);
                ch3 = triWave(phase3) * (0.62f * gateArp);
            }

            // 鼓组合成
            float kick = 0;
            if (kickPat[s] && stepPhase < 0.30f) {
                float e = std::exp(-stepPhase * (isFill ? 20.0f : 18.0f));
                kick = std::sin(2 * kPi * (55 + (isFill ? 120.0f : 90.0f) * (1 - stepPhase)) * globalTime) * e;
            }
            float snare = 0;
            if (snarePat[s] && stepPhase < 0.24f)
                snare = nextNoise() * std::exp(-stepPhase * 24);
            float ghost = 0;
            if (ghostPat[s] && stepPhase < 0.14f)
                ghost = nextNoise() * std::exp(-stepPhase * 32) * 0.20f;

            // 混音
            float mix = ch1 * 0.41f + ch2 * 0.35f + ch3 * 0.20f + kick * 0.64f + snare * 0.40f + ghost;
            if (isFill || fillCountdown > 0) mix *= 1.06f;
            if (muted) mix = 0;
            mix *= 0.28f;

            // 低通滤波 + 软限幅
            float lpAlpha = 0.14f + 0.02f * ((barIndex & 1) ? 1.0f : 0.0f)
                          + 0.02f * ((section == 2 || section == 4) ? 1.0f : 0.0f);
            lpAlpha = ClampFloat(lpAlpha, 0.12f, 0.21f);
            lowpassState += (mix - lowpassState) * lpAlpha;
            buffer[i] = (float)std::tanh(lowpassState * 1.9f) * 0.92f;
        }
    }

public:
    ChipMusicEngine() {}
    ~ChipMusicEngine() { shutdown(); }

    bool init() {
        if (initialized) return true;
        if (!IsAudioDeviceReady()) InitAudioDevice();
        SetAudioStreamBufferSizeDefault(chunkFrames);
        stream = LoadAudioStream(sampleRate, 32, 1);
        PlayAudioStream(stream);
        buffer.assign(chunkFrames, 0);
        initialized = true;
        return true;
    }

    // 当音频流需要数据时生成新的音频块
    void update(float) {
        if (!initialized) return;
        while (IsAudioStreamProcessed(stream)) {
            generateChunk();
            UpdateAudioStream(stream, buffer.data(), chunkFrames);
        }
    }

    void toggleMute() { muted = !muted; }
    bool isMuted() const { return muted; }

    void shutdown() {
        if (initialized) {
            StopAudioStream(stream);
            UnloadAudioStream(stream);
            initialized = false;
        }
        if (IsAudioDeviceReady()) CloseAudioDevice();
    }
};

/* ==================== 游戏管理器（主控类） ==================== */
// 负责游戏循环、状态管理、实体管理、渲染和输入
class GameManager {
    ResourceManager resourceManager;
    GameState currentState = MENU;

    Player* player = nullptr;
    list<Bullet*> bullets;         // 玩家子弹链表
    list<Bullet*> enemyBullets;    // 敌人子弹链表（统一用 Bullet 类）
    list<Enemy*> enemies;          // 敌机链表

    int score = 0;
    bool gameOver = false;
    float deltaTime = 1.0f / 60;
    float uiTime = 0;
    float shootCooldown = 0;       // 射击冷却
    float pauseCooldown = 0;       // 暂停防抖

    int enemySpawnRate = 30;       // 敌人生成速率（帧数间隔）
    int enemyShootChance = 2;      // 敌人射击概率
    float enemySpawnTimer = 0.5f;
    float enemyAdvanceSpeed = 0.33f;
    float enemyBulletSpeed = 0.66f;
    float hitStopTimer = 0;        // 命中停顿（增强打击感）
    float screenFlashAlpha = 0;    // 屏幕闪白强度

    float endScoreAnimTimer = 0;   // 结算分数动画计时
    int animatedEndScore = 0;
    UiDriftState titleDrift, hudDrift, microDrift;

    CameraFXState cameraFX;
    ParticleSystem particleSystem;
    ChipMusicEngine chipMusic;

    PerspectiveConfig perspectiveCfg;
    PerspectiveMapper perspectiveMapper;

    array<ParallaxLayer, 2> parallaxLayers = {};
    vector<Vector2> farStars;      // 远景星星
    vector<Vector2> midClouds;     // 中景星云

    Vector2 pendingHitPos = {0, 0};
    bool pendingHitFX = false;

    Button btnEasy, btnNormal, btnHell;
    Button btnPause, btnResume, btnMenu;

    /* --- 初始化 --- */

    // 初始化所有 UI 按钮的位置和样式
    void initUI() {
        int winW = GameConfig::GetWindowWidth(), winH = GameConfig::GetWindowHeight();
        int btnW = GameConfig::S(96), btnH = GameConfig::S(28);
        int btnX = winW / 2 - btnW / 2;

        btnEasy   = Button(btnX, GameConfig::S(95),  btnW, btnH, Texts::BTN_EASY,   GameConfig::S(13), {40,160,80,255},  "ui");
        btnNormal = Button(btnX, GameConfig::S(135), btnW, btnH, Texts::BTN_NORMAL,  GameConfig::S(13), {50,120,210,255}, "ui");
        btnHell   = Button(btnX, GameConfig::S(175), btnW, btnH, Texts::BTN_HELL,    GameConfig::S(13), {180,50,50,255},  "ui");

        btnPause  = Button(winW - GameConfig::S(45), GameConfig::S(5), GameConfig::S(40), GameConfig::S(20), Texts::BTN_PAUSE,  GameConfig::S(12), {50,120,210,255}, "ui");
        btnMenu   = Button(winW - GameConfig::S(90), GameConfig::S(5), GameConfig::S(40), GameConfig::S(20), Texts::BTN_MENU,   GameConfig::S(12), {200,80,80,255},  "ui");
        btnResume = Button(winW / 2 - GameConfig::S(45), winH / 2 + GameConfig::S(10), GameConfig::S(90), GameConfig::S(28), Texts::BTN_RESUME, GameConfig::S(14), {50,120,210,255}, "ui");
    }

    // 初始化视差背景层和星星/星云位置
    void initBackgroundLayers() {
        parallaxLayers[0] = {(float)GameConfig::S(20), 0, 0.70f, 120, 2, {220,232,255,120}};
        parallaxLayers[1] = {(float)GameConfig::S(32), 0, 0.30f, 34,  12, {152,125,210,70}};
        int winW = GameConfig::GetWindowWidth(), winH = GameConfig::GetWindowHeight();

        farStars.clear();
        farStars.reserve(parallaxLayers[0].density);
        for (int i = 0; i < parallaxLayers[0].density; ++i)
            farStars.push_back({RandomRange(0, (float)winW), RandomRange(0, (float)winH)});

        midClouds.clear();
        midClouds.reserve(parallaxLayers[1].density);
        for (int i = 0; i < parallaxLayers[1].density; ++i)
            midClouds.push_back({RandomRange(0, (float)winW), RandomRange(0, (float)winH)});
    }

    // 释放所有游戏实体内存
    void clearEntities() {
        delete player; player = nullptr;
        for (auto b : bullets) delete b;       bullets.clear();
        for (auto e : enemies) delete e;       enemies.clear();
        for (auto eb : enemyBullets) delete eb; enemyBullets.clear();
    }

    // 设置难度参数
    void setDifficulty(int spawnRate, int shootChance) {
        enemySpawnRate = std::max(1, spawnRate);
        enemyShootChance = std::max(0, shootChance);
        enemySpawnTimer = std::max(0.05f, enemySpawnRate / 60.0f);
        enemyAdvanceSpeed = 0.24f + (60 - (float)enemySpawnRate) / 140;
        enemyBulletSpeed = 0.58f + enemyShootChance * 0.032f;
    }

    // 进入结算状态
    void enterEndState() {
        if (currentState == END) return;
        currentState = END;
        endScoreAnimTimer = 0;
        animatedEndScore = 0;
        pauseCooldown = 0.20f;
    }

    /* --- 特效生成 --- */

    // 枪口火焰粒子
    void spawnMuzzleFX() {
        if (!player) return;
        const PerspectivePose& p = player->getPose();
        float gunY = p.screenPos.y - player->getHeight() * p.screenScale * 0.33f;
        float wing = player->getWidth() * p.screenScale * 0.25f;
        float gunX[2] = {p.screenPos.x - wing, p.screenPos.x + wing};

        for (int g = 0; g < 2; ++g) {
            int count = 6 + rand() % 5;
            for (int i = 0; i < count; ++i) {
                Particle pt;
                pt.active = true;
                pt.position = {gunX[g] + RandomRange(-2, 2), gunY + RandomRange(-2, 2)};
                pt.velocity = {RandomRange(-55, 55), RandomRange(-420, -250)};
                pt.maxLife = RandomRange(0.12f, 0.18f);
                pt.life = pt.maxLife;
                pt.size = RandomRange((float)GameConfig::S(1), (float)GameConfig::S(3));
                pt.rotation = RandomRange(0, 360);
                pt.spin = RandomRange(-120, 120);
                pt.startColor = {120, 220, 255, 230};
                pt.endColor = {80, 160, 255, 0};
                pt.priority = 1;
                particleSystem.emit(pt);
            }
        }
    }

    // 命中爆炸粒子
    void spawnHitFX() {
        if (!pendingHitFX) return;
        int count = 20 + rand() % 9;
        for (int i = 0; i < count; ++i) {
            float angle = RandomRange(0, kTau);
            float spd = RandomRange(130, 360);
            Particle pt;
            pt.active = true;
            pt.position = pendingHitPos;
            pt.velocity = {std::cos(angle) * spd, std::sin(angle) * spd};
            pt.maxLife = RandomRange(0.16f, 0.28f);
            pt.life = pt.maxLife;
            pt.size = RandomRange((float)GameConfig::S(1), (float)GameConfig::S(4));
            pt.rotation = RandomRange(0, 360);
            pt.spin = RandomRange(-200, 200);
            pt.startColor = {255, 220, 150, 240};
            pt.endColor = {255, 80, 40, 0};
            pt.priority = 3;
            particleSystem.emit(pt);
        }
        screenFlashAlpha = std::max(screenFlashAlpha, 34.0f);
        cameraFX.trauma = ClampFloat(cameraFX.trauma + 0.18f, 0, 1);
        hitStopTimer = std::max(hitStopTimer, 0.035f);
        pendingHitFX = false;
    }

    /* --- 更新函数 --- */

    // 更新视差背景滚动
    void updateParallax(float dt) {
        if (dt <= 0) return;
        float speedBoost = 1;
        if (player && currentState == PLAYING) {
            const auto& ms = player->getMotionState();
            speedBoost += ClampFloat((std::fabs(ms.velocity.x) + std::fabs(ms.velocity.y)) / 2, 0, 1) * 0.4f;
        }
        float h = (float)GameConfig::GetWindowHeight();
        parallaxLayers[0].offsetY = WrapFloat(parallaxLayers[0].offsetY + parallaxLayers[0].speed * dt * 0.7f * speedBoost, h);
        parallaxLayers[1].offsetY = WrapFloat(parallaxLayers[1].offsetY + parallaxLayers[1].speed * dt * 0.9f * speedBoost, h);
    }

    // 更新摄像机震动/旋转/缩放效果
    void updateCameraFX(float dt) {
        cameraFX.trauma = std::max(0.0f, cameraFX.trauma - 2.2f * dt);
        float shake = cameraFX.trauma * cameraFX.trauma;
        cameraFX.shakeX = RandomRange(-7, 7) * shake;
        cameraFX.shakeY = RandomRange(-7, 7) * shake;
        cameraFX.zoom = LerpFloat(cameraFX.zoom, 1 + shake * 0.02f, 1 - std::exp(-10 * dt));
        float playerRoll = player ? player->getMotionState().tiltDeg * 0.12f : 0;
        cameraFX.rollDeg = playerRoll + RandomRange(-1.8f, 1.8f) * shake;
    }

    // 更新 UI 文字漂移动画（弹簧阻尼系统）
    void updateUiDrift(float dt) {
        if (dt <= 0) return;
        auto tick = [dt](UiDriftState& s, float rMin, float rMax) {
            s.retargetTimer -= dt;
            if (s.retargetTimer <= 0) {
                s.target = RandomRange(-s.amp, s.amp);
                s.retargetTimer = RandomRange(rMin, rMax);
            }
            float accel = (s.target - s.x) * 26 - s.v * 8.5f;
            s.v += accel * dt;
            s.x += s.v * dt;
            s.x = ClampFloat(s.x, -s.amp * 1.35f, s.amp * 1.35f);
            if (std::fabs(s.x) < 0.004f && std::fabs(s.v) < 0.004f) { s.x = 0; s.v = 0; }
        };
        tick(titleDrift, 0.45f, 0.85f);
        tick(hudDrift, 0.32f, 0.64f);
        tick(microDrift, 0.26f, 0.48f);
    }

    // 重新计算 HUD 按钮布局
    void layoutHUD() {
        int winW = GameConfig::GetWindowWidth(), winH = GameConfig::GetWindowHeight();
        int topPad = GameConfig::S(6), sidePad = GameConfig::S(10), gap = GameConfig::S(4);
        int btnW = GameConfig::S(40), btnH = GameConfig::S(20);
        int pauseX = winW - sidePad - btnW;
        btnPause.setSize(btnW, btnH);
        btnMenu.setSize(btnW, btnH);
        btnPause.setPosition(pauseX, topPad);
        btnMenu.setPosition(pauseX - gap - btnW, topPad);
        btnResume.setSize(GameConfig::S(90), GameConfig::S(28));
        btnResume.setPosition(winW / 2 - GameConfig::S(45), winH / 2 + GameConfig::S(10));
    }

    // 将对象限制在走廊道路范围内
    void applyRoadBoundaryClamp(GameObject* obj, float extraMargin) {
        if (!obj) return;
        float depth = ClampFloat(obj->getDepthZ(), perspectiveCfg.minDepthZ, perspectiveCfg.maxDepthZ);
        float halfW = perspectiveMapper.laneHalfWidth(depth);
        if (halfW <= 0.001f) { obj->setLaneX(0); return; }

        float scale = obj->getScreenScale();
        if (scale <= 0.01f) scale = perspectiveMapper.depthToScale(depth);

        // 根据对象类型确定安全边距
        float safeR = obj->getWidth() * scale * 0.50f;
        if (dynamic_cast<Player*>(obj))
            safeR = obj->getWidth() * scale * 0.42f + GameConfig::S(2);
        else if (dynamic_cast<Enemy*>(obj))
            safeR = obj->getWidth() * scale * 0.42f + GameConfig::S(1);

        safeR += std::max(0.0f, extraMargin);
        float maxLane = ClampFloat((halfW - safeR) / halfW, 0, 1);
        obj->setLaneX(ClampFloat(obj->getLaneX(), -maxLane, maxLane));
    }

    // 处理玩家输入（移动 + 射击）
    void processInput(float worldDt) {
        if (!player) return;

        // P 键暂停
        if (IsKeyPressed(KEY_P) && pauseCooldown <= 0) {
            currentState = PAUSED;
            pauseCooldown = 0.20f;
            return;
        }
        if (worldDt <= 0) return;

        player->move(worldDt);
        applyRoadBoundaryClamp(player, 0);

        if (shootCooldown > 0)
            shootCooldown = std::max(0.0f, shootCooldown - worldDt);

        // 空格键连射
        if (IsKeyDown(KEY_SPACE) && shootCooldown <= 0) {
            float left  = ClampFloat(player->getLaneX() - 0.060f, -1, 1);
            float right = ClampFloat(player->getLaneX() + 0.060f, -1, 1);
            float d = player->getDepthZ() - 0.012f;
            bullets.push_back(new Bullet(left,  d, 1.45f, -1, &resourceManager));
            bullets.push_back(new Bullet(right, d, 1.45f, -1, &resourceManager));
            player->triggerRecoil(3);
            spawnMuzzleFX();
            shootCooldown = 0.10f;
        }
    }

    // 更新所有实体的透视位置、移动和排序
    void updatePerspectiveWorld(float dt) {
        if (player) {
            applyRoadBoundaryClamp(player, 0);
            player->updatePerspective(perspectiveCfg);
        }

        // 更新玩家子弹
        auto updateBulletList = [&](list<Bullet*>& lst, float boundCheck) {
            for (auto it = lst.begin(); it != lst.end();) {
                if (dt > 0 && !(*it)->move(dt)) {
                    delete *it; it = lst.erase(it);
                } else {
                    bool outOfBounds = (*it)->isPlayerBullet() ? ((*it)->getDepthZ() < -0.03f) : ((*it)->getDepthZ() > 1.02f);
                    if (outOfBounds) { delete *it; it = lst.erase(it); continue; }
                    applyRoadBoundaryClamp(*it, 0);
                    (*it)->updatePerspective(perspectiveCfg);
                    ++it;
                }
            }
        };
        updateBulletList(bullets, -0.03f);
        updateBulletList(enemyBullets, 1.02f);

        // 更新敌机
        for (auto it = enemies.begin(); it != enemies.end();) {
            if (dt > 0 && !(*it)->move(dt)) {
                delete *it; it = enemies.erase(it);
            } else {
                if ((*it)->getDepthZ() > 1.01f) { delete *it; it = enemies.erase(it); continue; }
                applyRoadBoundaryClamp(*it, 0);
                (*it)->updatePerspective(perspectiveCfg);
                ++it;
            }
        }

        // 按深度排序（远处先画）
        bullets.sort([](const Bullet* a, const Bullet* b) { return a->getDepthZ() < b->getDepthZ(); });
        enemyBullets.sort([](const Bullet* a, const Bullet* b) { return a->getDepthZ() < b->getDepthZ(); });
        enemies.sort([](const Enemy* a, const Enemy* b) { return a->getDepthZ() < b->getDepthZ(); });
    }

    // 检测所有碰撞：敌弹-玩家、敌机-玩家、玩家弹-敌机
    void resolvePerspectiveCollisions() {
        if (!player) return;
        const PerspectivePose& pp = player->getPose();
        float playerR = pp.screenRadius * 0.82f;

        // 敌人子弹 vs 玩家
        for (auto it = enemyBullets.begin(); it != enemyBullets.end();) {
            const PerspectivePose& ep = (*it)->getPose();
            float r = ep.screenRadius + playerR;
            if (DistSq(ep.screenPos, pp.screenPos) <= r * r) {
                gameOver = true;
                screenFlashAlpha = std::max(screenFlashAlpha, 92.0f);
                cameraFX.trauma = ClampFloat(cameraFX.trauma + 0.45f, 0, 1);
                hitStopTimer = std::max(hitStopTimer, 0.045f);
                delete *it; it = enemyBullets.erase(it);
                continue;
            }
            ++it;
        }

        // 敌机 vs 玩家（接触即死）
        for (auto it = enemies.begin(); it != enemies.end(); ++it) {
            const PerspectivePose& ep = (*it)->getPose();
            if (DistSq(ep.screenPos, pp.screenPos) <= (ep.screenRadius + playerR) * (ep.screenRadius + playerR)) {
                gameOver = true;
                screenFlashAlpha = std::max(screenFlashAlpha, 95.0f);
                cameraFX.trauma = ClampFloat(cameraFX.trauma + 0.45f, 0, 1);
                hitStopTimer = std::max(hitStopTimer, 0.045f);
                break;
            }
        }

        // 玩家子弹 vs 敌机
        for (auto eIt = enemies.begin(); eIt != enemies.end();) {
            bool destroyed = false;
            const PerspectivePose& ep = (*eIt)->getPose();

            for (auto bIt = bullets.begin(); bIt != bullets.end();) {
                const PerspectivePose& bp = (*bIt)->getPose();
                float r = ep.screenRadius + bp.screenRadius;
                if (DistSq(ep.screenPos, bp.screenPos) <= r * r) {
                    score += 10;
                    destroyed = true;
                    pendingHitPos = ep.screenPos;
                    pendingHitFX = true;
                    delete *bIt; bIt = bullets.erase(bIt);
                    break;
                }
                ++bIt;
            }

            if (destroyed) {
                spawnHitFX();
                delete *eIt; eIt = enemies.erase(eIt);
            } else {
                ++eIt;
            }
        }
    }

    // 更新游戏逻辑：生成敌人、敌人射击、碰撞检测
    void updateGameLogic(float worldDt) {
        if (!player) return;

        if (worldDt > 0) {
            // 定时生成敌机
            float interval = std::max(0.05f, enemySpawnRate / 60.0f);
            enemySpawnTimer -= worldDt;
            while (enemySpawnTimer <= 0) {
                enemies.push_back(new Enemy(RandomRange(-0.92f, 0.92f), 0.04f, enemyAdvanceSpeed, &resourceManager));
                enemySpawnTimer += interval;
            }

            // 敌人随机射击（概率与时间步长相关）
            float pScaled = ClampFloat(1 - std::pow(1 - enemyShootChance / 100.0f, worldDt * 60), 0, 0.95f);
            for (auto e : enemies)
                if (Random01() < pScaled)
                    enemyBullets.push_back(new Bullet(e->getLaneX(), e->getDepthZ() + 0.02f, enemyBulletSpeed, +1, &resourceManager));
        }

        updatePerspectiveWorld(worldDt);
        resolvePerspectiveCollisions();

        if (worldDt > 0) particleSystem.update(worldDt);
        if (gameOver) enterEndState();
    }

    /* --- 绘制函数 --- */

    // 绘制椭圆阴影（三层渐变模拟柔和阴影）
    void drawShadowEllipse(float x, float y, float rw, float rh, unsigned char alpha) const {
        DrawEllipse((int)x, (int)y, rw, rh, {10,10,15, (unsigned char)(alpha * 0.45f)});
        DrawEllipse((int)x, (int)y, rw * 1.30f, rh * 1.22f, {10,10,15, (unsigned char)(alpha * 0.25f)});
        DrawEllipse((int)x, (int)y, rw * 1.60f, rh * 1.45f, {10,10,15, (unsigned char)(alpha * 0.12f)});
    }

    // 绘制走廊透视引导线
    void drawCorridorGuides() {
        float phase = WrapFloat(uiTime * 0.85f, 0.09f);

        // 纵向车道标线
        float laneMarks[3] = {-0.66f, 0, 0.66f};
        for (float lane : laneMarks) {
            for (float d = phase; d < 1; d += 0.09f) {
                float d2 = d + 0.045f;
                if (d2 > 1) break;
                Vector2 p0 = perspectiveMapper.projectToScreen(lane, d);
                Vector2 p1 = perspectiveMapper.projectToScreen(lane, d2);
                float nf = ClampFloat((d + d2) * 0.5f, 0, 1);
                DrawLineEx(p0, p1, LerpFloat(1.2f, 4, nf), {120,230,255, (unsigned char)(45 + 95 * nf)});
            }
        }

        // 横向参考线
        for (float d = phase; d < 1; d += 0.09f) {
            Vector2 l = perspectiveMapper.projectToScreen(-1, d);
            Vector2 r = perspectiveMapper.projectToScreen(1, d);
            float nf = ClampFloat(d, 0, 1);
            DrawLineEx(l, r, LerpFloat(0.9f, 2.4f, nf), {130,185,255, (unsigned char)(32 + 84 * nf)});
        }

        // 走廊侧壁辐射线
        float topY = perspectiveCfg.horizonY, bottomY = perspectiveCfg.bottomY;
        float cx = GameConfig::GetWindowWidth() * 0.5f;
        for (int i = 1; i <= 4; ++i) {
            float t = i / 5.0f;
            float xL = LerpFloat(0, cx - perspectiveCfg.laneHalfFar, t);
            float xR = LerpFloat((float)GameConfig::GetWindowWidth(), cx + perspectiveCfg.laneHalfFar, t);
            float thick = LerpFloat(0.8f, 1.8f, 1 - t);
            DrawLineEx({xL, bottomY}, {cx - perspectiveCfg.laneHalfFar, topY}, thick, {75,100,150,52});
            DrawLineEx({xR, bottomY}, {cx + perspectiveCfg.laneHalfFar, topY}, thick, {75,100,150,52});
        }
    }

    // 绘制完整走廊背景（图片/渐变 + 星星 + 星云 + 道路 + 引导线）
    void drawCorridorBackground() {
        int winW = GameConfig::GetWindowWidth(), winH = GameConfig::GetWindowHeight();

        // 背景图片或渐变
        if (resourceManager.isBackgroundImageValid()) {
            Texture2D tex = *resourceManager.getBackgroundImage();
            DrawTexturePro(tex, {0,0,(float)tex.width,(float)tex.height}, {0,0,(float)winW,(float)winH}, {0,0}, 0, {255,255,255,205});
        } else {
            DrawRectangleGradientV(0, 0, winW, winH, {16,18,28,255}, {38,28,52,255});
        }

        // 远景星星层
        const auto& far = parallaxLayers[0];
        for (size_t i = 0; i < farStars.size(); ++i) {
            Vector2 p = farStars[i];
            float fy = WrapFloat(p.y + far.offsetY, (float)winH);
            float fx = WrapFloat(p.x + std::sin((p.y + uiTime * 40) * 0.009f) * far.drift, (float)winW);
            float twinkle = 0.55f + 0.45f * std::sin(p.x * 0.03f + p.y * 0.02f + uiTime * 2.4f);
            DrawCircleV({fx, fy}, 1 + twinkle * 1.25f, {far.color.r, far.color.g, far.color.b, (unsigned char)(far.color.a * twinkle * far.alpha)});
        }

        // 中景星云层
        const auto& mid = parallaxLayers[1];
        for (size_t i = 0; i < midClouds.size(); ++i) {
            Vector2 p = midClouds[i];
            float fy = WrapFloat(p.y + mid.offsetY, (float)winH);
            float fx = WrapFloat(p.x + std::sin((p.y + uiTime * 30) * 0.007f) * mid.drift, (float)winW);
            float radius = (float)GameConfig::S(15) + (float)(i % 3) * GameConfig::S(6);
            DrawCircleGradient((int)fx, (int)fy, radius,
                {mid.color.r, mid.color.g, mid.color.b, (unsigned char)(mid.color.a * 0.65f)},
                {mid.color.r, mid.color.g, mid.color.b, 0});
        }

        // 走廊道路区域（梯形）
        Vector2 topL = {GameConfig::GetWindowWidth() * 0.5f - perspectiveCfg.laneHalfFar, perspectiveCfg.horizonY};
        Vector2 topR = {GameConfig::GetWindowWidth() * 0.5f + perspectiveCfg.laneHalfFar, perspectiveCfg.horizonY};
        Vector2 botL = {GameConfig::GetWindowWidth() * 0.5f - perspectiveCfg.laneHalfNear, perspectiveCfg.bottomY};
        Vector2 botR = {GameConfig::GetWindowWidth() * 0.5f + perspectiveCfg.laneHalfNear, perspectiveCfg.bottomY};

        DrawTriangle(topL, botL, botR, {23,29,45,220});
        DrawTriangle(topL, topR, botR, {28,34,52,220});

        // 左右侧翼暗区
        DrawTriangle({0, perspectiveCfg.horizonY - GameConfig::S(8)}, topL, botL, {18,20,30,185});
        DrawTriangle({0, (float)winH}, {0, perspectiveCfg.horizonY - GameConfig::S(8)}, botL, {16,18,28,195});
        DrawTriangle(topR, {(float)winW, perspectiveCfg.horizonY - GameConfig::S(8)}, botR, {18,20,30,185});
        DrawTriangle({(float)winW, perspectiveCfg.horizonY - GameConfig::S(8)}, {(float)winW, (float)winH}, botR, {16,18,28,195});

        drawCorridorGuides();

        // 走廊左右边界线
        DrawLineEx(topL, botL, 2, {160,200,255,85});
        DrawLineEx(topR, botR, 2, {160,200,255,85});

        // 顶部和底部渐变暗角
        DrawRectangleGradientV(0, 0, winW, GameConfig::S(22), {0,0,0,80}, {0,0,0,0});
        DrawRectangleGradientV(0, winH - GameConfig::S(42), winW, GameConfig::S(42), {0,0,0,0}, {0,0,0,100});
    }

    // 用摄像机变换绘制所有游戏实体（阴影 -> 实体 -> 粒子）
    void drawWorldWithCamera() {
        if (!player) return;

        // 设置带震动的 2D 摄像机
        float winW = (float)GameConfig::GetWindowWidth(), winH = (float)GameConfig::GetWindowHeight();
        Camera2D cam;
        cam.target = {winW * 0.5f, winH * 0.5f};
        cam.offset = {winW * 0.5f + cameraFX.shakeX, winH * 0.5f + cameraFX.shakeY};
        cam.rotation = cameraFX.rollDeg;
        cam.zoom = cameraFX.zoom;
        BeginMode2D(cam);

        // 收集所有对象并按深度排序
        vector<pair<float, GameObject*>> objects;
        objects.reserve(enemies.size() + bullets.size() + enemyBullets.size() + 1);
        for (auto e : enemies)       objects.push_back({e->getDepthZ(), e});
        for (auto b : bullets)       objects.push_back({b->getDepthZ(), b});
        for (auto eb : enemyBullets) objects.push_back({eb->getDepthZ(), eb});
        objects.push_back({player->getDepthZ(), player});
        std::sort(objects.begin(), objects.end(),
            [](const pair<float, GameObject*>& a, const pair<float, GameObject*>& b) { return a.first < b.first; });

        // 先画所有阴影
        for (size_t i = 0; i < objects.size(); ++i) {
            GameObject* obj = objects[i].second;
            const PerspectivePose& p = obj->getPose();
            float df = ClampFloat(p.depthZ, 0, 1);
            float rw = std::max(2.0f, p.screenRadius * 0.80f);
            float rh = std::max(1.0f, p.screenRadius * 0.24f);
            unsigned char a = (unsigned char)(55 + 85 * df);
            bool isShip = dynamic_cast<Enemy*>(obj) || dynamic_cast<Player*>(obj);
            if (isShip) { rw *= 1.10f; rh *= 1.10f; a = (unsigned char)std::min(255, (int)a + 8); }
            drawShadowEllipse(p.screenPos.x, p.screenPos.y + p.screenRadius * 0.85f, rw, rh, a);
        }

        // 再画所有实体
        for (size_t i = 0; i < objects.size(); ++i) objects[i].second->draw();

        particleSystem.draw();
        EndMode2D();
    }

    // 绘制屏幕后处理效果（闪白、暗角、扫描线）
    void drawScreenFX() {
        int winW = GameConfig::GetWindowWidth(), winH = GameConfig::GetWindowHeight();

        // 命中闪白
        if (screenFlashAlpha > 0) {
            DrawRectangle(0, 0, winW, winH, {224,238,255, (unsigned char)ClampFloat(screenFlashAlpha, 0, 255)});
            screenFlashAlpha = std::max(0.0f, screenFlashAlpha - 340 * deltaTime);
        }

        // 四边暗角
        DrawRectangleGradientV(0, 0, winW, GameConfig::S(26), {0,0,0,90}, {0,0,0,0});
        DrawRectangleGradientV(0, winH - GameConfig::S(48), winW, GameConfig::S(48), {0,0,0,0}, {0,0,0,110});
        DrawRectangleGradientH(0, 0, GameConfig::S(26), winH, {0,0,0,70}, {0,0,0,0});
        DrawRectangleGradientH(winW - GameConfig::S(26), 0, GameConfig::S(26), winH, {0,0,0,0}, {0,0,0,70});

        // CRT 扫描线效果
        for (int y = 0; y < winH; y += 4) {
            unsigned char a = (unsigned char)(10 + 6 * (0.5f + 0.5f * std::sin(uiTime * 42 + y * 0.045f)));
            DrawLine(0, y, winW, y, {22,24,35, a});
        }
    }

    // 绘制音乐开关状态指示
    void drawMusicIndicator() {
        int x = 0, y = 0;
        if (currentState == MENU) { x = GameConfig::GetWindowWidth() - GameConfig::S(36); y = GameConfig::S(9); }
        else if (currentState == PLAYING || currentState == PAUSED) { x = GameConfig::S(58); y = GameConfig::S(34); }
        else { x = GameConfig::S(58); y = GameConfig::S(20); }
        float drift = microDrift.x * 0.45f;
        GraphicsEngine::drawFxTextCenter(x, y, chipMusic.isMuted() ? Texts::MUSIC_OFF : Texts::MUSIC_ON, GameConfig::S(8), 0.40f, drift);
        GraphicsEngine::drawFxTextCenter(x, y + GameConfig::S(10), "M: TOGGLE", GameConfig::S(7), 0.32f, drift * 0.75f);
    }

    // 绘制统一 UI 层（根据当前状态绘制不同界面）
    void drawUnifiedUI() {
        int winW = GameConfig::GetWindowWidth(), winH = GameConfig::GetWindowHeight();
        int leftPad = GameConfig::S(10), topPad = GameConfig::S(6);
        layoutHUD();

        if (currentState == MENU) {
            GraphicsEngine::drawFxTextCenter(winW / 2, GameConfig::S(40), Texts::MENU_TITLE, GameConfig::S(22), 1, titleDrift.x);
            btnEasy.draw(); btnNormal.draw(); btnHell.draw();
            GraphicsEngine::drawFxTextCenter(winW / 2, winH - GameConfig::S(16), Texts::DEVELOPERS, GameConfig::S(10), 0.45f, microDrift.x * 0.65f);
            drawMusicIndicator();
            return;
        }

        if (currentState == PLAYING) {
            string scoreText = "SCORE: " + to_string(score);
            GraphicsEngine::drawFxTextCenter(leftPad + GameConfig::S(56), topPad + GameConfig::S(9), scoreText.c_str(), GameConfig::S(16), 0.70f, hudDrift.x);
            btnPause.draw(); btnMenu.draw();
            drawMusicIndicator();
            return;
        }

        if (currentState == PAUSED) {
            string scoreText = "SCORE: " + to_string(score);
            GraphicsEngine::drawFxTextCenter(leftPad + GameConfig::S(56), topPad + GameConfig::S(9), scoreText.c_str(), GameConfig::S(16), 0.65f, hudDrift.x * 0.95f);
            // 半透明暂停面板
            int bx = winW / 2 - GameConfig::S(84), by = winH / 2 - GameConfig::S(52);
            Rectangle panel = {(float)bx, (float)by, (float)GameConfig::S(168), (float)GameConfig::S(106)};
            DrawRectangleRounded(panel, 0.08f, 8, {26,28,40,225});
            DrawRectangleRoundedLinesEx(panel, 0.08f, 8, (float)std::max(2, GameConfig::S(1)), {220,230,255,220});
            GraphicsEngine::drawFxTextCenter(winW / 2, winH / 2 - GameConfig::S(24), Texts::PAUSED_TITLE, GameConfig::S(20), 0.95f, titleDrift.x * 0.72f);
            GraphicsEngine::drawFxTextCenter(winW / 2, winH / 2 - GameConfig::S(4), Texts::PAUSED_HINT, GameConfig::S(12), 0.55f, microDrift.x * 0.75f);
            btnResume.draw(); btnPause.draw(); btnMenu.draw();
            drawMusicIndicator();
            return;
        }

        if (currentState == END) {
            // 结算面板
            int bx = winW / 2 - GameConfig::S(114), by = winH / 2 - GameConfig::S(74);
            Rectangle panel = {(float)bx, (float)by, (float)GameConfig::S(228), (float)GameConfig::S(148)};
            DrawRectangleRounded(panel, 0.06f, 8, {12,12,18,232});
            DrawRectangleRoundedLinesEx(panel, 0.06f, 8, (float)std::max(2, GameConfig::S(1)), {220,230,255,210});
            GraphicsEngine::drawFxTextCenter(winW / 2, winH / 2 - GameConfig::S(46), "GAME OVER", GameConfig::S(28), 1, titleDrift.x);
            GraphicsEngine::drawFxTextCenter(winW / 2, winH / 2 + GameConfig::S(10), ("Final Score: " + to_string(animatedEndScore)).c_str(), GameConfig::S(16), 0.76f, hudDrift.x);
            GraphicsEngine::drawFxTextCenter(winW / 2, winH / 2 + GameConfig::S(42), Texts::END_HINT, GameConfig::S(12), 0.58f, microDrift.x * 0.70f);
            drawMusicIndicator();
        }
    }

    /* --- 各状态的更新函数 --- */

    void updateMenu() {
        drawCorridorBackground();
        Vector2 mp = GetMousePosition();
        bool pressed = IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
        bool down = IsMouseButtonDown(MOUSE_BUTTON_LEFT);

        if (btnEasy.update(mp.x, mp.y, pressed, down))   { setDifficulty(60, 1); resetGame(); currentState = PLAYING; return; }
        if (btnNormal.update(mp.x, mp.y, pressed, down)) { setDifficulty(30, 2); resetGame(); currentState = PLAYING; return; }
        if (btnHell.update(mp.x, mp.y, pressed, down))   { setDifficulty(10, 6); resetGame(); currentState = PLAYING; return; }

        drawScreenFX();
        drawUnifiedUI();
    }

    void updatePlaying() {
        drawCorridorBackground();
        Vector2 mp = GetMousePosition();
        bool pressed = IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
        bool down = IsMouseButtonDown(MOUSE_BUTTON_LEFT);

        if (btnPause.update(mp.x, mp.y, pressed, down) && pauseCooldown <= 0) { currentState = PAUSED; pauseCooldown = 0.20f; return; }
        if (btnMenu.update(mp.x, mp.y, pressed, down)) { currentState = MENU; resetGame(); return; }

        particleSystem.beginFrame();
        float worldDt = deltaTime;
        if (hitStopTimer > 0) { hitStopTimer = std::max(0.0f, hitStopTimer - deltaTime); worldDt = 0; }

        processInput(worldDt);
        if (currentState != PLAYING) return;
        updateGameLogic(worldDt);
        if (currentState != PLAYING) return;

        drawWorldWithCamera();
        drawScreenFX();
        drawUnifiedUI();
    }

    void updatePaused() {
        drawCorridorBackground();
        Vector2 mp = GetMousePosition();
        bool pressed = IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
        bool down = IsMouseButtonDown(MOUSE_BUTTON_LEFT);

        bool resumeClicked = btnResume.update(mp.x, mp.y, pressed, down);
        btnPause.update(mp.x, mp.y, false, down);
        bool menuClicked = btnMenu.update(mp.x, mp.y, pressed, down);

        if (resumeClicked && pauseCooldown <= 0) { currentState = PLAYING; pauseCooldown = 0.20f; return; }
        if (menuClicked) { currentState = MENU; resetGame(); return; }
        if (IsKeyPressed(KEY_P) && pauseCooldown <= 0) { currentState = PLAYING; pauseCooldown = 0.20f; return; }

        updatePerspectiveWorld(0);
        drawWorldWithCamera();
        drawScreenFX();
        drawUnifiedUI();
    }

    void updateGameOver() {
        drawCorridorBackground();
        updatePerspectiveWorld(0);

        // 分数滚动动画
        endScoreAnimTimer = std::min(0.35f, endScoreAnimTimer + deltaTime);
        animatedEndScore = (int)std::round(score * EaseOutCubic(endScoreAnimTimer / 0.35f));

        drawWorldWithCamera();
        drawScreenFX();
        drawUnifiedUI();

        if (IsKeyPressed(KEY_SPACE)) { resetGame(); currentState = PLAYING; return; }
        if (IsKeyPressed(KEY_ESCAPE)) { resetGame(); currentState = MENU; return; }
    }

public:
    GameManager() {
        srand((unsigned)time(nullptr));
        SetConfigFlags(FLAG_WINDOW_HIGHDPI | FLAG_MSAA_4X_HINT);
        InitWindow(GameConfig::GetWindowWidth(), GameConfig::GetWindowHeight(), "PlaneFight (raylib)");
        SetTargetFPS(60);
        SetExitKey(KEY_NULL);

        resourceManager.loadAllResources();

        // 初始化透视走廊参数
        perspectiveCfg.horizonY = (float)GameConfig::S(54);
        perspectiveCfg.bottomY = (float)(GameConfig::GetWindowHeight() - GameConfig::S(10));
        perspectiveCfg.laneHalfFar = (float)GameConfig::S(24);
        perspectiveCfg.laneHalfNear = (float)GameConfig::S(126);
        perspectiveMapper.setConfig(perspectiveCfg);

        initUI();
        initBackgroundLayers();
        chipMusic.init();

        // 设置 UI 漂移参数
        titleDrift.amp = 2; hudDrift.amp = 1.2f; microDrift.amp = 0.7f;
        titleDrift.retargetTimer = 0.45f; hudDrift.retargetTimer = 0.35f; microDrift.retargetTimer = 0.30f;

        resetGame();
    }

    ~GameManager() {
        clearEntities();
        chipMusic.shutdown();
        if (IsWindowReady()) CloseWindow();
    }

    void resetGame() {
        clearEntities();
        player = new Player(&resourceManager);
        player->updatePerspective(perspectiveCfg);
        score = 0;
        gameOver = false;
        shootCooldown = 0;
        hitStopTimer = 0;
        screenFlashAlpha = 0;
        cameraFX = CameraFXState();
        particleSystem.clear();
        pendingHitFX = false;
        endScoreAnimTimer = 0;
        animatedEndScore = 0;
        enemySpawnTimer = std::max(0.05f, enemySpawnRate / 60.0f);
    }

    // 游戏主循环
    void run() {
        while (!WindowShouldClose()) {
            deltaTime = ClampFloat(GetFrameTime(), 0.001f, 0.05f);
            uiTime += deltaTime;
            GraphicsEngine::setFXTime(uiTime);
            updateUiDrift(deltaTime);

            if (IsKeyPressed(KEY_M)) chipMusic.toggleMute();
            chipMusic.update(deltaTime);

            if (pauseCooldown > 0) pauseCooldown = std::max(0.0f, pauseCooldown - deltaTime);

            updateParallax(currentState == PAUSED ? deltaTime * 0.10f : deltaTime);
            updateCameraFX(deltaTime);
            layoutHUD();

            BeginDrawing();
            ClearBackground(BLACK);
            switch (currentState) {
                case MENU:    updateMenu();     break;
                case PLAYING: updatePlaying();  break;
                case PAUSED:  updatePaused();   break;
                case END:     updateGameOver(); break;
            }
            EndDrawing();
        }
    }
};

/* ==================== 程序入口 ==================== */
int main() {
    { GameManager game; game.run(); }
    return 0;
}
