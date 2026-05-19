#include <GL/glut.h>
#include <iostream>
#include <string>
#include <vector>
#include <cmath>
#include <ctime>
#include <cstdlib>
#include <sstream>
#include <windows.h>
#include <mmsystem.h>
#pragma comment(lib, "winmm.lib")

#define SAMPLE_RATE 22050
void playTone(float freq, int ms, float vol = 0.8f)
{
    int numSamples = (SAMPLE_RATE * ms) / 1000;
    short* buf = (short*)malloc(numSamples * sizeof(short));
    if (!buf) return;

    for (int i = 0; i < numSamples; i++)
    {
        float t       = (float)i / SAMPLE_RATE;
        float fade    = 1.0f - (float)i / numSamples;
        buf[i] = (short)(32767.0f * vol * fade * sin(2.0f * 3.14159f * freq * t));
    }

    WAVEHDR      hdr  = {0};
    WAVEFORMATEX fmt  = {0};
    fmt.wFormatTag      = WAVE_FORMAT_PCM;
    fmt.nChannels       = 1;
    fmt.nSamplesPerSec  = SAMPLE_RATE;
    fmt.wBitsPerSample  = 16;
    fmt.nBlockAlign     = 2;
    fmt.nAvgBytesPerSec = SAMPLE_RATE * 2;

    HWAVEOUT hWave;
    if (waveOutOpen(&hWave, WAVE_MAPPER, &fmt, 0, 0, CALLBACK_NULL) != MMSYSERR_NOERROR)
    {
        free(buf);
        return;
    }

    hdr.lpData         = (LPSTR)buf;
    hdr.dwBufferLength = numSamples * sizeof(short);
    waveOutPrepareHeader(hWave, &hdr, sizeof(hdr));
    waveOutWrite(hWave, &hdr, sizeof(hdr));

    while (!(hdr.dwFlags & WHDR_DONE))
        Sleep(1);

    waveOutUnprepareHeader(hWave, &hdr, sizeof(hdr));
    waveOutClose(hWave);
    free(buf);
}

struct SoundJob { float freq; int ms; float vol; };

DWORD WINAPI soundThread(LPVOID param)
{
    SoundJob* j = (SoundJob*)param;
    playTone(j->freq, j->ms, j->vol);
    free(j);
    return 0;
}

void asyncTone(float freq, int ms, float vol = 0.8f)
{
    SoundJob* j = (SoundJob*)malloc(sizeof(SoundJob));
    j->freq = freq; j->ms = ms; j->vol = vol;

    HANDLE h = CreateThread(NULL, 0, soundThread, j, 0, NULL);
    if (h) CloseHandle(h);

}

void soundWall()          { asyncTone(880,  30, 0.4f); }   // short high tick
void soundPaddle()        { asyncTone(440,  50, 0.6f); }   // medium pop
void soundBrickNormal()   { asyncTone(600,  40, 0.7f); }   // brick destroyed
void soundBrickHit()      { asyncTone(300,  30, 0.5f); }   // brick damaged, not destroyed
void soundPerkGood()      { asyncTone(880,  80, 0.8f); }   // good perk collected
void soundPerkBad()       { asyncTone(150, 150, 0.8f); }   // bad perk / damage
void soundLoseLife()      { asyncTone(200, 300, 0.9f); }   // lose a life
void soundGameOver()      { asyncTone(100, 500, 1.0f); }   // game over
void soundWin()           { asyncTone(1047,300, 0.9f); }   // win
void soundLaunch()        { asyncTone(660,  60, 0.6f); }   // ball launched
void soundMenu()          { asyncTone(520,  40, 0.5f); }   // menu click
void soundSpeedUp()       { asyncTone(990, 100, 0.7f); }   // ball speed increased
void soundBullet()        { asyncTone(1200, 30, 0.5f); }   // bullet fired


using namespace std;
int windowWidth  = 800;
int windowHeight = 600;

#define STATE_MENU       0
#define STATE_PLAYING    1
#define STATE_PAUSED     2
#define STATE_GAME_OVER  3
#define STATE_WIN        4
#define STATE_HELP       5
#define STATE_HIGHSCORE  6

int gameState = STATE_MENU;

float paddleX       = 350.0f;
float paddleY       = 30.0f;
float paddleWidth   = 100.0f;
float paddleHeight  = 15.0f;
float paddleSpeed   = 10.0f;


bool moveLeft  = false;
bool moveRight = false;

