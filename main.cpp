/*
 * =====================================================================================
 *
 * Filename:  PlaneFight_Enterprise_Edition.cpp
 *
 * Description:  Space Shooter Game (Enterprise Extended Edition)
 * 这是一个基于 EasyX 图形库和纯 C++ STL 链表实现的飞行射击游戏。
 * 包含完整的状态机（主菜单、游戏中、暂停、结束）、难度控制系统、
 * UI 交互系统以及基于 alpha 通道的透明贴图渲染系统。
 *
 * Compiler:  MSVC / Visual Studio
 *
 * Author:  方恒康、石岩、耿乐、王智轩
 *
 * =====================================================================================
 */

#define _WIN32_WINNT 0x0600
#include <windows.h>
#include <graphics.h>  
#include <conio.h>
#include <list>
#include <string>
#include <vector>
#include <ctime>
#include <tchar.h>
#include <algorithm> 

 // 引入透明贴图所需的库，用于 AlphaBlend 函数
#pragma comment(lib, "msimg32.lib")

using namespace std;

 /**
  * @class GameConfig
  * @brief 游戏全局配置类，包含缩放系数、窗口尺寸等所有硬编码数据。
  */
class GameConfig {
public:
    // 缩放系统常数
    static const double SCALE;
    static const int BASE_WIDTH;
    static const int BASE_HEIGHT;

    // 颜色系统常数 (Palette)
    static const COLORREF COLOR_BG;
    static const COLORREF COLOR_GRID;
    static const COLORREF COLOR_PLAYER;
    static const COLORREF COLOR_ENEMY;
    static const COLORREF COLOR_BULLET;
    static const COLORREF COLOR_TEXT;

    /**
     * @brief 尺寸缩放转换器
     * @param v 基础尺寸值
     * @return int 经过 SCALE 缩放后的实际像素尺寸
     */
    static inline int S(double v) {
        return static_cast<int>(v * SCALE);
    }

    /**
     * @brief 获取窗口实际缩放后的宽度
     */
    static inline int GetWindowWidth() {
        return S(BASE_WIDTH);
    }

    /**
     * @brief 获取窗口实际缩放后的高度
     */
    static inline int GetWindowHeight() {
        return S(BASE_HEIGHT);
    }
};

// 静态成员变量初始化
const double GameConfig::SCALE = 3.4;
const int GameConfig::BASE_WIDTH = 256;
const int GameConfig::BASE_HEIGHT = 256;

const COLORREF GameConfig::COLOR_BG = RGB(30, 30, 35);
const COLORREF GameConfig::COLOR_GRID = RGB(50, 50, 60);
const COLORREF GameConfig::COLOR_PLAYER = RGB(50, 150, 250);
const COLORREF GameConfig::COLOR_ENEMY = RGB(230, 80, 80);
const COLORREF GameConfig::COLOR_BULLET = RGB(255, 200, 50);
const COLORREF GameConfig::COLOR_TEXT = RGB(240, 240, 240);



 /**
  * @class Vector2D
  * @brief 表示 2D 空间中的一个点或向量，提供基础的数学运算支持。
  */
class Vector2D {
private:
    double x;
    double y;

public:
    /**
     * @brief 默认构造函数，初始化为 (0,0)
     */
    Vector2D();

    /**
     * @brief 带参构造函数
     * @param _x X坐标
     * @param _y Y坐标
     */
    Vector2D(double _x, double _y);

    /**
     * @brief 析构函数
     */
    ~Vector2D();

    // Getters and Setters
    double getX() const;
    double getY() const;
    void setX(double _x);
    void setY(double _y);
    void set(double _x, double _y);

    // 数学运算辅助
    void addX(double dx);
    void addY(double dy);
};

/**
 * @class MathUtils
 * @brief 提供游戏中常用的碰撞检测和几何运算工具库。
 */
class MathUtils {
public:
    /**
     * @brief 矩形 AABB 碰撞检测
     * @param x1 第一个矩形左上角 X
     * @param y1 第一个矩形左上角 Y
     * @param w1 第一个矩形宽度
     * @param h1 第一个矩形高度
     * @param x2 第二个矩形左上角 X
     * @param y2 第二个矩形左上角 Y
     * @param w2 第二个矩形宽度
     * @param h2 第二个矩形高度
     * @return bool 如果发生重叠返回 true
     */
    static bool isColliding(int x1, int y1, int w1, int h1, int x2, int y2, int w2, int h2);

    /**
     * @brief 点与矩形的碰撞检测 (主要用于鼠标点击 UI)
     * @param px 点的 X 坐标
     * @param py 点的 Y 坐标
     * @param rx 矩形左上角 X
     * @param ry 矩形左上角 Y
     * @param rw 矩形宽度
     * @param rh 矩形高度
     * @return bool 如果点在矩形内部返回 true
     */
    static bool isPointInRect(int px, int py, int rx, int ry, int rw, int rh);

    /**
     * @brief 将数值限制在给定范围内
     * @param value 原始数值
     * @param minVal 最小值
     * @param maxVal 最大值
     * @return int 限制后的数值
     */
    static int clamp(int value, int minVal, int maxVal);
};



 /**
  * @class GraphicsEngine
  * @brief 封装 EasyX 图形库的基础绘制功能，提供更高层的绘制接口。
  */
