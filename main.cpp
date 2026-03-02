/*
 * =====================================================================================
 *
 * Filename:  PlaneFight_Enterprise_Edition.cpp
 *
 * Description:  Space Shooter Game (Enterprise Extended Edition)
 * 这是一个基于 raylib 图形库和纯 C++ STL 链表实现的飞行射击游戏。
 * 包含完整的状态机（主菜单、游戏中、暂停、结束）、难度控制系统、
 * UI 交互系统以及透明贴图渲染系统。
 *
 * Compiler:  MSVC / CMake
 *
 * =====================================================================================
 */

#include <raylib.h>

#include <algorithm>
#include <cstdlib>
#include <ctime>
#include <list>
#include <string>
#include <unordered_set>
#include <vector>

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
const char* const END_HINT = u8"\u6309 [\u7A7A\u683C] \u91CD\u8BD5\uFF0C[ESC] \u9000\u51FA";
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

public:
    static void setUIFont(const Font* font, bool available);
    static void drawImageAlpha(int x, int y, const Texture2D* tex);
    static void outTextCenter(int x, int y, const char* str, int fontSize = 20, const char* fontName = "微软雅黑");
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

public:
    GameObject(double _x, double _y, int _w, int _h);
    virtual ~GameObject();

    virtual void draw() = 0;
    virtual bool move() = 0;

    double getX() const;
    double getY() const;
    int getWidth() const;
    int getHeight() const;
    bool getIsAlive() const;
    void setIsAlive(bool state);
};

class Bullet : public GameObject {
private:
    ResourceManager* resMgr;

public:
    Bullet(double _x, double _y, ResourceManager* _resMgr);
    virtual ~Bullet();

    virtual void draw() override;
    virtual bool move() override;
};

class EnemyBullet : public GameObject {
private:
    ResourceManager* resMgr;

public:
    EnemyBullet(double _x, double _y, ResourceManager* _resMgr);
    virtual ~EnemyBullet();

    virtual void draw() override;
    virtual bool move() override;
};

class Enemy : public GameObject {
private:
    ResourceManager* resMgr;

public:
    Enemy(double _x, double _y, ResourceManager* _resMgr);
    virtual ~Enemy();

    virtual void draw() override;
    virtual bool move() override;
};

class Player : public GameObject {
private:
    ResourceManager* resMgr;

public:
    Player(ResourceManager* _resMgr);
    virtual ~Player();

    virtual void draw() override;
    virtual bool move() override;

    int getCollisionX() const;
    int getCollisionY() const;
    int getCollisionWidth() const;
    int getCollisionHeight() const;
};

class Button {
private:
    int x;
    int y;
    int w;
    int h;
    string text;
    bool isHover;
    int fontSize;
    Color baseColor;
    string fontName;

public:
    Button();
    Button(int _x, int _y, int _w, int _h, string _text, int _fs, Color _c, string _fn);
    ~Button();

    void draw();
    bool update(float mouseX, float mouseY, bool mousePressed);

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
    long frameCount;
    int shootTimer;
    int pauseCooldown;

    int enemySpawnRate;
    int enemyShootChance;

    Button btnEasy;
    Button btnNormal;
    Button btnHell;
    Button btnPause;
    Button btnResume;
    Button btnMenu;

    void initUI();
    void clearEntities();
    void setDifficulty(int spawnRate, int shootChance);

    void updateMenu();
    void updatePlaying();
    void updatePaused();
    void updateGameOver();

    void processInput();
    void updateGameLogic();
    void renderGameScene();
    void drawBackground();

public:
    GameManager();
    ~GameManager();

    void resetGame();
    void run();
};