float ballX      = 400.0f;
float ballY      = 60.0f;
float ballRadius = 10.0f;
float ballDX     = 3.0f;
float ballDY     = 4.0f;
bool  ballLaunched = false;

float ballSpeedTimer    = 0.0f;
float ballSpeedInterval = 15.0f;
float ballSpeedMax      = 12.0f;

#define BRICK_ROWS    7
#define BRICK_COLS    12
#define BRICK_WIDTH   58
#define BRICK_HEIGHT  22
#define BRICK_OFFSET_X 25
#define BRICK_OFFSET_Y 300


#define BRICK_EMPTY    0
#define BRICK_NORMAL   1
#define BRICK_HARD     2
#define BRICK_WALL     3

struct Brick
{
    float x, y;
    int   type;
    int   hits;
    bool  active;
    float r, g, b;
};

Brick bricks[BRICK_ROWS][BRICK_COLS];


#define PERK_NONE          0
#define PERK_EXTRA_LIFE    1
#define PERK_SPEED_UP      2
#define PERK_WIDE_PADDLE   3
#define PERK_FIREBALL      4
#define PERK_SHRINK_PADDLE 5
#define PERK_INSTANT_DEATH 6

struct PerkItem
{
    float x, y;
    float width, height;
    int   type;
    bool  active;
    float r, g, b;
    float fallSpeed;
};

vector<PerkItem> perkItems;


float widePaddleTimer  = 0.0f;
float fireballTimer    = 0.0f;
bool  isFireball       = false;
bool  isWidePaddle     = false;


int   playerLives  = 3;
int   playerScore  = 0;
float gameTime     = 0.0f;
int   highScore    = 0;


struct Bullet
{
    float x, y;
    bool  active;
};
vector<Bullet> bullets;
bool shootingPaddle = false;
float shootingTimer = 0.0f;

void drawRect(float x, float y, float w, float h,
              float r, float g, float b)
{
    glColor3f(r, g, b);
    glBegin(GL_QUADS);
        glVertex2f(x,     y    );
        glVertex2f(x + w, y    );
        glVertex2f(x + w, y + h);
        glVertex2f(x,     y + h);
    glEnd();
}

void drawRectOutline(float x, float y, float w, float h,
                     float r, float g, float b)
{
    glColor3f(r, g, b);
    glBegin(GL_LINE_LOOP);
        glVertex2f(x,     y    );
        glVertex2f(x + w, y    );
        glVertex2f(x + w, y + h);
        glVertex2f(x,     y + h);
    glEnd();
}

void drawCircle(float cx, float cy, float radius,
                float r, float g, float b)
{
    glColor3f(r, g, b);
    glBegin(GL_TRIANGLE_FAN);
        glVertex2f(cx, cy);
        for (int i = 0; i <= 36; i++)
        {
            float angle = i * 3.14159f * 2.0f / 36.0f;
            glVertex2f(cx + cos(angle) * radius,
                       cy + sin(angle) * radius);
        }
    glEnd();
}


void drawText(float x, float y, string text,
              float r, float g, float b)
{
    glColor3f(r, g, b);
    glRasterPos2f(x, y);
    for (int i = 0; i < (int)text.size(); i++)
    {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, text[i]);
    }
}
void drawTextLarge(float x, float y, string text,
                   float r, float g, float b)
{
    glColor3f(r, g, b);
    glRasterPos2f(x, y);
    for (int i = 0; i < (int)text.size(); i++)
    {
        glutBitmapCharacter(GLUT_BITMAP_TIMES_ROMAN_24, text[i]);
    }
}

string intToStr(int val)
{
    stringstream ss;
    ss << val;
    return ss.str();
}


string floatToStr(float val, int decimals)
{
    stringstream ss;
    ss.precision(decimals);
    ss << fixed << val;
    return ss.str();
}

int countActiveBricks()
{
    int count = 0;
    for (int r = 0; r < BRICK_ROWS; r++)
        for (int c = 0; c < BRICK_COLS; c++)
            if (bricks[r][c].active)
                count++;
    return count;
}