class GraphicsEngine {
public:
    /**
     * @brief 绘制支持 Alpha 通道的透明 PNG 图片
     * @param x 绘制目标的 X 坐标
     * @param y 绘制目标的 Y 坐标
     * @param img 指向已加载 IMAGE 对象的指针
     */
    static void drawImageAlpha(int x, int y, IMAGE* img);

    /**
     * @brief 绘制居中对齐的文本，支持抗锯齿和自定义字体
     * @param x 文本中心的 X 坐标
     * @param y 文本中心的 Y 坐标
     * @param str 要绘制的文本字符串
     * @param fontSize 字体大小
     * @param fontName 字体名称 (如 "微软雅黑")
     */
    static void outTextCenter(int x, int y, const char* str, int fontSize = 20, const char* fontName = "微软雅黑");
};

/**
 * @class ResourceManager
 * @brief 管理游戏中所有的图片、音效等静态资源，实现统一加载和生命周期管理。
 */
class ResourceManager {
private:
    IMAGE imgPlayer;
    IMAGE imgEnemy;
    IMAGE imgBulletPlayer;
    IMAGE imgBulletEnemy;
    IMAGE imgBackground;

    bool hasImgPlayer;
    bool hasImgEnemy;
    bool hasImgBulletP;
    bool hasImgBulletE;
    bool hasImgBackground;

public:
    ResourceManager();
    ~ResourceManager();

    /**
     * @brief 加载所有必须的图片资源
     */
    void loadAllResources();

    // 资源获取接口
    IMAGE* getPlayerImage();
    IMAGE* getEnemyImage();
    IMAGE* getBulletPlayerImage();
    IMAGE* getBulletEnemyImage();
    IMAGE* getBackgroundImage();

    // 资源有效性检查接口
    bool isPlayerImageValid() const;
    bool isEnemyImageValid() const;
    bool isBulletPlayerImageValid() const;
    bool isBulletEnemyImageValid() const;
    bool isBackgroundImageValid() const;
};


 /**
  * @class GameObject
  * @brief 游戏世界中所有可交互实体的抽象基类。
  */
class GameObject {
protected:
    Vector2D position;  ///< 实体的二维坐标
    int width;          ///< 实体的宽度边界
    int height;         ///< 实体的高度边界
    bool isAlive;       ///< 实体的存活状态标志

public:
    /**
     * @brief 实体基类构造函数
     * @param _x 初始 X 坐标
     * @param _y 初始 Y 坐标
     * @param _w 实体宽度
     * @param _h 实体高度
     */
    GameObject(double _x, double _y, int _w, int _h);

    /**
     * @brief 实体基类虚析构函数，确保派生类内存安全
     */
    virtual ~GameObject();

    /**
     * @brief 纯虚函数：执行实体的绘制逻辑
     */
    virtual void draw() = 0;

    /**
     * @brief 纯虚函数：执行实体的物理位移和逻辑更新
     * @return bool 如果实体超出了屏幕边界或生命周期结束，应返回 false
     */
    virtual bool move() = 0;

    // Getters and Setters
    double getX() const;
    double getY() const;
    int getWidth() const;
    int getHeight() const;
    bool getIsAlive() const;
    void setIsAlive(bool state);
};

/**
 * @class Bullet
 * @brief 玩家发射的子弹实体类。
 */
class Bullet : public GameObject {
private:
    ResourceManager* resMgr; ///< 资源管理器指针

public:
    /**
     * @brief 玩家子弹构造函数
     * @param _x 初始生成 X 坐标
     * @param _y 初始生成 Y 坐标
     * @param _resMgr 资源管理器引用
     */
    Bullet(double _x, double _y, ResourceManager* _resMgr);
    virtual ~Bullet();

    virtual void draw() override;
    virtual bool move() override;
};

/**
 * @class EnemyBullet
 * @brief 敌机发射的子弹实体类。
 */
class EnemyBullet : public GameObject {
private:
    ResourceManager* resMgr;

public:
    /**
     * @brief 敌机子弹构造函数
     * @param _x 初始生成 X 坐标
     * @param _y 初始生成 Y 坐标
     * @param _resMgr 资源管理器引用
     */
    EnemyBullet(double _x, double _y, ResourceManager* _resMgr);
    virtual ~EnemyBullet();

    virtual void draw() override;
    virtual bool move() override;
};

/**
 * @class Enemy
 * @brief 敌机实体类，负责向下移动和渲染。
 */
class Enemy : public GameObject {
private:
    ResourceManager* resMgr;

public:
    /**
     * @brief 敌机构造函数
     * @param _x 初始生成 X 坐标
     * @param _y 初始生成 Y 坐标
     * @param _resMgr 资源管理器引用
     */
    Enemy(double _x, double _y, ResourceManager* _resMgr);
    virtual ~Enemy();

    virtual void draw() override;
    virtual bool move() override;
};

/**
 * @class Player
 * @brief 玩家操控的飞机实体类。接收键盘输入进行二维移动。
 */
class Player : public GameObject {
private:
    ResourceManager* resMgr;

public:
    /**
     * @brief 玩家构造函数
     * @param _resMgr 资源管理器引用
     */
    Player(ResourceManager* _resMgr);
    virtual ~Player();

    virtual void draw() override;
    virtual bool move() override;

    /**
     * @brief 获取玩家用于碰撞判定的精准边界 X
     */
    int getCollisionX() const;