const Font* GraphicsEngine::uiFont = nullptr;
bool GraphicsEngine::hasUIFont = false;

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
    return (x1 < x2 + w2 && x1 + w1 > x2 &&
            y1 < y2 + h2 && y1 + h1 > y2);
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
        Vector2 pos = {(float)x - size.x / 2.0f, (float)y - size.y / 2.0f};
        DrawTextEx(*uiFont, str, pos, (float)fontSize, 1.0f, GameConfig::COLOR_TEXT);
        return;
    }

    int w = MeasureText(str, fontSize);
    DrawText(str, x - w / 2, y - fontSize / 2, fontSize, GameConfig::COLOR_TEXT);
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

    if (!FileExists(path)) {
        return false;
    }

    Image img = LoadImage(path);
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
        u8"\u6309 [\u7A7A\u683C] \u91CD\u8BD5\uFF0C[ESC] \u9000\u51FA"
        u8"SCORE: Final Score GAME OVER"
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

GameObject::GameObject(double _x, double _y, int _w, int _h) {
    this->position.set(_x, _y);
    this->width = _w;
    this->height = _h;
    this->isAlive = true;
}

GameObject::~GameObject() {}

double GameObject::getX() const { return this->position.getX(); }
double GameObject::getY() const { return this->position.getY(); }
int GameObject::getWidth() const { return this->width; }
int GameObject::getHeight() const { return this->height; }
bool GameObject::getIsAlive() const { return this->isAlive; }
void GameObject::setIsAlive(bool state) { this->isAlive = state; }

Bullet::Bullet(double _x, double _y, ResourceManager* _resMgr)
    : GameObject(_x, _y, GameConfig::S(8), GameConfig::S(24)), resMgr(_resMgr) {
}

Bullet::~Bullet() {}

void Bullet::draw() {
    if (this->resMgr->isBulletPlayerImageValid()) {
        GraphicsEngine::drawImageAlpha((int)this->position.getX(), (int)this->position.getY(), this->resMgr->getBulletPlayerImage());
    } else {
        DrawRectangle((int)this->position.getX(), (int)this->position.getY(), this->width, this->height, GameConfig::COLOR_BULLET);
    }
}

bool Bullet::move() {
    this->position.addY(-GameConfig::S(12));
    return this->position.getY() + this->height > 0;
}

EnemyBullet::EnemyBullet(double _x, double _y, ResourceManager* _resMgr)
    : GameObject(_x, _y, GameConfig::S(8), GameConfig::S(24)), resMgr(_resMgr) {
}

EnemyBullet::~EnemyBullet() {}

void EnemyBullet::draw() {
    if (this->resMgr->isBulletEnemyImageValid()) {
        GraphicsEngine::drawImageAlpha((int)this->position.getX(), (int)this->position.getY(), this->resMgr->getBulletEnemyImage());
    } else {
        DrawRectangle((int)this->position.getX(), (int)this->position.getY(), this->width, this->height, {255, 100, 100, 255});
    }
}

bool EnemyBullet::move() {
    this->position.addY(GameConfig::S(7));
    return this->position.getY() < GameConfig::GetWindowHeight();
}

Enemy::Enemy(double _x, double _y, ResourceManager* _resMgr)
    : GameObject(_x, _y, GameConfig::S(40), GameConfig::S(40)), resMgr(_resMgr) {
}

Enemy::~Enemy() {}

void Enemy::draw() {
    if (this->resMgr->isEnemyImageValid()) {
        GraphicsEngine::drawImageAlpha((int)this->position.getX(), (int)this->position.getY(), this->resMgr->getEnemyImage());
    } else {
        int px = (int)this->position.getX();
        int py = (int)this->position.getY();
        DrawTriangle({(float)px, (float)py}, {(float)(px + this->width), (float)py},
                     {(float)(px + this->width / 2), (float)(py + this->height)},
                     GameConfig::COLOR_ENEMY);
    }
}

bool Enemy::move() {
    this->position.addY(GameConfig::S(3));
    return this->position.getY() < GameConfig::GetWindowHeight();
}

Player::Player(ResourceManager* _resMgr)
    : GameObject(GameConfig::GetWindowWidth() / 2 - GameConfig::S(16),
                 GameConfig::GetWindowHeight() - GameConfig::S(55),
                 GameConfig::S(32),
                 GameConfig::S(32)),
      resMgr(_resMgr) {
}