void spawnPerk(float x, float y)
{
    // Randomly decide if a perk drops (40% chance)
    int chance = rand() % 100;
    if (chance > 40) return;

    PerkItem p;
    p.x         = x;
    p.y         = y;
    p.width     = 25.0f;
    p.height    = 15.0f;
    p.active    = true;
    p.fallSpeed = 2.5f;

    int perkRoll = rand() % 6;
    if      (perkRoll == 0) { p.type=PERK_EXTRA_LIFE;    p.r=0;   p.g=1;   p.b=0;   }
    else if (perkRoll == 1) { p.type=PERK_SPEED_UP;      p.r=1;   p.g=1;   p.b=0;   }
    else if (perkRoll == 2) { p.type=PERK_WIDE_PADDLE;   p.r=0;   p.g=1;   p.b=1;   }
    else if (perkRoll == 3) { p.type=PERK_FIREBALL;      p.r=1;   p.g=0.5; p.b=0;   }
    else if (perkRoll == 4) { p.type=PERK_SHRINK_PADDLE; p.r=1;   p.g=0;   p.b=0;   }
    else                    { p.type=PERK_INSTANT_DEATH;  p.r=0.5; p.g=0;   p.b=0.5; }

    perkItems.push_back(p);
}

void initBricks()
{
    for (int r = 0; r < BRICK_ROWS; r++)
    {
        for (int c = 0; c < BRICK_COLS; c++)
        {
            Brick &b = bricks[r][c];

            b.x      = BRICK_OFFSET_X + c * (BRICK_WIDTH  + 4);
            b.y      = BRICK_OFFSET_Y + r * (BRICK_HEIGHT + 4);
            b.active = true;

            if (r == 0 || r == BRICK_ROWS - 1)
            {

                b.type = BRICK_WALL;
                b.hits = 3;
                b.r    = 0.6f; b.g = 0.4f; b.b = 0.2f;
            }
            else if (r == 1 || r == BRICK_ROWS - 2)
            {
                b.type = BRICK_HARD;
                b.hits = 2;
                b.r    = 0.9f; b.g = 0.2f; b.b = 0.8f;
            }
            else
            {

                b.type = BRICK_NORMAL;
                b.hits = 1;

                float colorRoll = (rand() % 4);
                if      (colorRoll == 0) { b.r=1;    b.g=0.2f; b.b=0.2f; }
                else if (colorRoll == 1) { b.r=1;    b.g=0.6f; b.b=0;    }
                else if (colorRoll == 2) { b.r=0.8f; b.g=0;    b.b=0.8f; }
                else                     { b.r=0.2f; b.g=0.4f; b.b=1;    }
            }
        }
    }
}

void resetBall()
{
    ballX        = paddleX + paddleWidth / 2.0f;
    ballY        = paddleY + paddleHeight + ballRadius + 1;
    ballDX       = 3.0f;
    ballDY       = 4.0f;
    ballLaunched = false;
}

void resetGame()
{
    playerLives    = 3;
    playerScore    = 0;
    gameTime       = 0.0f;
    ballSpeedTimer = 0.0f;
    paddleX        = 350.0f;
    paddleWidth    = 100.0f;
    isFireball     = false;
    isWidePaddle   = false;
    widePaddleTimer= 0.0f;
    fireballTimer  = 0.0f;
    shootingPaddle = false;
    shootingTimer  = 0.0f;
    bullets.clear();
    perkItems.clear();
    initBricks();
    resetBall();
}


void applyPerk(int type)
{
    if (type == PERK_EXTRA_LIFE)
    {
        soundPerkGood();
        playerLives++;
        cout << "Perk: Extra Life! Lives = " << playerLives << endl;
    }
    else if (type == PERK_SPEED_UP)
    {
        soundPerkGood();

        float speed = sqrt(ballDX*ballDX + ballDY*ballDY);
        if (speed < ballSpeedMax)
        {
            ballDX *= 1.2f;
            ballDY *= 1.2f;
        }
        cout << "Perk: Speed Up!" << endl;
    }
    else if (type == PERK_WIDE_PADDLE)
    {
        soundPerkGood();
        isWidePaddle    = true;
        widePaddleTimer = 15.0f;
        paddleWidth     = 160.0f;
        cout << "Perk: Wide Paddle!" << endl;
    }
    else if (type == PERK_FIREBALL)
    {
        soundPerkGood();
        isFireball    = true;
        fireballTimer = 10.0f;
        cout << "Perk: Fireball!" << endl;
    }
    else if (type == PERK_SHRINK_PADDLE)
    {
        soundPerkBad();
        paddleWidth = max(40.0f, paddleWidth - 30.0f);
        cout << "Damage: Shrink Paddle!" << endl;
    }
    else if (type == PERK_INSTANT_DEATH)
    {
        soundPerkBad();
        playerLives--;
        cout << "Damage: Instant Death!" << endl;
        if (playerLives <= 0)
        {
            if (playerScore > highScore) highScore = playerScore;
            soundGameOver();
            gameState = STATE_GAME_OVER;
        }
        else
        {
            soundLoseLife();
            resetBall();
        }
    }
}