    /**
     * @brief 获取玩家用于碰撞判定的精准边界 Y
     */
    int getCollisionY() const;

    /**
     * @brief 获取玩家用于碰撞判定的精准边界宽度
     */
    int getCollisionWidth() const;

    /**
     * @brief 获取玩家用于碰撞判定的精准边界高度
     */
    int getCollisionHeight() const;
};



 /**
  * @class Button
  * @brief UI 按钮组件类，支持悬停特效、自定义字体和圆角渲染。
  */
class Button {
private:
    int x;                  ///< 按钮左上角 X
    int y;                  ///< 按钮左上角 Y
    int w;                  ///< 按钮宽度
    int h;                  ///< 按钮高度
    string text;            ///< 按钮显示的文本内容
    bool isHover;           ///< 鼠标是否悬停在按钮上方
    int fontSize;           ///< 文本字体大小
    COLORREF baseColor;     ///< 按钮的基础背景颜色
    string fontName;        ///< 文本字体名称

public:
    /**
     * @brief 默认构造函数
     */
    Button();

    /**
     * @brief 完整的带参构造函数
     */
    Button(int _x, int _y, int _w, int _h, string _text, int _fs, COLORREF _c, string _fn);

    /**
     * @brief 析构函数
     */
    ~Button();

    /**
     * @brief 渲染按钮本身、阴影和发光边框
     */
    void draw();

    /**
     * @brief 更新按钮状态，检测鼠标悬停和点击事件
     * @param msg EasyX 获取到的鼠标消息结构体
     * @return bool 如果按钮被点击，返回 true
     */
    bool update(const MOUSEMSG& msg);

    // Getters and Setters
    void setPosition(int _x, int _y);
    void setSize(int _w, int _h);
    void setText(const string& str);
    bool getHoverState() const;
};



 /**
  * @enum GameState
  * @brief 描述游戏的四大主状态
  */
enum GameState {
    MENU,       ///< 处于主菜单状态
    PLAYING,    ///< 正在游戏进程中
    PAUSED,     ///< 游戏被暂停
    END         ///< 游戏结束（结算画面）
};

/**
 * @class GameManager
 * @brief 游戏的核心控制器。负责状态机流转、实体链表管理、事件派发和主循环。
 */
class GameManager {
private:
    // --- 核心系统组件 ---
    ResourceManager resourceManager;    ///< 全局资源管理器实例
    GameState currentState;             ///< 当前游戏运行状态

    // --- 游戏实体链表 ---
    Player* player;                     ///< 玩家实体对象指针
    list<Bullet*> bullets;              ///< 玩家发射的子弹实体链表
    list<Enemy*> enemies;               ///< 敌机实体链表
    list<EnemyBullet*> enemyBullets;    ///< 敌机发射的子弹实体链表

    // --- 游戏逻辑状态数据 ---
    int score;                          ///< 玩家当前得分
    bool gameOver;                      ///< 是否触发了游戏结束标志
    long frameCount;                    ///< 全局逻辑帧计数器
    int shootTimer;                     ///< 玩家射击冷却计时器
    int pauseCooldown;                  ///< 暂停按键的防抖冷却计时器

    // --- 动态难度控制变量 ---
    int enemySpawnRate;                 ///< 生成敌机的频率（帧数间隔）
    int enemyShootChance;               ///< 敌机发射子弹的概率（百分比）

    // --- UI 界面组件 ---
    Button btnEasy;                     ///< 主菜单：简单模式按钮
    Button btnNormal;                   ///< 主菜单：普通模式按钮
    Button btnHell;                     ///< 主菜单：地狱模式按钮
    Button btnPause;                    ///< 游戏中：暂停按钮
    Button btnResume;                   ///< 暂停菜单：继续按钮
    Button btnMenu;                     ///< 游戏中：返回菜单按钮

    // --- 内部辅助方法 ---
    void initUI();                      ///< 初始化配置所有 UI 组件的位置和大小
    void clearEntities();               ///< 清空当前场景中所有的动态实体，释放内存
    void setDifficulty(int spawnRate, int shootChance); ///< 设定动态难度参数

    // --- 状态机分解方法 ---
    void updateMenu();                  ///< 处理主菜单的逻辑和渲染
    void updatePlaying();               ///< 处理游戏进行中的逻辑和渲染
    void updatePaused();                ///< 处理暂停菜单的逻辑和渲染
    void updateGameOver();              ///< 处理游戏结束画面的逻辑和渲染

    void processInput();                ///< 处理按键输入事件
    void updateGameLogic();             ///< 更新实体位移、碰撞检测和生成逻辑
    void renderGameScene();             ///< 绘制背景和所有游戏实体
    void drawBackground();              ///< 绘制动态网格或贴图背景

public:
    /**
     * @brief 游戏管理器构造函数，初始化窗口和基本系统
     */
    GameManager();

    /**
     * @brief 游戏管理器析构函数，负责关闭窗口和内存泄露清理
     */
    ~GameManager();

    /**
     * @brief 重置游戏核心数据，为下一局做准备
     */
    void resetGame();

    /**
     * @brief 启动游戏主循环。阻塞调用，直到游戏退出
     */
    void run();
};



 // ----------------------------------------------------------------------------------
 // Vector2D Implementation
 // ----------------------------------------------------------------------------------
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

bool MathUtils::isPointInRect(int px, int py, int rx, int ry, int rw, int rh) {
    return (px >= rx && px <= rx + rw && py >= ry && py <= ry + rh);
}

