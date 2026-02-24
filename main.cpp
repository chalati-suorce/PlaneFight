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

// 引入透明贴图所需的库
#pragma comment(lib, "msimg32.lib")

using namespace std;

// ================= 核心缩放系统 =================
// 放大 3.4 倍 (基础 256x256 -> 放大后约 870x870)
const double SCALE = 3.4;
inline int S(double v) { return static_cast<int>(v * SCALE); } // 缩放转换器

//  视觉配置 (基准尺寸自动乘以 SCALE)
const int WIN_WIDTH = S(256);
const int WIN_HEIGHT = S(256);
// ===============================================

const COLORREF COLOR_BG = RGB(30, 30, 35);
const COLORREF COLOR_GRID = RGB(50, 50, 60);
const COLORREF COLOR_PLAYER = RGB(50, 150, 250);
const COLORREF COLOR_ENEMY = RGB(230, 80, 80);
const COLORREF COLOR_BULLET = RGB(255, 200, 50);
const COLORREF COLOR_TEXT = RGB(240, 240, 240);

//  辅助：矩形碰撞检测 
bool isColliding(int x1, int y1, int w1, int h1, int x2, int y2, int w2, int h2) {
    return (x1 < x2 + w2 && x1 + w1 > x2 &&
        y1 < y2 + h2 && y1 + h1 > y2);
}

bool isPointInRect(int px, int py, int rx, int ry, int rw, int rh) {
    return (px >= rx && px <= rx + rw && py >= ry && py <= ry + rh);
}