void checkBallBrickCollision()
{
    for (int r = 0; r < BRICK_ROWS; r++)
    {
        for (int c = 0; c < BRICK_COLS; c++)
        {
            Brick &b = bricks[r][c];
            if (!b.active) continue;

            float closestX = max(b.x, min(ballX, b.x + BRICK_WIDTH));
            float closestY = max(b.y, min(ballY, b.y + BRICK_HEIGHT));

            float distX = ballX - closestX;
            float distY = ballY - closestY;
            float dist  = sqrt(distX*distX + distY*distY);

            if (dist < ballRadius)
            {

                b.hits--;

                if (b.hits <= 0)
                {
                    b.active = false;
                    if      (b.type == BRICK_NORMAL) playerScore += 10;
                    else if (b.type == BRICK_HARD)   playerScore += 25;
                    else if (b.type == BRICK_WALL)   playerScore += 50;

                    soundBrickNormal();
                    spawnPerk(b.x + BRICK_WIDTH/2, b.y);
                }
                else
                {

                    b.r = min(1.0f, b.r + 0.2f);
                    soundBrickHit();
                }


                if (isFireball)
                {

                }
                else
                {

                    if (abs(distX) > abs(distY))
                        ballDX = -ballDX;
                    else
                        ballDY = -ballDY;
                }


                return;
            }
        }
    }
}


void checkWin()
{
    if (countActiveBricks() == 0)
    {
        if (playerScore > highScore) highScore = playerScore;
        soundWin();
        gameState = STATE_WIN;
    }
}


void update(float deltaTime)
{
    if (gameState != STATE_PLAYING) return;

    gameTime += deltaTime;

    ballSpeedTimer += deltaTime;
    if (ballSpeedTimer >= ballSpeedInterval)
    {
        ballSpeedTimer = 0.0f;
        float speed = sqrt(ballDX*ballDX + ballDY*ballDY);
        if (speed < ballSpeedMax)
        {
            float factor = 1.1f;
            ballDX *= factor;
            ballDY *= factor;
            soundSpeedUp();
            cout << "Ball speed increased!" << endl;
        }
    }

    if (isWidePaddle)
    {
        widePaddleTimer -= deltaTime;
        if (widePaddleTimer <= 0)
        {
            isWidePaddle = false;
            paddleWidth  = 100.0f;
        }
    }
    if (isFireball)
    {
        fireballTimer -= deltaTime;
        if (fireballTimer <= 0)
        {
            isFireball = false;
        }
    }
    if (shootingPaddle)
    {
        shootingTimer -= deltaTime;
        if (shootingTimer <= 0)
        {
            shootingPaddle = false;
        }
    }

    if (moveLeft)
    {
        paddleX -= paddleSpeed;
        if (paddleX < 10) paddleX = 10;
    }
    if (moveRight)
    {
        paddleX += paddleSpeed;
        if (paddleX + paddleWidth > windowWidth - 10)
            paddleX = windowWidth - 10 - paddleWidth;
    }

    if (!ballLaunched)
    {
        ballX = paddleX + paddleWidth / 2.0f;
        ballY = paddleY + paddleHeight + ballRadius + 1;
        return;
    }

    ballX += ballDX;
    ballY += ballDY;

    if (ballX - ballRadius < 10)
    {
        ballX  = 10 + ballRadius;
        ballDX = -ballDX;
        soundWall();
    }

    if (ballX + ballRadius > windowWidth - 10)
    {
        ballX  = windowWidth - 10 - ballRadius;
        ballDX = -ballDX;
        soundWall();
    }

    if (ballY + ballRadius > windowHeight - 10)
    {
        ballY  = windowHeight - 10 - ballRadius;
        ballDY = -ballDY;
        soundWall();
    }


    if (ballY - ballRadius < 0)
    {
        playerLives--;
        if (playerLives <= 0)
        {
            if (playerScore > highScore) highScore = playerScore;
            soundGameOver();
            gameState = STATE_GAME_OVER;
        }
        else
        {
            soundLoseLife();
            resetBall();
        }
        return;
    }

    if (ballY - ballRadius <= paddleY + paddleHeight &&
        ballY - ballRadius >= paddleY                &&
        ballX >= paddleX                             &&
        ballX <= paddleX + paddleWidth               &&
        ballDY < 0)
    {

        ballDY = -ballDY;
        ballY  = paddleY + paddleHeight + ballRadius + 1;

        float hitPos = (ballX - paddleX) / paddleWidth;  // 0 to 1
        // hitPos 0 = left edge, 1 = right edge
        float speed  = sqrt(ballDX*ballDX + ballDY*ballDY);
        float angle  = (hitPos - 0.5f) * 2.5f;  // -1.25 to +1.25 radians range
        ballDX       = speed * sin(angle);
        ballDY       = speed * cos(angle);
        if (ballDY < 0) ballDY = -ballDY;  // always go up
        if (abs(ballDY) < 1.0f) ballDY = 1.5f;

        soundPaddle();
    }

    checkBallBrickCollision();

    for (int i = 0; i < (int)perkItems.size(); i++)
    {
        PerkItem &p = perkItems[i];
        if (!p.active) continue;

        p.y -= p.fallSpeed;


        if (p.y <= paddleY + paddleHeight &&
            p.y >= paddleY               &&
            p.x + p.width  >= paddleX    &&
            p.x <= paddleX + paddleWidth)
        {
            p.active = false;
            applyPerk(p.type);
        }

        if (p.y < 0)
        {
            p.active = false;
        }
    }

    for (int i = 0; i < (int)bullets.size(); i++)
    {
        Bullet &blt = bullets[i];
        if (!blt.active) continue;

        blt.y += 7.0f;

        for (int r = 0; r < BRICK_ROWS; r++)
        {
            for (int c = 0; c < BRICK_COLS; c++)
            {
                Brick &br = bricks[r][c];
                if (!br.active) continue;

                if (blt.x >= br.x && blt.x <= br.x + BRICK_WIDTH &&
                    blt.y >= br.y && blt.y <= br.y + BRICK_HEIGHT)
                {
                    blt.active = false;
                    br.hits--;
                    if (br.hits <= 0)
                    {
                        br.active = false;
                        playerScore += 10;
                        soundBrickNormal();
                        spawnPerk(br.x + BRICK_WIDTH/2, br.y);
                    }
                    else
                    {
                        soundBrickHit();
                    }
                }
            }
        }
        if (blt.y > windowHeight)
            blt.active = false;
    }

    checkWin();

}