Player::~Player() {}

void Player::draw() {
    if (this->resMgr->isPlayerImageValid()) {
        GraphicsEngine::drawImageAlpha((int)this->position.getX(), (int)this->position.getY(), this->resMgr->getPlayerImage());
    } else {
        int px = (int)this->position.getX();
        int py = (int)this->position.getY();

        Vector2 pts[4] = {
            {(float)(px + this->width / 2), (float)py},
            {(float)px, (float)(py + this->height)},
            {(float)(px + this->width / 2), (float)(py + this->height - GameConfig::S(6))},
            {(float)(px + this->width), (float)(py + this->height)}
        };

        DrawTriangleFan(pts, 4, GameConfig::COLOR_PLAYER);
        DrawCircle(px + this->width / 2, py + GameConfig::S(6), (float)GameConfig::S(2), WHITE);
    }
}

bool Player::move() {
    int winWidth = GameConfig::GetWindowWidth();
    int winHeight = GameConfig::GetWindowHeight();
    double moveSpeed = GameConfig::S(6);

    if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT)) {
        if (this->position.getX() > 0) this->position.addX(-moveSpeed);
    }
    if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT)) {
        if (this->position.getX() < winWidth - this->width) this->position.addX(moveSpeed);
    }
    if (IsKeyDown(KEY_W) || IsKeyDown(KEY_UP)) {
        if (this->position.getY() > 0) this->position.addY(-moveSpeed);
    }
    if (IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN)) {
        if (this->position.getY() < winHeight - this->height) this->position.addY(moveSpeed);
    }
    return true;
}

int Player::getCollisionX() const {
    return (int)this->position.getX() + GameConfig::S(6);
}

int Player::getCollisionY() const {
    return (int)this->position.getY() + GameConfig::S(6);
}

int Player::getCollisionWidth() const {
    return this->width - GameConfig::S(12);
}

int Player::getCollisionHeight() const {
    return this->height - GameConfig::S(12);
}

Button::Button()
    : x(0), y(0), w(0), h(0), text(""), isHover(false), fontSize(16), baseColor({255, 255, 255, 255}), fontName("微软雅黑") {
}

Button::Button(int _x, int _y, int _w, int _h, string _text, int _fs, Color _c, string _fn)
    : x(_x), y(_y), w(_w), h(_h), text(_text), isHover(false), fontSize(_fs), baseColor(_c), fontName(_fn) {
}

Button::~Button() {}

void Button::draw() {
    int cornerRadius = GameConfig::S(6);

    Rectangle shadowRect = {(float)(x + GameConfig::S(3)), (float)(y + GameConfig::S(3)), (float)w, (float)h};
    Rectangle mainRect = {(float)x, (float)y, (float)w, (float)h};

    float roundness = 0.0f;
    int minSide = std::min(w, h);
    if (minSide > 0) {
        roundness = (float)(cornerRadius * 2) / (float)minSide;
        roundness = std::min(1.0f, roundness);
    }

    DrawRectangleRounded(shadowRect, roundness, 8, {15, 15, 20, 255});

    int r = std::min(255, (int)this->baseColor.r + (this->isHover ? 50 : 0));
    int g = std::min(255, (int)this->baseColor.g + (this->isHover ? 50 : 0));
    int b = std::min(255, (int)this->baseColor.b + (this->isHover ? 50 : 0));

    DrawRectangleRounded(mainRect, roundness, 8, {(unsigned char)r, (unsigned char)g, (unsigned char)b, 255});

    Color borderColor = this->isHover
        ? Color{240, 240, 255, 255}
        : Color{(unsigned char)std::min(255, r + 30), (unsigned char)std::min(255, g + 30), (unsigned char)std::min(255, b + 30), 255};

    DrawRectangleRoundedLinesEx(mainRect, roundness, 8, (float)std::max(2, GameConfig::S(1)), borderColor);
    GraphicsEngine::outTextCenter(x + w / 2, y + h / 2, this->text.c_str(), this->fontSize, this->fontName.c_str());
}