//  辅助：绘制居中文字 
void outTextCenter(int x, int y, const char* str, int fontSize = 20, const char* fontName = "微软雅黑") {
    LOGFONT f;
    gettextstyle(&f);
    f.lfHeight = fontSize;
    f.lfQuality = ANTIALIASED_QUALITY;

    // 支持动态传入字体名称
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

//  辅助：绘制透明 PNG 贴图 
void drawImageAlpha(int x, int y, IMAGE* img) {
    if (!img || img->getwidth() == 0) return;
    HDC dstDC = GetImageHDC();
    HDC srcDC = GetImageHDC(img);
    int w = img->getwidth();
    int h = img->getheight();
    BLENDFUNCTION bf = { AC_SRC_OVER, 0, 255, AC_SRC_ALPHA };
    AlphaBlend(dstDC, x, y, w, h, srcDC, 0, 0, w, h, bf);
}

//  全局贴图资源 
IMAGE imgPlayer, imgEnemy, imgBulletPlayer, imgBulletEnemy;
bool hasImgPlayer = false, hasImgEnemy = false, hasImgBulletP = false, hasImgBulletE = false;

//  基类 
class GameObject {
public:
    double x, y;
    int width, height;

    GameObject(double _x, double _y, int _w, int _h)
        : x(_x), y(_y), width(_w), height(_h) {
    }
    virtual ~GameObject() {}
    virtual void draw() = 0;
    virtual bool move() = 0;
};

//  玩家子弹 
class Bullet : public GameObject {
public:
    Bullet(double _x, double _y) : GameObject(_x, _y, S(8), S(24)) {}

    void draw() override {
        if (hasImgBulletP) {
            drawImageAlpha((int)x, (int)y, &imgBulletPlayer);
        }
        else {
            setfillcolor(COLOR_BULLET);
            solidrectangle((int)x, (int)y, (int)x + width, (int)y + height);
        }
    }

    bool move() override {
        y -= S(12); // 玩家子弹向上，按比例提速
        return y + height > 0;
    }
};

//  敌机子弹 
class EnemyBullet : public GameObject {
public:
    EnemyBullet(double _x, double _y) : GameObject(_x, _y, S(8), S(24)) {}

    void draw() override {
        if (hasImgBulletE) {
            drawImageAlpha((int)x, (int)y, &imgBulletEnemy);
        }
        else {
            setfillcolor(RGB(255, 100, 100)); // 红色备用激光
            solidrectangle((int)x, (int)y, (int)x + width, (int)y + height);
        }
    }

    bool move() override {
        y += S(7); // 敌机子弹向下
        return y < WIN_HEIGHT;
    }
};

//  敌机 
class Enemy : public GameObject {
public:
    Enemy(double _x, double _y) : GameObject(_x, _y, S(40), S(40)) {}

    void draw() override {
        if (hasImgEnemy) {
            drawImageAlpha((int)x, (int)y, &imgEnemy);
        }
        else {
            setfillcolor(COLOR_ENEMY);
            POINT pts[] = {
                {(int)x, (int)y},
                {(int)x + width, (int)y},
                {(int)x + width / 2, (int)y + height}
            };
            solidpolygon(pts, 3);
        }
    }

    bool move() override {
        y += S(3);
        return y < WIN_HEIGHT;
    }
};

//  玩家 
class Player : public GameObject {
public:
    Player() : GameObject(WIN_WIDTH / 2 - S(16), WIN_HEIGHT - S(55), S(32), S(32)) {}

    void draw() override {
        if (hasImgPlayer) {
            drawImageAlpha((int)x, (int)y, &imgPlayer);
        }
        else {
            setfillcolor(COLOR_PLAYER);
            POINT pts[] = {
                {(int)x + width / 2, (int)y},
                {(int)x, (int)y + height},
                {(int)x + width / 2, (int)y + height - S(6)},
                {(int)x + width, (int)y + height}
            };
            solidpolygon(pts, 4);
            setfillcolor(WHITE);
            solidcircle((int)x + width / 2, (int)y + S(6), S(2));
        }
    }

    bool move() override {
        if (GetAsyncKeyState('A') || GetAsyncKeyState(VK_LEFT)) { if (x > 0) x -= S(6); }
        if (GetAsyncKeyState('D') || GetAsyncKeyState(VK_RIGHT)) { if (x < WIN_WIDTH - width) x += S(6); }
        if (GetAsyncKeyState('W') || GetAsyncKeyState(VK_UP)) { if (y > 0) y -= S(6); }
        if (GetAsyncKeyState('S') || GetAsyncKeyState(VK_DOWN)) { if (y < WIN_HEIGHT - height) y += S(6); }
        return true;
    }
};

//  UI 按钮 (支持自定义字体)
struct Button {
    int x, y, w, h;
    string text;
    bool isHover;
    int fontSize;
    COLORREF baseColor;
    string fontName; // 新增：按钮字体

    Button(int _x = 0, int _y = 0, int _w = 0, int _h = 0, string _text = "", int _fs = 16, COLORREF _c = RGB(50, 120, 210), string _fn = "微软雅黑")
        : x(_x), y(_y), w(_w), h(_h), text(_text), isHover(false), fontSize(_fs), baseColor(_c), fontName(_fn) {
    }

    void draw() {
        int cornerRadius = S(6); // 圆角稍微缩小一点，更精致

        // 1. 绘制阴影
        setfillcolor(RGB(15, 15, 20));
        solidroundrect(x + S(3), y + S(3), x + w + S(3), y + h + S(3), cornerRadius, cornerRadius);

        // 2. 绘制主体 (悬停时提亮)
        int r = min(255, GetRValue(baseColor) + (isHover ? 50 : 0));
        int g = min(255, GetGValue(baseColor) + (isHover ? 50 : 0));
        int b = min(255, GetBValue(baseColor) + (isHover ? 50 : 0));
        setfillcolor(RGB(r, g, b));
        solidroundrect(x, y, x + w, y + h, cornerRadius, cornerRadius);

        // 3. 绘制边缘
        setlinecolor(isHover ? RGB(240, 240, 255) : RGB(r + 30, g + 30, b + 30));
        setlinestyle(PS_SOLID, max(2, S(1)));
        roundrect(x, y, x + w, y + h, cornerRadius, cornerRadius);
        setlinestyle(PS_SOLID, 1);

        // 4. 绘制文字 (使用结构体中保存的字体名)
        setbkmode(TRANSPARENT);
        settextcolor(WHITE);
        outTextCenter(x + w / 2, y + h / 2, text.c_str(), fontSize, fontName.c_str());
    }

    bool update(const MOUSEMSG& msg) {
        isHover = isPointInRect(msg.x, msg.y, x, y, w, h);
        return (isHover && msg.uMsg == WM_LBUTTONDOWN);
    }
};

//  游戏管理器 
class GameManager {
private:
    Player* player;
    list<Bullet*> bullets;
    list<Enemy*> enemies;
    list<EnemyBullet*> enemyBullets;

    int score;
    bool gameOver;
    long frameCount;
    int shootTimer;
    int pauseCooldown;

    // 难度控制变量
    int enemySpawnRate;
    int enemyShootChance;

    enum GameState { MENU, PLAYING, PAUSED, END };
    GameState currentState;

    IMAGE bgImg;
    bool hasBgImg;

    // 菜单按钮
    Button btnEasy;
    Button btnNormal;
    Button btnHell;

    // 游戏内按钮
    Button btnPause;
    Button btnResume;
    Button btnMenu;

public:
    GameManager() {
        initgraph(WIN_WIDTH, WIN_HEIGHT);
        setbkcolor(COLOR_BG);

        // 加载背景
        loadimage(&bgImg, _T("purple.png"), WIN_WIDTH, WIN_HEIGHT, true);
        hasBgImg = (bgImg.getwidth() > 0);

        // 加载资源
        loadimage(&imgPlayer, _T("playerShip_blue.png"), S(32), S(32), true);
        hasImgPlayer = (imgPlayer.getwidth() > 0);

        loadimage(&imgEnemy, _T("playerShip_red.png"), S(40), S(40), true);
        hasImgEnemy = (imgEnemy.getwidth() > 0);

        loadimage(&imgBulletPlayer, _T("laserBlue.png"), S(8), S(24), true);
        hasImgBulletP = (imgBulletPlayer.getwidth() > 0);

        loadimage(&imgBulletEnemy, _T("laserRed.png"), S(8), S(24), true);
        hasImgBulletE = (imgBulletEnemy.getwidth() > 0);

        // 初始化主菜单按钮 (尺寸全面缩小：宽120->96, 高35->28, 字体16->13)
        int btnW = S(96), btnH = S(28), btnX = WIN_WIDTH / 2 - btnW / 2;
        btnEasy = Button(btnX, S(95), btnW, btnH, "简单模式", S(13), RGB(40, 160, 80));
        btnNormal = Button(btnX, S(135), btnW, btnH, "普通模式", S(13), RGB(50, 120, 210));
        btnHell = Button(btnX, S(175), btnW, btnH, "地狱模式", S(13), RGB(180, 50, 50));

        // 游戏界面的右上角按钮
        btnPause = Button(WIN_WIDTH - S(45), S(5), S(40), S(20), "暂停", S(12));
        btnMenu = Button(WIN_WIDTH - S(90), S(5), S(40), S(20), "菜单", S(12), RGB(200, 80, 80));

        btnResume = Button(WIN_WIDTH / 2 - S(45), WIN_HEIGHT / 2 + S(10), S(90), S(28), "点击继续", S(14));

        currentState = MENU;

        enemySpawnRate = 30;
        enemyShootChance = 2;

        resetGame();
    }

    ~GameManager() {
        closegraph();
        delete player;
        for (auto b : bullets) delete b;
        for (auto e : enemies) delete e;
        for (auto eb : enemyBullets) delete eb;
    }

    void resetGame() {
        if (player) delete player;
        player = new Player();
        score = 0;
        gameOver = false;
        frameCount = 0;
        shootTimer = 0;
        pauseCooldown = 0;

        for (auto b : bullets) delete b; bullets.clear();
        for (auto e : enemies) delete e; enemies.clear();
        for (auto eb : enemyBullets) delete eb; enemyBullets.clear();

        srand((unsigned int)time(0));
    }

    void setDifficulty(int spawnRate, int shootChance) {
        enemySpawnRate = spawnRate;
        enemyShootChance = shootChance;
    }

    void run() {
        BeginBatchDraw();

        while (true) {
            cleardevice();

            if (currentState == MENU) {
                runMenu();
            }
            else if (currentState == PLAYING) {
                drawBackground();
                runGame();
                if (gameOver) currentState = END;
            }
            else if (currentState == PAUSED) {
                drawBackground();
                renderGame();
                runPaused();
            }
            else if (currentState == END) {
                showGameOver();
                if (currentState == PLAYING) resetGame();
                if (currentState == MENU) break;
            }

            FlushBatchDraw();
            Sleep(16);
        }

        EndBatchDraw();
    }

private:
    void drawBackground() {
        if (hasBgImg) {
            putimage(0, 0, &bgImg);
        }
        else {
            setfillcolor(COLOR_BG);
            solidrectangle(0, 0, WIN_WIDTH, WIN_HEIGHT);
            setlinecolor(COLOR_GRID);
            for (int i = 0; i < WIN_WIDTH; i += S(40)) line(i, 0, i, WIN_HEIGHT);
            for (int i = 0; i < WIN_HEIGHT; i += S(40)) line(0, i, WIN_WIDTH, i);
        }
    }

    void runMenu() {
        drawBackground();
        setbkmode(TRANSPARENT);

        // 标题尺寸缩小 (32->26)，更换为具有街机游戏感的“华文琥珀”字体
        settextcolor(RGB(20, 20, 20));
        outTextCenter(WIN_WIDTH / 2 + S(2), S(42), "基于链表的飞机大战游戏", S(22), "华文琥珀");
        settextcolor(COLOR_TEXT);
        outTextCenter(WIN_WIDTH / 2, S(40), "基于链表的飞机大战游戏", S(22), "华文琥珀");

        MOUSEMSG msg = { 0 };
        while (MouseHit()) {
            msg = GetMouseMsg();

            if (btnEasy.update(msg)) {
                setDifficulty(60, 1);
                currentState = PLAYING;
                FlushMouseMsgBuffer();
                break;
            }
            if (btnNormal.update(msg)) {
                setDifficulty(30, 2);
                currentState = PLAYING;
                FlushMouseMsgBuffer();
                break;
            }
            if (btnHell.update(msg)) {
                setDifficulty(10, 6);
                currentState = PLAYING;
                FlushMouseMsgBuffer();
                break;
            }
        }

        btnEasy.draw();
        btnNormal.draw();
        btnHell.draw();

        // 开发者字体尺寸缩小 (12->10)，更换为圆润精致的“幼圆”
        settextcolor(RGB(150, 150, 150));
        outTextCenter(WIN_WIDTH / 2, WIN_HEIGHT - S(15), "开发者：方恒康、石岩、耿乐、王智轩", S(10), "幼圆");
    }

    void runGame() {
        MOUSEMSG msg = { 0 };
        while (MouseHit()) {
            msg = GetMouseMsg();
            if (btnPause.update(msg)) {
                currentState = PAUSED;
                pauseCooldown = 15;
                FlushMouseMsgBuffer();
                return;
            }
            if (btnMenu.update(msg)) {
                currentState = MENU;
                resetGame();
                FlushMouseMsgBuffer();
                return;
            }
        }

        handleInput();
        update();
        renderGame();

        btnPause.draw();
        btnMenu.draw();

        frameCount++;
    }

    void runPaused() {
        setfillcolor(COLOR_BG);
        solidrectangle(WIN_WIDTH / 2 - S(80), WIN_HEIGHT / 2 - S(50), WIN_WIDTH / 2 + S(80), WIN_HEIGHT / 2 + S(50));
        setlinecolor(WHITE);
        rectangle(WIN_WIDTH / 2 - S(80), WIN_HEIGHT / 2 - S(50), WIN_WIDTH / 2 + S(80), WIN_HEIGHT / 2 + S(50));

        setbkmode(TRANSPARENT);
        settextcolor(WHITE);
        outTextCenter(WIN_WIDTH / 2, WIN_HEIGHT / 2 - S(25), "游戏暂停", S(20), "微软雅黑");
        outTextCenter(WIN_WIDTH / 2, WIN_HEIGHT / 2 - S(5), "或按 [P] 键继续", S(12), "微软雅黑");

        if (pauseCooldown > 0) pauseCooldown--;

        MOUSEMSG msg = { 0 };
        while (MouseHit()) {
            msg = GetMouseMsg();
            if (btnResume.update(msg) && pauseCooldown <= 0) {
                currentState = PLAYING;
                pauseCooldown = 15;
                FlushMouseMsgBuffer();
                return;
            }
            if (btnMenu.update(msg)) {
                currentState = MENU;
                resetGame();
                FlushMouseMsgBuffer();
                return;
            }
        }

        btnResume.draw();

        btnPause.draw();
        btnMenu.draw();

        if (GetAsyncKeyState('P') && pauseCooldown <= 0) {
            currentState = PLAYING;
            pauseCooldown = 15;
        }
    }

    void handleInput() {
        player->move();

        if (shootTimer > 0) shootTimer--;
        if (GetAsyncKeyState(VK_SPACE) && shootTimer <= 0) {
            bullets.push_back(new Bullet(player->x + S(4), player->y));
            bullets.push_back(new Bullet(player->x + player->width - S(12), player->y));
            shootTimer = 10;
        }

        if (pauseCooldown > 0) pauseCooldown--;
        if (GetAsyncKeyState('P') && pauseCooldown <= 0) {
            currentState = PAUSED;
            pauseCooldown = 15;
        }
    }

    void update() {
        if (frameCount % enemySpawnRate == 0) {
            int randX = rand() % (WIN_WIDTH - S(40));
            enemies.push_back(new Enemy(randX, -S(40)));
        }

        for (auto it = bullets.begin(); it != bullets.end(); ) {
            if (!(*it)->move()) {
                delete* it;
                it = bullets.erase(it);
            }
            else { ++it; }
        }

        for (auto it = enemyBullets.begin(); it != enemyBullets.end(); ) {
            if (!(*it)->move()) {
                delete* it;
                it = enemyBullets.erase(it);
                continue;
            }
            if (isColliding((*it)->x, (*it)->y, (*it)->width, (*it)->height,
                player->x + S(6), player->y + S(6), player->width - S(12), player->height - S(12))) {
                gameOver = true;
            }
            ++it;
        }

        for (auto it = enemies.begin(); it != enemies.end(); ) {
            bool isDead = false;
            if (!(*it)->move()) {
                delete* it;
                it = enemies.erase(it);
                continue;
            }

            if (rand() % 100 < enemyShootChance) {
                enemyBullets.push_back(new EnemyBullet((*it)->x + (*it)->width / 2 - S(4), (*it)->y + (*it)->height));
            }

            if (isColliding((*it)->x, (*it)->y, (*it)->width, (*it)->height,
                player->x + S(6), player->y + S(6), player->width - S(12), player->height - S(12))) {
                gameOver = true;
            }

            for (auto bIt = bullets.begin(); bIt != bullets.end(); ) {
                if (isColliding((*bIt)->x, (*bIt)->y, (*bIt)->width, (*bIt)->height,
                    (*it)->x, (*it)->y, (*it)->width, (*it)->height)) {
                    score += 10;
                    isDead = true;
                    delete* bIt;
                    bIt = bullets.erase(bIt);
                    break;
                }
                else {
                    ++bIt;
                }
            }

            if (isDead) {
                delete* it;
                it = enemies.erase(it);
            }
            else {
                ++it;
            }
        }
    }

    void renderGame() {
        player->draw();
        for (auto b : bullets) b->draw();
        for (auto e : enemies) e->draw();
        for (auto eb : enemyBullets) eb->draw();

        setbkmode(TRANSPARENT);
        settextcolor(COLOR_TEXT);
        string scoreText = "SCORE: " + to_string(score);

        LOGFONT f;
        gettextstyle(&f);
        f.lfHeight = S(16);
        f.lfWidth = 0;
        f.lfQuality = ANTIALIASED_QUALITY;
        _tcscpy_s(f.lfFaceName, _T("Consolas"));
        settextstyle(&f);

#ifdef UNICODE
        int len = MultiByteToWideChar(CP_ACP, 0, scoreText.c_str(), -1, NULL, 0);
        wchar_t* wstr = new wchar_t[len];
        MultiByteToWideChar(CP_ACP, 0, scoreText.c_str(), -1, wstr, len);
        outtextxy(S(5), S(5), wstr);
        delete[] wstr;
#else
        outtextxy(S(5), S(5), scoreText.c_str());
#endif
    }

    void showGameOver() {
        setfillcolor(RGB(0, 0, 0));
        solidrectangle(WIN_WIDTH / 2 - S(110), WIN_HEIGHT / 2 - S(70), WIN_WIDTH / 2 + S(110), WIN_HEIGHT / 2 + S(70));
        setlinecolor(WHITE);
        rectangle(WIN_WIDTH / 2 - S(110), WIN_HEIGHT / 2 - S(70), WIN_WIDTH / 2 + S(110), WIN_HEIGHT / 2 + S(70));

        settextcolor(RGB(255, 50, 50));
        outTextCenter(WIN_WIDTH / 2, WIN_HEIGHT / 2 - S(30), "GAME OVER", S(28), "Impact");

        settextcolor(WHITE);
        string scoreMsg = "Final Score: " + to_string(score);
        outTextCenter(WIN_WIDTH / 2, WIN_HEIGHT / 2 + S(10), scoreMsg.c_str(), S(16), "微软雅黑");

        outTextCenter(WIN_WIDTH / 2, WIN_HEIGHT / 2 + S(40), "按 [空格] 重试，[ESC] 退出", S(12), "微软雅黑");

        FlushBatchDraw();

        while (true) {
            if (GetAsyncKeyState(VK_SPACE) & 0x8000) {
                currentState = PLAYING;
                break;
            }
            if (GetAsyncKeyState(VK_ESCAPE) & 0x8000) {
                currentState = MENU;
                break;
            }
            Sleep(50);
        }
        while (GetAsyncKeyState(VK_SPACE) & 0x8000) Sleep(10);
    }
};

int main() {
    SetProcessDPIAware();

    GameManager game;
    game.run();
    return 0;
}