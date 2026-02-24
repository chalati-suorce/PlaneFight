#define _WIN32_WINNT 0x0600
#include <windows.h>
#include <graphics.h>  
#include <conio.h>
#include <list>
#include <string>
#include <vector>
#include <ctime>
#include <tchar.h>

// 引入透明贴图所需的库
#pragma comment(lib, "msimg32.lib")

using namespace std;

//  视觉配置 
const int WIN_WIDTH = 480;
const int WIN_HEIGHT = 640;

const COLORREF COLOR_BG = RGB(30, 30, 35);
const COLORREF COLOR_GRID = RGB(50, 50, 60);
const COLORREF COLOR_PLAYER = RGB(50, 150, 250);
const COLORREF COLOR_ENEMY = RGB(230, 80, 80);
const COLORREF COLOR_BULLET = RGB(255, 200, 50);
const COLORREF COLOR_TEXT = RGB(240, 240, 240);
const COLORREF COLOR_BTN_NORMAL = RGB(70, 70, 80);
const COLORREF COLOR_BTN_HOVER = RGB(90, 90, 100);

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
    f.lfQuality = ANTIALIASED_QUALITY; // 增强清晰度：开启文字抗锯齿平滑边缘
    _tcscpy_s(f.lfFaceName, _T("微软雅黑"));
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
    // 使用 AlphaBlend 处理 PNG 的透明通道
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
    Bullet(double _x, double _y) : GameObject(_x, _y, 8, 24) {}

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
        y -= 12; // 玩家子弹向上
        return y + height > 0;
    }
};

//  敌机子弹 
class EnemyBullet : public GameObject {
public:
    EnemyBullet(double _x, double _y) : GameObject(_x, _y, 8, 24) {}

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
        y += 7; // 敌机子弹向下
        return y < WIN_HEIGHT;
    }
};

//  敌机 
class Enemy : public GameObject {
public:
    Enemy(double _x, double _y) : GameObject(_x, _y, 40, 40) {}

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
        y += 3;
        return y < WIN_HEIGHT;
    }
};

//  玩家 
class Player : public GameObject {
public:
    Player() : GameObject(WIN_WIDTH / 2 - 20, WIN_HEIGHT - 60, 48, 48) {}

    void draw() override {
        if (hasImgPlayer) {
            drawImageAlpha((int)x, (int)y, &imgPlayer);
        }
        else {
            setfillcolor(COLOR_PLAYER);
            POINT pts[] = {
                {(int)x + width / 2, (int)y},
                {(int)x, (int)y + height},
                {(int)x + width / 2, (int)y + height - 10},
                {(int)x + width, (int)y + height}
            };
            solidpolygon(pts, 4);
            setfillcolor(WHITE);
            solidcircle((int)x + width / 2, (int)y + 10, 2);
        }
    }

    bool move() override {
        if (GetAsyncKeyState('A') || GetAsyncKeyState(VK_LEFT)) { if (x > 0) x -= 6; }
        if (GetAsyncKeyState('D') || GetAsyncKeyState(VK_RIGHT)) { if (x < WIN_WIDTH - width) x += 6; }
        if (GetAsyncKeyState('W') || GetAsyncKeyState(VK_UP)) { if (y > 0) y -= 6; }
        if (GetAsyncKeyState('S') || GetAsyncKeyState(VK_DOWN)) { if (y < WIN_HEIGHT - height) y += 6; }
        return true;
    }
};

//  UI 按钮 
struct Button {
    int x, y, w, h;
    string text;
    bool isHover;

    Button(int _x, int _y, int _w, int _h, string _text)
        : x(_x), y(_y), w(_w), h(_h), text(_text), isHover(false) {
    }