void drawBricks()
{
    for (int r = 0; r < BRICK_ROWS; r++)
    {
        for (int c = 0; c < BRICK_COLS; c++)
        {
            Brick &b = bricks[r][c];
            if (!b.active) continue;

            drawRect(b.x, b.y, BRICK_WIDTH, BRICK_HEIGHT, b.r, b.g, b.b);

            drawRectOutline(b.x, b.y, BRICK_WIDTH, BRICK_HEIGHT, 0.0f, 0.0f, 0.0f);

            if (b.type == BRICK_HARD || b.type == BRICK_WALL)
            {
                string hitsStr = intToStr(b.hits);
                drawText(b.x + BRICK_WIDTH/2 - 4, b.y + 6, hitsStr, 1, 1, 1);
            }
        }
    }
}


void drawPaddle()
{

    float pr = 0.5f, pg = 0.5f, pb = 1.0f;

    if (isFireball)    { pr=1; pg=0.4f; pb=0; }
    if (isWidePaddle)  { pr=0; pg=1;    pb=1;  }

    drawRect(paddleX, paddleY, paddleWidth, paddleHeight, pr, pg, pb);
    drawRectOutline(paddleX, paddleY, paddleWidth, paddleHeight, 1, 1, 1);
}


void drawBall()
{
    float br = 1.0f, bg = 1.0f, bb = 1.0f;
    if (isFireball) { br=1; bg=0.4f; bb=0; }

    drawCircle(ballX, ballY, ballRadius, br, bg, bb);
    drawCircle(ballX - 3, ballY + 3, ballRadius * 0.4f, 1, 1, 1);
}

void drawPerkItems()
{
    for (int i = 0; i < (int)perkItems.size(); i++)
    {
        PerkItem &p = perkItems[i];
        if (!p.active) continue;

        drawRect(p.x, p.y, p.width, p.height, p.r, p.g, p.b);
        drawRectOutline(p.x, p.y, p.width, p.height, 1, 1, 1);

        string label = "?";
        if      (p.type == PERK_EXTRA_LIFE)    label = "+L";
        else if (p.type == PERK_SPEED_UP)      label = "+S";
        else if (p.type == PERK_WIDE_PADDLE)   label = "+W";
        else if (p.type == PERK_FIREBALL)      label = "FB";
        else if (p.type == PERK_SHRINK_PADDLE) label = "-W";
        else if (p.type == PERK_INSTANT_DEATH) label = "XX";

        drawText(p.x + 2, p.y + 3, label, 0, 0, 0);
    }
}