bool Button::update(float mouseX, float mouseY, bool mousePressed) {
    this->isHover = MathUtils::isPointInRect(mouseX, mouseY, this->x, this->y, this->w, this->h);
    if (this->isHover && mousePressed) {
        return true;
    }
    return false;
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

GameManager::GameManager() {
    SetConfigFlags(FLAG_WINDOW_HIGHDPI | FLAG_MSAA_4X_HINT);
    InitWindow(GameConfig::GetWindowWidth(), GameConfig::GetWindowHeight(), "PlaneFight (raylib)");
    SetTargetFPS(60);
    SetExitKey(KEY_NULL);

    this->resourceManager.loadAllResources();
    this->initUI();

    this->currentState = MENU;
    this->enemySpawnRate = 30;
    this->enemyShootChance = 2;
    this->player = nullptr;

    this->resetGame();
}

GameManager::~GameManager() {
    this->clearEntities();
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

void GameManager::resetGame() {
    this->clearEntities();

    this->player = new Player(&this->resourceManager);
    this->score = 0;
    this->gameOver = false;
    this->frameCount = 0;
    this->shootTimer = 0;
    this->pauseCooldown = 0;

    srand((unsigned int)time(nullptr));
}

void GameManager::setDifficulty(int spawnRate, int shootChance) {
    this->enemySpawnRate = spawnRate;
    this->enemyShootChance = shootChance;
}

void GameManager::run() {
    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(GameConfig::COLOR_BG);

        switch (this->currentState) {
            case MENU:
                this->updateMenu();
                break;
            case PLAYING:
                this->updatePlaying();
                if (this->gameOver) {
                    this->currentState = END;
                }
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

void GameManager::drawBackground() {
    if (this->resourceManager.isBackgroundImageValid()) {
        GraphicsEngine::drawImageAlpha(0, 0, this->resourceManager.getBackgroundImage());
    } else {
        DrawRectangle(0, 0, GameConfig::GetWindowWidth(), GameConfig::GetWindowHeight(), GameConfig::COLOR_BG);

        for (int i = 0; i < GameConfig::GetWindowWidth(); i += GameConfig::S(40)) {
            DrawLine(i, 0, i, GameConfig::GetWindowHeight(), GameConfig::COLOR_GRID);
        }
        for (int i = 0; i < GameConfig::GetWindowHeight(); i += GameConfig::S(40)) {
            DrawLine(0, i, GameConfig::GetWindowWidth(), i, GameConfig::COLOR_GRID);
        }
    }
}

void GameManager::updateMenu() {
    this->drawBackground();

    int winW = GameConfig::GetWindowWidth();
    int winH = GameConfig::GetWindowHeight();

    GraphicsEngine::outTextCenter(winW / 2, GameConfig::S(40), Texts::MENU_TITLE, GameConfig::S(22), "ui");

    Vector2 mousePos = GetMousePosition();
    bool mousePressed = IsMouseButtonPressed(MOUSE_BUTTON_LEFT);

    bool easyClicked = this->btnEasy.update(mousePos.x, mousePos.y, mousePressed);
    bool normalClicked = this->btnNormal.update(mousePos.x, mousePos.y, mousePressed);
    bool hellClicked = this->btnHell.update(mousePos.x, mousePos.y, mousePressed);

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

    this->btnEasy.draw();
    this->btnNormal.draw();
    this->btnHell.draw();

    GraphicsEngine::outTextCenter(winW / 2, winH - GameConfig::S(15), Texts::DEVELOPERS, GameConfig::S(10), "ui");
}

void GameManager::updatePlaying() {
    this->drawBackground();

    Vector2 mousePos = GetMousePosition();
    bool mousePressed = IsMouseButtonPressed(MOUSE_BUTTON_LEFT);

    bool pauseClicked = this->btnPause.update(mousePos.x, mousePos.y, mousePressed);
    bool menuClicked = this->btnMenu.update(mousePos.x, mousePos.y, mousePressed);

    if (pauseClicked) {
        this->currentState = PAUSED;
        this->pauseCooldown = 15;
        return;
    }
    if (menuClicked) {
        this->currentState = MENU;
        this->resetGame();
        return;
    }

    this->processInput();

    if (this->currentState != PLAYING) {
        return;
    }

    this->updateGameLogic();
    this->renderGameScene();

    this->btnPause.draw();
    this->btnMenu.draw();

    this->frameCount++;
}

void GameManager::updatePaused() {
    this->drawBackground();
    this->renderGameScene();

    int winW = GameConfig::GetWindowWidth();
    int winH = GameConfig::GetWindowHeight();

    int boxX = winW / 2 - GameConfig::S(80);
    int boxY = winH / 2 - GameConfig::S(50);
    int boxW = GameConfig::S(160);
    int boxH = GameConfig::S(100);

    DrawRectangle(boxX, boxY, boxW, boxH, {30, 30, 35, 230});
    DrawRectangleLines(boxX, boxY, boxW, boxH, WHITE);

    GraphicsEngine::outTextCenter(winW / 2, winH / 2 - GameConfig::S(25), Texts::PAUSED_TITLE, GameConfig::S(20), "ui");
    GraphicsEngine::outTextCenter(winW / 2, winH / 2 - GameConfig::S(5), Texts::PAUSED_HINT, GameConfig::S(12), "ui");

    if (this->pauseCooldown > 0) {
        this->pauseCooldown--;
    }

    Vector2 mousePos = GetMousePosition();
    bool mousePressed = IsMouseButtonPressed(MOUSE_BUTTON_LEFT);

    bool resumeClicked = this->btnResume.update(mousePos.x, mousePos.y, mousePressed);
    this->btnPause.update(mousePos.x, mousePos.y, false);
    bool menuClicked = this->btnMenu.update(mousePos.x, mousePos.y, mousePressed);

    if (resumeClicked && this->pauseCooldown <= 0) {
        this->currentState = PLAYING;
        this->pauseCooldown = 15;
        return;
    }

    if (menuClicked) {
        this->currentState = MENU;
        this->resetGame();
        return;
    }

    this->btnResume.draw();
    this->btnPause.draw();
    this->btnMenu.draw();

    if (IsKeyPressed(KEY_P) && this->pauseCooldown <= 0) {
        this->currentState = PLAYING;
        this->pauseCooldown = 15;
    }
}

void GameManager::processInput() {
    if (this->player == nullptr) {
        return;
    }

    this->player->move();

    if (this->shootTimer > 0) {
        this->shootTimer--;
    }

    if (IsKeyDown(KEY_SPACE) && this->shootTimer <= 0) {
        double leftGunX = this->player->getX() + GameConfig::S(4);
        double rightGunX = this->player->getX() + this->player->getWidth() - GameConfig::S(12);
        double gunY = this->player->getY();

        this->bullets.push_back(new Bullet(leftGunX, gunY, &this->resourceManager));
        this->bullets.push_back(new Bullet(rightGunX, gunY, &this->resourceManager));

        this->shootTimer = 10;
    }

    if (this->pauseCooldown > 0) {
        this->pauseCooldown--;
    }

    if (IsKeyPressed(KEY_P) && this->pauseCooldown <= 0) {
        this->currentState = PAUSED;
        this->pauseCooldown = 15;
    }
}

void GameManager::updateGameLogic() {
    if (this->player == nullptr) {
        return;
    }

    if (this->enemySpawnRate > 0 && this->frameCount % this->enemySpawnRate == 0) {
        int randX = rand() % (GameConfig::GetWindowWidth() - GameConfig::S(40));
        this->enemies.push_back(new Enemy(randX, -GameConfig::S(40), &this->resourceManager));
    }

    for (auto it = this->bullets.begin(); it != this->bullets.end();) {
        if (!(*it)->move()) {
            delete *it;
            it = this->bullets.erase(it);
        } else {
            ++it;
        }
    }

    for (auto it = this->enemyBullets.begin(); it != this->enemyBullets.end();) {
        if (!(*it)->move()) {
            delete *it;
            it = this->enemyBullets.erase(it);
            continue;
        }

        if (MathUtils::isColliding((int)(*it)->getX(), (int)(*it)->getY(), (*it)->getWidth(), (*it)->getHeight(),
                                   this->player->getCollisionX(), this->player->getCollisionY(),
                                   this->player->getCollisionWidth(), this->player->getCollisionHeight())) {
            this->gameOver = true;
        }
        ++it;
    }

    for (auto it = this->enemies.begin(); it != this->enemies.end();) {
        bool isEnemyDestroyed = false;

        if (!(*it)->move()) {
            delete *it;
            it = this->enemies.erase(it);
            continue;
        }

        if (rand() % 100 < this->enemyShootChance) {
            double spawnX = (*it)->getX() + (*it)->getWidth() / 2 - GameConfig::S(4);
            double spawnY = (*it)->getY() + (*it)->getHeight();
            this->enemyBullets.push_back(new EnemyBullet(spawnX, spawnY, &this->resourceManager));
        }

        if (MathUtils::isColliding((int)(*it)->getX(), (int)(*it)->getY(), (*it)->getWidth(), (*it)->getHeight(),
                                   this->player->getCollisionX(), this->player->getCollisionY(),
                                   this->player->getCollisionWidth(), this->player->getCollisionHeight())) {
            this->gameOver = true;
        }

        for (auto bIt = this->bullets.begin(); bIt != this->bullets.end();) {
            if (MathUtils::isColliding((int)(*bIt)->getX(), (int)(*bIt)->getY(), (*bIt)->getWidth(), (*bIt)->getHeight(),
                                       (int)(*it)->getX(), (int)(*it)->getY(), (*it)->getWidth(), (*it)->getHeight())) {
                this->score += 10;
                isEnemyDestroyed = true;
                delete *bIt;
                bIt = this->bullets.erase(bIt);
                break;
            } else {
                ++bIt;
            }
        }

        if (isEnemyDestroyed) {
            delete *it;
            it = this->enemies.erase(it);
        } else {
            ++it;
        }
    }
}

void GameManager::renderGameScene() {
    if (this->player == nullptr) {
        return;
    }

    this->player->draw();

    for (auto b : this->bullets) {
        b->draw();
    }
    for (auto e : this->enemies) {
        e->draw();
    }
    for (auto eb : this->enemyBullets) {
        eb->draw();
    }

    string scoreText = "SCORE: " + to_string(this->score);
    GraphicsEngine::outTextCenter(GameConfig::S(45), GameConfig::S(15), scoreText.c_str(), GameConfig::S(16), "Consolas");
}

void GameManager::updateGameOver() {
    this->drawBackground();
    this->renderGameScene();

    int winW = GameConfig::GetWindowWidth();
    int winH = GameConfig::GetWindowHeight();

    int boxX = winW / 2 - GameConfig::S(110);
    int boxY = winH / 2 - GameConfig::S(70);
    int boxW = GameConfig::S(220);
    int boxH = GameConfig::S(140);

    DrawRectangle(boxX, boxY, boxW, boxH, BLACK);
    DrawRectangleLines(boxX, boxY, boxW, boxH, WHITE);

    const char* gameOverText = "GAME OVER";
    int gameOverFontSize = GameConfig::S(28);
    int textW = MeasureText(gameOverText, gameOverFontSize);
    DrawText(gameOverText, winW / 2 - textW / 2, winH / 2 - GameConfig::S(45), gameOverFontSize, {255, 50, 50, 255});

    string scoreMsg = "Final Score: " + to_string(this->score);
    GraphicsEngine::outTextCenter(winW / 2, winH / 2 + GameConfig::S(10), scoreMsg.c_str(), GameConfig::S(16), "ui");
    GraphicsEngine::outTextCenter(winW / 2, winH / 2 + GameConfig::S(40), Texts::END_HINT, GameConfig::S(12), "ui");

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

int main() {
    GameManager game;
    game.run();

    return 0;
}