    void draw() {
        setfillcolor(isHover ? COLOR_BTN_HOVER : COLOR_BTN_NORMAL);
        solidrectangle(x, y, x + w, y + h);
        setbkmode(TRANSPARENT);
        settextcolor(WHITE);
        outTextCenter(x + w / 2, y + h / 2, text.c_str(), 24, "微软雅黑");
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
    list<EnemyBullet*> enemyBullets; // 新增敌机子弹链表

    int score;
    bool gameOver;
    long frameCount;
    int shootTimer;
    int pauseCooldown;

    enum GameState { MENU, PLAYING, PAUSED, END };
    GameState currentState;

    int bgY;
    IMAGE bgImg;
    bool hasBgImg;

public:
    GameManager() {
        initgraph(WIN_WIDTH, WIN_HEIGHT);
        setbkcolor(COLOR_BG);

        // 加载背景
        loadimage(&bgImg, _T("purple.png"), WIN_WIDTH, WIN_HEIGHT);
        hasBgImg = (bgImg.getwidth() > 0);

        // 统一加载贴图资源并按所需大小缩放
        loadimage(&imgPlayer, _T("playerShip_blue.png"), 48, 48, true);
        hasImgPlayer = (imgPlayer.getwidth() > 0);

        loadimage(&imgEnemy, _T("playerShip_red.png"), 40, 40, true);
        hasImgEnemy = (imgEnemy.getwidth() > 0);

        loadimage(&imgBulletPlayer, _T("laserBlue.png"), 8, 24, true);
        hasImgBulletP = (imgBulletPlayer.getwidth() > 0);

        loadimage(&imgBulletEnemy, _T("laserRed.png"), 8, 24, true);
        hasImgBulletE = (imgBulletEnemy.getwidth() > 0);

        currentState = MENU;
        bgY = 0;
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
                if (currentState == PLAYING) resetGame(); // 结束后按键重新开始
                if (currentState == MENU) break;          // 按下退出
            }

            FlushBatchDraw();
            Sleep(16);
        }

        EndBatchDraw();
    }