void drawBullets()
{
    for (int i = 0; i < (int)bullets.size(); i++)
    {
        Bullet &blt = bullets[i];
        if (!blt.active) continue;
        drawRect(blt.x - 2, blt.y, 4, 10, 1, 1, 0);
    }
}

void drawHUD()
{

    drawRect(0, 0, windowWidth, 20, 0.1f, 0.1f, 0.1f);


    string livesStr = "Lives: " + intToStr(playerLives);
    drawText(10, 3, livesStr, 0, 1, 0);


    string scoreStr = "Score: " + intToStr(playerScore);
    drawText(150, 3, scoreStr, 1, 1, 0);

    string timeStr = "Time: " + floatToStr(gameTime, 1) + "s";
    drawText(320, 3, timeStr, 0, 1, 1);

    string highStr = "High: " + intToStr(highScore);
    drawText(500, 3, highStr, 1, 0.5f, 0);

    if (isWidePaddle)
    {
        string wpStr = "WIDE:" + floatToStr(widePaddleTimer, 0) + "s";
        drawText(650, 3, wpStr, 0, 1, 1);
    }
    if (isFireball)
    {
        string fbStr = "FIRE:" + floatToStr(fireballTimer, 0) + "s";
        drawText(650, 3, fbStr, 1, 0.4f, 0);
    }

    for (int i = 0; i < playerLives; i++)
    {
        drawCircle(20 + i * 20, 580, 7, 1, 1, 1);
    }
}


void drawWalls()
{
    // Left wall
    drawRect(0, 0, 10, windowHeight, 0.3f, 0.3f, 0.3f);
    // Right wall
    drawRect(windowWidth - 10, 0, 10, windowHeight, 0.3f, 0.3f, 0.3f);
    // Top wall
    drawRect(0, windowHeight - 10, windowWidth, 10, 0.3f, 0.3f, 0.3f);
}

void drawMenu()
{

    drawRect(0, 0, windowWidth, windowHeight, 0, 0, 0.1f);

    drawTextLarge(250, 480, "DX BALL GAME", 1, 0.8f, 0);
    drawTextLarge(270, 450, "============", 0.5f, 0.5f, 0.5f);

    drawText(340, 380, "1. Start Game",    1, 1, 0);
    drawText(340, 340, "2. High Score",    0, 1, 0);
    drawText(340, 300, "3. Help",          0, 1, 1);
    drawText(340, 260, "4. Exit",          1, 0, 0);

    drawText(220, 150, "Press 1 / 2 / 3 / 4 to select", 0.7f, 0.7f, 0.7f);

    drawCircle(100, 400, 20, 1, 1, 0);
    drawCircle(700, 400, 20, 0, 1, 1);
    drawCircle(400, 200, 15, 1, 0, 0.5f);

}

void drawPaused()
{
    // Semi-transparent overlay
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(0, 0, 0, 0.6f);
    glBegin(GL_QUADS);
        glVertex2f(0, 0);
        glVertex2f(windowWidth, 0);
        glVertex2f(windowWidth, windowHeight);
        glVertex2f(0, windowHeight);
    glEnd();
    glDisable(GL_BLEND);

    drawTextLarge(310, 360, "PAUSED", 1, 1, 0);
    drawText(260, 310, "Press P to Resume", 1, 1, 1);
    drawText(260, 270, "Press R to Restart", 0, 1, 0);
    drawText(260, 230, "Press ESC / M for Menu", 1, 0, 0);
}

void drawGameOver()
{
    drawRect(0, 0, windowWidth, windowHeight, 0.1f, 0, 0);

    drawTextLarge(280, 400, "GAME OVER!", 1, 0, 0);
    drawText(260, 350, "Score: " + intToStr(playerScore),    1, 1, 0);
    drawText(260, 310, "High Score: " + intToStr(highScore), 0, 1, 0);
    drawText(260, 270, "Time: " + floatToStr(gameTime, 1) + " seconds", 0, 1, 1);

    drawText(240, 200, "Press R to Restart",  1, 1, 1);
    drawText(240, 160, "Press M for Menu",    0.7f, 0.7f, 0.7f);
    drawText(240, 120, "Press ESC to Exit",   1, 0, 0);
}