int MathUtils::clamp(int value, int minVal, int maxVal) {
    if (value < minVal) return minVal;
    if (value > maxVal) return maxVal;
    return value;
}



void GraphicsEngine::drawImageAlpha(int x, int y, IMAGE* img) {
    if (!img || img->getwidth() == 0) return;
    HDC dstDC = GetImageHDC();
    HDC srcDC = GetImageHDC(img);
    int w = img->getwidth();
    int h = img->getheight();
    BLENDFUNCTION bf = { AC_SRC_OVER, 0, 255, AC_SRC_ALPHA };
    AlphaBlend(dstDC, x, y, w, h, srcDC, 0, 0, w, h, bf);
}

void GraphicsEngine::outTextCenter(int x, int y, const char* str, int fontSize, const char* fontName) {
    LOGFONT f;
    gettextstyle(&f);
    f.lfHeight = fontSize;
    f.lfQuality = ANTIALIASED_QUALITY;

#ifdef UNICODE
    int fnLen = MultiByteToWideChar(CP_ACP, 0, fontName, -1, NULL, 0);
    wchar_t* wFontName = new wchar_t[fnLen];
    MultiByteToWideChar(CP_ACP, 0, fontName, -1, wFontName, fnLen);
    _tcscpy_s(f.lfFaceName, wFontName);
    delete[] wFontName;
#else
    _tcscpy_s(f.lfFaceName, fontName);
#endif

    settextstyle(&f);

#ifdef UNICODE
    int len = MultiByteToWideChar(CP_ACP, 0, str, -1, NULL, 0);
    wchar_t* wstr = new wchar_t[len];
    MultiByteToWideChar(CP_ACP, 0, str, -1, wstr, len);
    int w = textwidth(wstr);
    int h = textheight(wstr);
    outtextxy(x - w / 2, y - h / 2, wstr);
    delete[] wstr;
#else
    int w = textwidth(str);
    int h = textheight(str);
    outtextxy(x - w / 2, y - h / 2, str);
#endif
}


ResourceManager::ResourceManager() {
    this->hasImgPlayer = false;
    this->hasImgEnemy = false;
    this->hasImgBulletP = false;
    this->hasImgBulletE = false;
    this->hasImgBackground = false;
}

ResourceManager::~ResourceManager() {
    // IMAGE objects in EasyX clean up their own handles.
}

void ResourceManager::loadAllResources() {
    // 背景图
    loadimage(&this->imgBackground, _T("purple.png"), GameConfig::GetWindowWidth(), GameConfig::GetWindowHeight(), true);
    this->hasImgBackground = (this->imgBackground.getwidth() > 0);

    // 玩家飞机 (尺寸: 32x32)
    loadimage(&this->imgPlayer, _T("playerShip_blue.png"), GameConfig::S(32), GameConfig::S(32), true);
    this->hasImgPlayer = (this->imgPlayer.getwidth() > 0);

    // 敌机 (尺寸: 40x40)
    loadimage(&this->imgEnemy, _T("playerShip_red.png"), GameConfig::S(40), GameConfig::S(40), true);
    this->hasImgEnemy = (this->imgEnemy.getwidth() > 0);

    // 玩家子弹 (尺寸: 8x24)
    loadimage(&this->imgBulletPlayer, _T("laserBlue.png"), GameConfig::S(8), GameConfig::S(24), true);
    this->hasImgBulletP = (this->imgBulletPlayer.getwidth() > 0);

    // 敌机子弹 (尺寸: 8x24)
    loadimage(&this->imgBulletEnemy, _T("laserRed.png"), GameConfig::S(8), GameConfig::S(24), true);
    this->hasImgBulletE = (this->imgBulletEnemy.getwidth() > 0);
}

IMAGE* ResourceManager::getPlayerImage() { return &this->imgPlayer; }
IMAGE* ResourceManager::getEnemyImage() { return &this->imgEnemy; }
IMAGE* ResourceManager::getBulletPlayerImage() { return &this->imgBulletPlayer; }
IMAGE* ResourceManager::getBulletEnemyImage() { return &this->imgBulletEnemy; }
IMAGE* ResourceManager::getBackgroundImage() { return &this->imgBackground; }

bool ResourceManager::isPlayerImageValid() const { return this->hasImgPlayer; }
bool ResourceManager::isEnemyImageValid() const { return this->hasImgEnemy; }
bool ResourceManager::isBulletPlayerImageValid() const { return this->hasImgBulletP; }
bool ResourceManager::isBulletEnemyImageValid() const { return this->hasImgBulletE; }
bool ResourceManager::isBackgroundImageValid() const { return this->hasImgBackground; }


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
    }
    else {
        setfillcolor(GameConfig::COLOR_BULLET);
        solidrectangle((int)this->position.getX(), (int)this->position.getY(),
            (int)this->position.getX() + this->width, (int)this->position.getY() + this->height);
    }
}

bool Bullet::move() {
    this->position.addY(-GameConfig::S(12)); // 玩家子弹向上，按比例提速
    return this->position.getY() + this->height > 0;
}


EnemyBullet::EnemyBullet(double _x, double _y, ResourceManager* _resMgr)
    : GameObject(_x, _y, GameConfig::S(8), GameConfig::S(24)), resMgr(_resMgr) {
}