private:
    void drawBackground() {
        if (hasBgImg) {
            bgY = (bgY + 2) % WIN_HEIGHT;
            putimage(0, bgY, &bgImg);
            putimage(0, bgY - WIN_HEIGHT, &bgImg);
        }
        else {
            setlinecolor(COLOR_GRID);
            bgY = (bgY + 1) % 40;
            for (int i = 0; i < WIN_WIDTH; i += 40) line(i, 0, i, WIN_HEIGHT);
            for (int i = -40; i < WIN_HEIGHT; i += 40) line(0, i + bgY, WIN_WIDTH, i + bgY);
        }
    }

    void runMenu() {
        drawBackground();
        setbkmode(TRANSPARENT);

        settextcolor(RGB(20, 20, 20));
        outTextCenter(WIN_WIDTH / 2 + 2, 122, "基于链表的飞机大战游戏", 32, "微软雅黑");
        //outTextCenter(WIN_WIDTH / 2 + 2, 162, "贴图版", 32, "微软雅黑");

        settextcolor(COLOR_TEXT);
        outTextCenter(WIN_WIDTH / 2, 120, "基于链表的飞机大战游戏", 32, "微软雅黑");
        //outTextCenter(WIN_WIDTH / 2, 160, "贴图版", 32, "微软雅黑");

        settextcolor(RGB(150, 150, 150));
        outTextCenter(WIN_WIDTH / 2, 580, "开发者：方恒康、石岩、耿乐、王智轩", 20, "微软雅黑");

        static Button startBtn(WIN_WIDTH / 2 - 80, 400, 160, 50, "开始游戏");

        MOUSEMSG msg = { 0 };
        while (MouseHit()) msg = GetMouseMsg();

        if (startBtn.update(msg)) {
            currentState = PLAYING;
            FlushMouseMsgBuffer();
        }
        startBtn.draw();
    }

    void runGame() {
        handleInput();
        update();
        renderGame();
        frameCount++;
    }

    void runPaused() {
        setfillcolor(COLOR_BG);
        solidrectangle(WIN_WIDTH / 2 - 120, WIN_HEIGHT / 2 - 60, WIN_WIDTH / 2 + 120, WIN_HEIGHT / 2 + 60);
        setlinecolor(WHITE);
        rectangle(WIN_WIDTH / 2 - 120, WIN_HEIGHT / 2 - 60, WIN_WIDTH / 2 + 120, WIN_HEIGHT / 2 + 60);

        setbkmode(TRANSPARENT);
        settextcolor(WHITE);
        outTextCenter(WIN_WIDTH / 2, WIN_HEIGHT / 2 - 15, "游戏暂停", 30, "微软雅黑");
        outTextCenter(WIN_WIDTH / 2, WIN_HEIGHT / 2 + 25, "按 [P] 键继续", 16, "微软雅黑");

        if (pauseCooldown > 0) pauseCooldown--;

        if (GetAsyncKeyState('P') && pauseCooldown <= 0) {
            currentState = PLAYING;
            pauseCooldown = 15;
        }
    }

    void handleInput() {
        player->move();

        if (shootTimer > 0) shootTimer--;
        if (GetAsyncKeyState(VK_SPACE) && shootTimer <= 0) {
            bullets.push_back(new Bullet(player->x + 6, player->y));
            bullets.push_back(new Bullet(player->x + player->width - 14, player->y));
            shootTimer = 10;
        }

        if (pauseCooldown > 0) pauseCooldown--;
        if (GetAsyncKeyState('P') && pauseCooldown <= 0) {
            currentState = PAUSED;
            pauseCooldown = 15;
        }
    }

    void update() {
        if (frameCount % 30 == 0) {
            int randX = rand() % (WIN_WIDTH - 40);
            enemies.push_back(new Enemy(randX, -40));
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
                player->x + 10, player->y + 10, player->width - 20, player->height - 20)) {
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

            if (rand() % 100 < 2) {
                enemyBullets.push_back(new EnemyBullet((*it)->x + (*it)->width / 2 - 4, (*it)->y + (*it)->height));
            }

            if (isColliding((*it)->x, (*it)->y, (*it)->width, (*it)->height,
                player->x + 10, player->y + 10, player->width - 20, player->height - 20)) {
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

        // 增强清晰度：为左上角分数文字也开启抗锯齿
        LOGFONT f;
        gettextstyle(&f);
        f.lfHeight = 20;
        f.lfWidth = 0;
        f.lfQuality = ANTIALIASED_QUALITY;
        _tcscpy_s(f.lfFaceName, _T("Consolas"));
        settextstyle(&f);

#ifdef UNICODE
        int len = MultiByteToWideChar(CP_ACP, 0, scoreText.c_str(), -1, NULL, 0);
        wchar_t* wstr = new wchar_t[len];
        MultiByteToWideChar(CP_ACP, 0, scoreText.c_str(), -1, wstr, len);
        outtextxy(15, 15, wstr);
        delete[] wstr;
#else
        outtextxy(15, 15, scoreText.c_str());
#endif
    }

    void showGameOver() {
        setfillcolor(RGB(0, 0, 0));
        solidrectangle(WIN_WIDTH / 2 - 150, WIN_HEIGHT / 2 - 100, WIN_WIDTH / 2 + 150, WIN_HEIGHT / 2 + 100);
        setlinecolor(WHITE);
        rectangle(WIN_WIDTH / 2 - 150, WIN_HEIGHT / 2 - 100, WIN_WIDTH / 2 + 150, WIN_HEIGHT / 2 + 100);

        settextcolor(RGB(255, 50, 50));
        outTextCenter(WIN_WIDTH / 2, WIN_HEIGHT / 2 - 40, "GAME OVER", 40, "Impact");

        settextcolor(WHITE);
        string scoreMsg = "Final Score: " + to_string(score);
        outTextCenter(WIN_WIDTH / 2, WIN_HEIGHT / 2 + 20, scoreMsg.c_str(), 24, "微软雅黑");

        outTextCenter(WIN_WIDTH / 2, WIN_HEIGHT / 2 + 60, "按 [空格] 重试，按 [ESC] 退出", 16, "微软雅黑");

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