void drawWin()
{
    drawRect(0, 0, windowWidth, windowHeight, 0, 0.1f, 0);

    drawTextLarge(250, 420, "YOU WIN! Congratulations!", 0, 1, 0);
    drawText(260, 370, "Score: " + intToStr(playerScore),    1, 1, 0);
    drawText(260, 330, "High Score: " + intToStr(highScore), 0, 1, 1);
    drawText(260, 290, "Time: " + floatToStr(gameTime, 1) + " seconds", 1, 1, 1);

    drawText(260, 230, "Press R to Play Again", 1, 1, 1);
    drawText(260, 190, "Press M for Menu",      0.7f, 0.7f, 0.7f);
}

void drawHighScore()
{
    drawRect(0, 0, windowWidth, windowHeight, 0, 0, 0.15f);

    drawTextLarge(280, 480, "HIGH SCORE", 1, 0.8f, 0);

    drawText(300, 400, "Best Score : " + intToStr(highScore), 0, 1, 0);
    drawText(300, 360, "Keep Playing to Beat It!", 1, 1, 0);

    drawText(300, 200, "Press M to go back to Menu", 0.7f, 0.7f, 0.7f);
}

void drawHelp()
{
    drawRect(0, 0, windowWidth, windowHeight, 0, 0, 0.2f);

    drawTextLarge(300, 540, "HELP / CONTROLS", 1, 1, 0);

    drawText(80, 490, "LEFT ARROW / A  - Move paddle left",      1, 1, 1);
    drawText(80, 460, "RIGHT ARROW / D - Move paddle right",     1, 1, 1);
    drawText(80, 430, "SPACE           - Launch ball",           1, 1, 1);
    drawText(80, 400, "P               - Pause / Resume",        1, 1, 1);
    drawText(80, 370, "F               - Shoot bullet (if perk)",1, 1, 1);
    drawText(80, 340, "ESC / M         - Menu",                  1, 1, 1);
    drawText(80, 310, "Mouse Move      - Move paddle",           1, 1, 1);

    drawTextLarge(80, 270, "PERKS:", 0, 1, 0);
    drawText(80, 240, "+L (Green)  = Extra Life",                0,   1,   0  );
    drawText(80, 215, "+S (Yellow) = Speed Up Ball",             1,   1,   0  );
    drawText(80, 190, "+W (Cyan)   = Wide Paddle for 15 sec",    0,   1,   1  );
    drawText(80, 165, "FB (Orange) = Fireball (break through) 10s", 1,   0.5f,0  );
    drawText(80, 140, "-W (Red)    = Shrink Paddle (BAD!)",      1,   0,   0  );
    drawText(80, 115, "XX (Purple) = Instant Death (BAD!)",      0.5f,0,   0.5f);

    drawTextLarge(80, 75, "BRICK TYPES:", 1, 0.5f, 0);
    drawText(80, 50, "Normal=1 hit | Hard=2 hits | Wall(brown)=3 hits", 1, 1, 1);

    drawText(300, 20, "Press M to go back", 0.7f, 0.7f, 0.7f);
}


void drawGame()
{
    drawWalls();
    drawBricks();
    drawPaddle();
    drawBall();
    drawPerkItems();
    drawBullets();
    drawHUD();
}


void display()
{
    glClear(GL_COLOR_BUFFER_BIT);
    glLoadIdentity();

    if (gameState == STATE_MENU) drawMenu();
    else if (gameState == STATE_PLAYING)   { drawGame(); }

    else if (gameState == STATE_PAUSED)    { drawGame(); drawPaused(); }
    else if (gameState == STATE_GAME_OVER) drawGameOver();
    else if (gameState == STATE_WIN)       drawWin();
    else if (gameState == STATE_HELP)      drawHelp();

    else if (gameState == STATE_HIGHSCORE) drawHighScore();



    glutSwapBuffers();
}

float lastTime = 0.0f;

void timer(int value)
{
    float currentTime = glutGet(GLUT_ELAPSED_TIME) / 1000.0f;
    float deltaTime   = currentTime - lastTime;
    lastTime          = currentTime;

    if (deltaTime > 0.1f) deltaTime = 0.1f;

    update(deltaTime);
    glutPostRedisplay();

    glutTimerFunc(16, timer, 0);
}