EnemyBullet::~EnemyBullet() {}

void EnemyBullet::draw() {
    if (this->resMgr->isBulletEnemyImageValid()) {
        GraphicsEngine::drawImageAlpha((int)this->position.getX(), (int)this->position.getY(), this->resMgr->getBulletEnemyImage());
    }
    else {
        setfillcolor(RGB(255, 100, 100)); // 红色备用激光
        solidrectangle((int)this->position.getX(), (int)this->position.getY(),
            (int)this->position.getX() + this->width, (int)this->position.getY() + this->height);
    }
}

bool EnemyBullet::move() {
    this->position.addY(GameConfig::S(7)); // 敌机子弹向下
    return this->position.getY() < GameConfig::GetWindowHeight();
}


Enemy::Enemy(double _x, double _y, ResourceManager* _resMgr)
    : GameObject(_x, _y, GameConfig::S(40), GameConfig::S(40)), resMgr(_resMgr) {
}

Enemy::~Enemy() {}

void Enemy::draw() {
    if (this->resMgr->isEnemyImageValid()) {
        GraphicsEngine::drawImageAlpha((int)this->position.getX(), (int)this->position.getY(), this->resMgr->getEnemyImage());
    }
    else {
        setfillcolor(GameConfig::COLOR_ENEMY);
        int px = (int)this->position.getX();
        int py = (int)this->position.getY();
        POINT pts[] = {
            {px, py},
            {px + this->width, py},
            {px + this->width / 2, py + this->height}
        };
        solidpolygon(pts, 3);
    }
}

bool Enemy::move() {
    this->position.addY(GameConfig::S(3));
    return this->position.getY() < GameConfig::GetWindowHeight();
}


Player::Player(ResourceManager* _resMgr)
    : GameObject(GameConfig::GetWindowWidth() / 2 - GameConfig::S(16),
        GameConfig::GetWindowHeight() - GameConfig::S(55),
        GameConfig::S(32), GameConfig::S(32)),
    resMgr(_resMgr) {
}

Player::~Player() {}

void Player::draw() {
    if (this->resMgr->isPlayerImageValid()) {
        GraphicsEngine::drawImageAlpha((int)this->position.getX(), (int)this->position.getY(), this->resMgr->getPlayerImage());
    }
    else {
        setfillcolor(GameConfig::COLOR_PLAYER);
        int px = (int)this->position.getX();
        int py = (int)this->position.getY();
        POINT pts[] = {
            {px + this->width / 2, py},
            {px, py + this->height},
            {px + this->width / 2, py + this->height - GameConfig::S(6)},
            {px + this->width, py + this->height}
        };
        solidpolygon(pts, 4);
        setfillcolor(WHITE);
        solidcircle(px + this->width / 2, py + GameConfig::S(6), GameConfig::S(2));
    }
}

bool Player::move() {
    int winWidth = GameConfig::GetWindowWidth();
    int winHeight = GameConfig::GetWindowHeight();
    double moveSpeed = GameConfig::S(6);

    if (GetAsyncKeyState('A') || GetAsyncKeyState(VK_LEFT)) {
        if (this->position.getX() > 0) this->position.addX(-moveSpeed);
    }
    if (GetAsyncKeyState('D') || GetAsyncKeyState(VK_RIGHT)) {
        if (this->position.getX() < winWidth - this->width) this->position.addX(moveSpeed);
    }
    if (GetAsyncKeyState('W') || GetAsyncKeyState(VK_UP)) {
        if (this->position.getY() > 0) this->position.addY(-moveSpeed);
    }
    if (GetAsyncKeyState('S') || GetAsyncKeyState(VK_DOWN)) {
        if (this->position.getY() < winHeight - this->height) this->position.addY(moveSpeed);
    }
    return true; // 玩家实体只要活着就在场上，永远返回 true
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
    : x(0), y(0), w(0), h(0), text(""), isHover(false), fontSize(16), baseColor(RGB(255, 255, 255)), fontName("微软雅黑") {
}

Button::Button(int _x, int _y, int _w, int _h, string _text, int _fs, COLORREF _c, string _fn)
    : x(_x), y(_y), w(_w), h(_h), text(_text), isHover(false), fontSize(_fs), baseColor(_c), fontName(_fn) {
}

Button::~Button() {}

void Button::draw() {
    int cornerRadius = GameConfig::S(6);

    // 1. 绘制阴影层
    setfillcolor(RGB(15, 15, 20));
    solidroundrect(x + GameConfig::S(3), y + GameConfig::S(3), x + w + GameConfig::S(3), y + h + GameConfig::S(3), cornerRadius, cornerRadius);

    // 2. 绘制基础主体 (悬停时提亮 RGB 数值)
    int r = min(255, GetRValue(this->baseColor) + (this->isHover ? 50 : 0));
    int g = min(255, GetGValue(this->baseColor) + (this->isHover ? 50 : 0));
    int b = min(255, GetBValue(this->baseColor) + (this->isHover ? 50 : 0));
    setfillcolor(RGB(r, g, b));
    solidroundrect(x, y, x + w, y + h, cornerRadius, cornerRadius);

    // 3. 绘制外边缘发光线
    setlinecolor(this->isHover ? RGB(240, 240, 255) : RGB(r + 30, g + 30, b + 30));
    setlinestyle(PS_SOLID, max(2, GameConfig::S(1)));
    roundrect(x, y, x + w, y + h, cornerRadius, cornerRadius);
    setlinestyle(PS_SOLID, 1);

    // 4. 绘制内部文字
    setbkmode(TRANSPARENT);
    settextcolor(WHITE);
    GraphicsEngine::outTextCenter(x + w / 2, y + h / 2, this->text.c_str(), this->fontSize, this->fontName.c_str());
}

bool Button::update(const MOUSEMSG& msg) {
    this->isHover = MathUtils::isPointInRect(msg.x, msg.y, this->x, this->y, this->w, this->h);
    if (this->isHover && msg.uMsg == WM_LBUTTONDOWN) {
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
    // 建立窗口系统
    initgraph(GameConfig::GetWindowWidth(), GameConfig::GetWindowHeight());
    setbkcolor(GameConfig::COLOR_BG);

    // 初始化全局资源
    this->resourceManager.loadAllResources();

    // 构建所有UI实体
    this->initUI();

    // 设定初始游玩状态
    this->currentState = MENU;
    this->enemySpawnRate = 30;
    this->enemyShootChance = 2;
    this->player = nullptr;

    // 清理和重启逻辑参数
    this->resetGame();
}

GameManager::~GameManager() {
    closegraph();
    this->clearEntities();
}

void GameManager::initUI() {
    int winW = GameConfig::GetWindowWidth();
    int winH = GameConfig::GetWindowHeight();

    int btnW = GameConfig::S(96);
    int btnH = GameConfig::S(28);
    int btnX = winW / 2 - btnW / 2;

    this->btnEasy = Button(btnX, GameConfig::S(95), btnW, btnH, "简单模式", GameConfig::S(13), RGB(40, 160, 80), "微软雅黑");
    this->btnNormal = Button(btnX, GameConfig::S(135), btnW, btnH, "普通模式", GameConfig::S(13), RGB(50, 120, 210), "微软雅黑");
    this->btnHell = Button(btnX, GameConfig::S(175), btnW, btnH, "地狱模式", GameConfig::S(13), RGB(180, 50, 50), "微软雅黑");

    this->btnPause = Button(winW - GameConfig::S(45), GameConfig::S(5), GameConfig::S(40), GameConfig::S(20), "暂停", GameConfig::S(12), RGB(50, 120, 210), "微软雅黑");
    this->btnMenu = Button(winW - GameConfig::S(90), GameConfig::S(5), GameConfig::S(40), GameConfig::S(20), "菜单", GameConfig::S(12), RGB(200, 80, 80), "微软雅黑");

    this->btnResume = Button(winW / 2 - GameConfig::S(45), winH / 2 + GameConfig::S(10), GameConfig::S(90), GameConfig::S(28), "点击继续", GameConfig::S(14), RGB(50, 120, 210), "微软雅黑");
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

    srand((unsigned int)time(0));
}

void GameManager::setDifficulty(int spawnRate, int shootChance) {
    this->enemySpawnRate = spawnRate;
    this->enemyShootChance = shootChance;
}

void GameManager::run() {
    BeginBatchDraw();

    while (true) {
        cleardevice();

        // 简化的顶层状态机路由
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
            if (this->currentState == PLAYING) {
                this->resetGame(); // 捕获到状态转换，立刻重置
            }
            if (this->currentState == MENU) {
                break; // 如果在结束画面按下ESC退出到菜单
            }
            break;
        }

        // 检测底层菜单返回指令的提前退出
        if (this->currentState == MENU && this->gameOver == true) {
            // Placeholder: 如果需要在菜单强退可以拓展
        }

        FlushBatchDraw();
        Sleep(16); // 保证约 60 FPS
    }

    EndBatchDraw();
}

void GameManager::drawBackground() {
    if (this->resourceManager.isBackgroundImageValid()) {
        GraphicsEngine::drawImageAlpha(0, 0, this->resourceManager.getBackgroundImage());
    }
    else {
        setfillcolor(GameConfig::COLOR_BG);
        solidrectangle(0, 0, GameConfig::GetWindowWidth(), GameConfig::GetWindowHeight());
        setlinecolor(GameConfig::COLOR_GRID);
        for (int i = 0; i < GameConfig::GetWindowWidth(); i += GameConfig::S(40)) {
            line(i, 0, i, GameConfig::GetWindowHeight());
        }
        for (int i = 0; i < GameConfig::GetWindowHeight(); i += GameConfig::S(40)) {
            line(0, i, GameConfig::GetWindowWidth(), i);
        }
    }
}

void GameManager::updateMenu() {
    this->drawBackground();
    setbkmode(TRANSPARENT);

    int winW = GameConfig::GetWindowWidth();
    int winH = GameConfig::GetWindowHeight();

    // 绘制街机感标题阴影及本体
    settextcolor(RGB(20, 20, 20));
    GraphicsEngine::outTextCenter(winW / 2 + GameConfig::S(2), GameConfig::S(42), "基于链表的飞机大战游戏", GameConfig::S(22), "华文琥珀");
    settextcolor(GameConfig::COLOR_TEXT);
    GraphicsEngine::outTextCenter(winW / 2, GameConfig::S(40), "基于链表的飞机大战游戏", GameConfig::S(22), "华文琥珀");

    MOUSEMSG msg = { 0 };
    while (MouseHit()) {
        msg = GetMouseMsg();

        if (this->btnEasy.update(msg)) {
            this->setDifficulty(60, 1);
            this->currentState = PLAYING;
            FlushMouseMsgBuffer();
            break;
        }
        if (this->btnNormal.update(msg)) {
            this->setDifficulty(30, 2);
            this->currentState = PLAYING;
            FlushMouseMsgBuffer();
            break;
        }
        if (this->btnHell.update(msg)) {
            this->setDifficulty(10, 6);
            this->currentState = PLAYING;
            FlushMouseMsgBuffer();
            break;
        }
    }

    // 绘制界面可交互元素
    this->btnEasy.draw();
    this->btnNormal.draw();
    this->btnHell.draw();

    // 绘制底部开发者声明
    settextcolor(RGB(150, 150, 150));
    GraphicsEngine::outTextCenter(winW / 2, winH - GameConfig::S(15), "开发者：方恒康、石岩、耿乐、王智轩", GameConfig::S(10), "幼圆");
}

void GameManager::updatePlaying() {
    this->drawBackground();

    MOUSEMSG msg = { 0 };
    while (MouseHit()) {
        msg = GetMouseMsg();
        if (this->btnPause.update(msg)) {
            this->currentState = PAUSED;
            this->pauseCooldown = 15;
            FlushMouseMsgBuffer();
            return;
        }
        if (this->btnMenu.update(msg)) {
            this->currentState = MENU;
            this->resetGame();
            FlushMouseMsgBuffer();
            return;
        }
    }

    this->processInput();
    this->updateGameLogic();
    this->renderGameScene();

    // 绘制系统悬浮按钮
    this->btnPause.draw();
    this->btnMenu.draw();

    this->frameCount++;
}

void GameManager::updatePaused() {
    this->drawBackground();
    this->renderGameScene();

    int winW = GameConfig::GetWindowWidth();
    int winH = GameConfig::GetWindowHeight();

    // 绘制暗色半透明模态框
    setfillcolor(GameConfig::COLOR_BG);
    solidrectangle(winW / 2 - GameConfig::S(80), winH / 2 - GameConfig::S(50), winW / 2 + GameConfig::S(80), winH / 2 + GameConfig::S(50));
    setlinecolor(WHITE);
    rectangle(winW / 2 - GameConfig::S(80), winH / 2 - GameConfig::S(50), winW / 2 + GameConfig::S(80), winH / 2 + GameConfig::S(50));

    setbkmode(TRANSPARENT);
    settextcolor(WHITE);
    GraphicsEngine::outTextCenter(winW / 2, winH / 2 - GameConfig::S(25), "游戏暂停", GameConfig::S(20), "微软雅黑");
    GraphicsEngine::outTextCenter(winW / 2, winH / 2 - GameConfig::S(5), "或按 [P] 键继续", GameConfig::S(12), "微软雅黑");

    if (this->pauseCooldown > 0) {
        this->pauseCooldown--;
    }

    MOUSEMSG msg = { 0 };
    while (MouseHit()) {
        msg = GetMouseMsg();
        if (this->btnResume.update(msg) && this->pauseCooldown <= 0) {
            this->currentState = PLAYING;
            this->pauseCooldown = 15;
            FlushMouseMsgBuffer();
            return;
        }
        if (this->btnMenu.update(msg)) {
            this->currentState = MENU;
            this->resetGame();
            FlushMouseMsgBuffer();
            return;
        }
    }

    // 刷新模态框上的按钮
    this->btnResume.draw();
    this->btnPause.draw();
    this->btnMenu.draw();

    // 处理键盘快捷恢复逻辑
    if (GetAsyncKeyState('P') && this->pauseCooldown <= 0) {
        this->currentState = PLAYING;
        this->pauseCooldown = 15;
    }
}

void GameManager::processInput() {
    // 玩家位移委托
    this->player->move();

    // 处理射击逻辑与冷却
    if (this->shootTimer > 0) {
        this->shootTimer--;
    }

    if (GetAsyncKeyState(VK_SPACE) && this->shootTimer <= 0) {
        // 双联发设计，根据玩家缩放尺寸偏移生成子弹
        double leftGunX = this->player->getX() + GameConfig::S(4);
        double rightGunX = this->player->getX() + this->player->getWidth() - GameConfig::S(12);
        double gunY = this->player->getY();

        this->bullets.push_back(new Bullet(leftGunX, gunY, &this->resourceManager));
        this->bullets.push_back(new Bullet(rightGunX, gunY, &this->resourceManager));

        this->shootTimer = 10;
    }

    // 处理快捷键暂停防抖逻辑
    if (this->pauseCooldown > 0) {
        this->pauseCooldown--;
    }
    if (GetAsyncKeyState('P') && this->pauseCooldown <= 0) {
        this->currentState = PAUSED;
        this->pauseCooldown = 15;
    }
}

void GameManager::updateGameLogic() {
    // 1. 生成敌机
    if (this->frameCount % this->enemySpawnRate == 0) {
        int randX = rand() % (GameConfig::GetWindowWidth() - GameConfig::S(40));
        this->enemies.push_back(new Enemy(randX, -GameConfig::S(40), &this->resourceManager));
    }

    // 2. 玩家子弹位移与销毁检查
    for (auto it = this->bullets.begin(); it != this->bullets.end(); ) {
        if (!(*it)->move()) {
            delete* it;
            it = this->bullets.erase(it);
        }
        else {
            ++it;
        }
    }

    // 3. 敌机子弹位移与玩家受击检测
    for (auto it = this->enemyBullets.begin(); it != this->enemyBullets.end(); ) {
        if (!(*it)->move()) {
            delete* it;
            it = this->enemyBullets.erase(it);
            continue;
        }

        // 使用玩家暴露的碰撞 API
        if (MathUtils::isColliding((int)(*it)->getX(), (int)(*it)->getY(), (*it)->getWidth(), (*it)->getHeight(),
            this->player->getCollisionX(), this->player->getCollisionY(),
            this->player->getCollisionWidth(), this->player->getCollisionHeight())) {
            this->gameOver = true;
        }
        ++it;
    }

    // 4. 敌机本体位移、射击概率处理与双向碰撞检测
    for (auto it = this->enemies.begin(); it != this->enemies.end(); ) {
        bool isEnemyDestroyed = false;

        // 向下移动判定边界
        if (!(*it)->move()) {
            delete* it;
            it = this->enemies.erase(it);
            continue;
        }

        // 随机开火逻辑
        if (rand() % 100 < this->enemyShootChance) {
            double spawnX = (*it)->getX() + (*it)->getWidth() / 2 - GameConfig::S(4);
            double spawnY = (*it)->getY() + (*it)->getHeight();
            this->enemyBullets.push_back(new EnemyBullet(spawnX, spawnY, &this->resourceManager));
        }

        // 玩家与敌机发生神风碰撞
        if (MathUtils::isColliding((int)(*it)->getX(), (int)(*it)->getY(), (*it)->getWidth(), (*it)->getHeight(),
            this->player->getCollisionX(), this->player->getCollisionY(),
            this->player->getCollisionWidth(), this->player->getCollisionHeight())) {
            this->gameOver = true;
        }

        // 子弹攻击判定
        for (auto bIt = this->bullets.begin(); bIt != this->bullets.end(); ) {
            if (MathUtils::isColliding((int)(*bIt)->getX(), (int)(*bIt)->getY(), (*bIt)->getWidth(), (*bIt)->getHeight(),
                (int)(*it)->getX(), (int)(*it)->getY(), (*it)->getWidth(), (*it)->getHeight())) {
                this->score += 10;
                isEnemyDestroyed = true;
                delete* bIt;
                bIt = this->bullets.erase(bIt);
                break; // 击中一次后跳出内层循环
            }
            else {
                ++bIt;
            }
        }

        // 处理敌机阵亡生命周期
        if (isEnemyDestroyed) {
            delete* it;
            it = this->enemies.erase(it);
        }
        else {
            ++it;
        }
    }
}

void GameManager::renderGameScene() {
    // 从底层到高层逐个调用实体的虚函数 draw()
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

    // 左上角积分计分板渲染
    setbkmode(TRANSPARENT);
    settextcolor(GameConfig::COLOR_TEXT);
    string scoreText = "SCORE: " + to_string(this->score);

    GraphicsEngine::outTextCenter(GameConfig::S(45), GameConfig::S(15), scoreText.c_str(), GameConfig::S(16), "Consolas");
}

void GameManager::updateGameOver() {
    int winW = GameConfig::GetWindowWidth();
    int winH = GameConfig::GetWindowHeight();

    // 绘制纯黑高压感模态边框
    setfillcolor(RGB(0, 0, 0));
    solidrectangle(winW / 2 - GameConfig::S(110), winH / 2 - GameConfig::S(70), winW / 2 + GameConfig::S(110), winH / 2 + GameConfig::S(70));
    setlinecolor(WHITE);
    rectangle(winW / 2 - GameConfig::S(110), winH / 2 - GameConfig::S(70), winW / 2 + GameConfig::S(110), winH / 2 + GameConfig::S(70));

    // GAME OVER 字样渲染
    settextcolor(RGB(255, 50, 50));
    GraphicsEngine::outTextCenter(winW / 2, winH / 2 - GameConfig::S(30), "GAME OVER", GameConfig::S(28), "Impact");

    // 结算分数展示
    settextcolor(WHITE);
    string scoreMsg = "Final Score: " + to_string(this->score);
    GraphicsEngine::outTextCenter(winW / 2, winH / 2 + GameConfig::S(10), scoreMsg.c_str(), GameConfig::S(16), "微软雅黑");

    // 操作引导
    GraphicsEngine::outTextCenter(winW / 2, winH / 2 + GameConfig::S(40), "按 [空格] 重试，[ESC] 退出", GameConfig::S(12), "微软雅黑");

    FlushBatchDraw();

    // 等待操作轮询阻塞，减轻 CPU 负担
    while (true) {
        if (GetAsyncKeyState(VK_SPACE) & 0x8000) {
            this->currentState = PLAYING;
            break;
        }
        if (GetAsyncKeyState(VK_ESCAPE) & 0x8000) {
            this->currentState = MENU;
            break;
        }
        Sleep(50);
    }

    // 等待空格键抬起，防止重试后自动开火
    while (GetAsyncKeyState(VK_SPACE) & 0x8000) {
        Sleep(10);
    }
}



int main() {
    // 激活高 DPI 缩放感知，避免 Windows 在高分屏下导致画面虚化、马赛克和鼠标偏移问题
    SetProcessDPIAware();

    // 初始化唯一的 GameManager 并驱动主循环
    GameManager game;
    game.run();

    return 0;
}