void keyboard(unsigned char key, int x, int y)
{

    if (gameState == STATE_MENU)
    {
        if (key == '1')
        {
            soundMenu();
            resetGame();
            gameState = STATE_PLAYING;
        }
        else if (key == '2')
        {
            soundMenu();
            gameState = STATE_HIGHSCORE;
        }
        else if (key == '3')
        {
            soundMenu();
            gameState = STATE_HELP;
        }
        else if (key == '4' || key == 27)
        {
            exit(0);
        }
        return;
    }

    if (key == 27 || key == 'm' || key == 'M')
    {
        soundMenu();
        gameState = STATE_MENU;
        return;
    }

    if (gameState == STATE_HIGHSCORE)
    {
        return;
    }


    if (gameState == STATE_HELP)
    {
        return;
    }

    if (gameState == STATE_GAME_OVER || gameState == STATE_WIN)
    {
        if (key == 'r' || key == 'R')
        {
            soundMenu();
            resetGame();
            gameState = STATE_PLAYING;
        }
        return;
    }

    if (key == 'p' || key == 'P')
    {
        soundMenu();
        if (gameState == STATE_PLAYING)
            gameState = STATE_PAUSED;
        else if (gameState == STATE_PAUSED)
            gameState = STATE_PLAYING;
        return;
    }

    if (gameState == STATE_PAUSED)
    {
        if (key == 'r' || key == 'R')
        {
            soundMenu();
            resetGame();
            gameState = STATE_PLAYING;
        }
        return;
    }

    if (gameState == STATE_PLAYING)
    {
        if (key == ' ')
        {
            if (!ballLaunched)
            {
                ballLaunched = true;
                ballDX = 3.0f;
                ballDY = 4.0f;
                soundLaunch();
            }
        }

        if ((key == 'f' || key == 'F') && shootingPaddle)
        {
            Bullet b;
            b.x      = paddleX + paddleWidth / 2.0f;
            b.y      = paddleY + paddleHeight;
            b.active = true;
            bullets.push_back(b);
            soundBullet();
        }

        if (key == 'a' || key == 'A') moveLeft  = true;
        if (key == 'd' || key == 'D') moveRight = true;
    }


}

void keyboardUp(unsigned char key, int x, int y)
{
    if (key == 'a' || key == 'A') moveLeft  = false;
    if (key == 'd' || key == 'D') moveRight = false;
}

void specialKey(int key, int x, int y)
{
    if (gameState != STATE_PLAYING) return;

    if (key == GLUT_KEY_LEFT)  moveLeft  = true;
    if (key == GLUT_KEY_RIGHT) moveRight = true;
}

void specialKeyUp(int key, int x, int y)
{
    if (key == GLUT_KEY_LEFT)  moveLeft  = false;
    if (key == GLUT_KEY_RIGHT) moveRight = false;
}

void mouseMotion(int x, int y)
{
    if (gameState != STATE_PLAYING) return;


    paddleX = x - paddleWidth / 2.0f;


    if (paddleX < 10)
        paddleX = 10;
    if (paddleX + paddleWidth > windowWidth - 10)
        paddleX = windowWidth - 10 - paddleWidth;
}

void mouseClick(int button, int state, int x, int y)
{
    if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN)
    {
        if (gameState == STATE_PLAYING && !ballLaunched)
        {
            ballLaunched = true;
            ballDX = 3.0f;
            ballDY = 4.0f;
            soundLaunch();
        }
    }
}

void reshape(int w, int h)
{
    windowWidth  = w;
    windowHeight = h;

    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0, w, 0, h);
    glMatrixMode(GL_MODELVIEW);
}

int main(int argc, char** argv)
{
    srand((unsigned int)time(NULL));

    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
    glutInitWindowSize(windowWidth, windowHeight);
    glutInitWindowPosition(100, 50);
    glutCreateWindow("DX Ball Game - OpenGL");


    glClearColor(0, 0, 0, 1);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0, windowWidth, 0, windowHeight);
    glMatrixMode(GL_MODELVIEW);

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutKeyboardUpFunc(keyboardUp);
    glutSpecialFunc(specialKey);
    glutSpecialUpFunc(specialKeyUp);
    glutPassiveMotionFunc(mouseMotion);
    glutMotionFunc(mouseMotion);
    glutMouseFunc(mouseClick);

    lastTime = glutGet(GLUT_ELAPSED_TIME) / 1000.0f;
    glutTimerFunc(16, timer, 0);

    cout << "=== DX Ball Game ===" << endl;
    cout << "Controls:" << endl;
    cout << "  Arrow/A/D - Move paddle" << endl;
    cout << "  SPACE/Click - Launch ball" << endl;
    cout << "  P - Pause" << endl;
    cout << "  M/ESC - Menu" << endl;
    cout << "===================" << endl;

    glutMainLoop();
    return 0;
}
