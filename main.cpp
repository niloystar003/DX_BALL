// DXBall.cpp
// Complete DX Ball Game Implementation
// Beginner Friendly Code

#include <GL/glut.h>
#include <iostream>
#include <string>
#include <vector>
#include <cmath>
#include <ctime>
#include <cstdlib>
#include <sstream>

// ============================================================

using namespace std;

// ============================================================
// WINDOW SETTINGS
// ============================================================
int windowWidth  = 800;
int windowHeight = 600;

// ============================================================
// GAME STATES
// ============================================================
// We use simple integer constants for game state
#define STATE_MENU       0
#define STATE_PLAYING    1
#define STATE_PAUSED     2
#define STATE_GAME_OVER  3
#define STATE_WIN        4
#define STATE_HELP       5
#define STATE_HIGHSCORE  6

int gameState = STATE_MENU;

// ============================================================
// PADDLE SETTINGS
// ============================================================
float paddleX       = 350.0f;   // paddle left edge x
float paddleY       = 30.0f;    // paddle bottom edge y
float paddleWidth   = 100.0f;   // normal paddle width
float paddleHeight  = 15.0f;
float paddleSpeed   = 10.0f;

// ============================================================
// BALL SETTINGS
// ============================================================
float ballX      = 400.0f;
float ballY      = 60.0f;
float ballRadius = 10.0f;
float ballDX     = 3.0f;   // ball velocity x
float ballDY     = 4.0f;   // ball velocity y
bool  ballLaunched = false; // ball sticks to paddle until launched

// Speed increase timer
float ballSpeedTimer    = 0.0f;
float ballSpeedInterval = 15.0f; // increase speed every 15 seconds
float ballSpeedMax      = 12.0f;

// ============================================================
// BRICK SETTINGS
// ============================================================
#define BRICK_ROWS    7
#define BRICK_COLS    12
#define BRICK_WIDTH   58
#define BRICK_HEIGHT  22
#define BRICK_OFFSET_X 25
#define BRICK_OFFSET_Y 300  // bricks start from top area

// Brick types
#define BRICK_EMPTY    0
#define BRICK_NORMAL   1
#define BRICK_HARD     2   // needs 2 hits
#define BRICK_WALL     3   // needs 3 hits (brick wall)

struct Brick
{
    float x, y;           // position
    int   type;           // BRICK_NORMAL, BRICK_HARD, BRICK_WALL
    int   hits;           // how many hits remaining
    bool  active;         // is brick still alive?
    float r, g, b;        // color
};

Brick bricks[BRICK_ROWS][BRICK_COLS];

// ============================================================
// PERK / DROP ITEM SETTINGS
// ============================================================
// Perk types
#define PERK_NONE          0
#define PERK_EXTRA_LIFE    1   // grants extra life
#define PERK_SPEED_UP      2   // faster ball
#define PERK_WIDE_PADDLE   3   // wider paddle
#define PERK_FIREBALL      4   // ball breaks bricks without bouncing (for short time)
#define PERK_SHRINK_PADDLE 5   // paddle shrinks (damage)
#define PERK_INSTANT_DEATH 6   // lose a life immediately (damage)

struct PerkItem
{
    float x, y;
    float width, height;
    int   type;
    bool  active;
    float r, g, b;
    float fallSpeed;
};

vector<PerkItem> perkItems;  // list of falling perk items

// Perk effect timers
float widePaddleTimer  = 0.0f;   // time remaining for wide paddle
float fireballTimer    = 0.0f;   // time remaining for fireball
bool  isFireball       = false;
bool  isWidePaddle     = false;


// ============================================================
// SHOOTING PADDLE
// ============================================================
// (Optional advanced feature - simple bullets from paddle)
struct Bullet
{
    float x, y;
    bool  active;
};
vector<Bullet> bullets;
bool shootingPaddle = false;
float shootingTimer = 0.0f;

// ============================================================
// HELPER: Draw a filled rectangle
// ============================================================

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

// ============================================================
// HELPER: Draw rectangle border / outline
// ============================================================
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

// ============================================================
// HELPER: Draw a filled circle
// ============================================================
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
// ============================================================
// HELPER: Draw Text on screen
// ============================================================

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

// ============================================================
// HELPER: int to string
// ============================================================
string intToStr(int val)
{
    stringstream ss;
    ss << val;
    return ss.str();
}

// ============================================================
// DRAW BRICKS
// ============================================================
void drawBricks()
{
    for (int r = 0; r < BRICK_ROWS; r++)
    {
        for (int c = 0; c < BRICK_COLS; c++)
        {
            Brick &b = bricks[r][c];
            if (!b.active) continue;

            // Fill brick
            drawRect(b.x, b.y, BRICK_WIDTH, BRICK_HEIGHT, b.r, b.g, b.b);

            // Outline
            drawRectOutline(b.x, b.y, BRICK_WIDTH, BRICK_HEIGHT, 0.0f, 0.0f, 0.0f);

            // Show hits remaining for hard/wall bricks
            if (b.type == BRICK_HARD || b.type == BRICK_WALL)
            {
                string hitsStr = intToStr(b.hits);
                drawText(b.x + BRICK_WIDTH/2 - 4, b.y + 6, hitsStr, 1, 1, 1);
            }
        }
    }
}


// ============================================================
// DRAW PADDLE
// ============================================================
void drawPaddle()
{
    // Main paddle color
    float pr = 0.5f, pg = 0.5f, pb = 1.0f;

    if (isFireball)    { pr=1; pg=0.4f; pb=0; }
    if (isWidePaddle)  { pr=0; pg=1;    pb=1;  }

    drawRect(paddleX, paddleY, paddleWidth, paddleHeight, pr, pg, pb);
    drawRectOutline(paddleX, paddleY, paddleWidth, paddleHeight, 1, 1, 1);
}

// ============================================================
// DRAW BALL
// ============================================================
void drawBall()
{
    float br = 1.0f, bg = 1.0f, bb = 1.0f;
    if (isFireball) { br=1; bg=0.4f; bb=0; }

    drawCircle(ballX, ballY, ballRadius, br, bg, bb);
    // Inner highlight
    drawCircle(ballX - 3, ballY + 3, ballRadius * 0.4f, 1, 1, 1);
}

// ============================================================
// DRAW PERK ITEMS (falling)
// ============================================================
void drawPerkItems()
{
    for (int i = 0; i < (int)perkItems.size(); i++)
    {
        PerkItem &p = perkItems[i];
        if (!p.active) continue;

        drawRect(p.x, p.y, p.width, p.height, p.r, p.g, p.b);
        drawRectOutline(p.x, p.y, p.width, p.height, 1, 1, 1);

        // Label
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

// ============================================================
// DRAW BULLETS
// ============================================================
void drawBullets()
{
    for (int i = 0; i < (int)bullets.size(); i++)
    {
        Bullet &blt = bullets[i];
        if (!blt.active) continue;
        drawRect(blt.x - 2, blt.y, 4, 10, 1, 1, 0);
    }
}


// ============================================================
// DRAW SIDE WALLS
// ============================================================
void drawWalls()
{
    // Left wall
    drawRect(0, 0, 10, windowHeight, 0.3f, 0.3f, 0.3f);
    // Right wall
    drawRect(windowWidth - 10, 0, 10, windowHeight, 0.3f, 0.3f, 0.3f);
    // Top wall
    drawRect(0, windowHeight - 10, windowWidth, 10, 0.3f, 0.3f, 0.3f);
}

// ============================================================
// DRAW MENU PAGE
// ============================================================
void drawMenu()
{
    // Background
    drawRect(0, 0, windowWidth, windowHeight, 0, 0, 0.1f);

    // Title
    drawTextLarge(250, 480, "DX BALL GAME", 1, 0.8f, 0);
    drawTextLarge(270, 450, "============", 0.5f, 0.5f, 0.5f);

    // Menu options
    drawText(340, 380, "1. Start Game",    1, 1, 0);
    drawText(340, 340, "2. High Score",    0, 1, 0);
    drawText(340, 300, "3. Help",          0, 1, 1);
    drawText(340, 260, "4. Exit",          1, 0, 0);

    drawText(220, 150, "Press 1 / 2 / 3 / 4 to select", 0.7f, 0.7f, 0.7f);

    // Decorative balls
    drawCircle(100, 400, 20, 1, 1, 0);
    drawCircle(700, 400, 20, 0, 1, 1);
    drawCircle(400, 200, 15, 1, 0, 0.5f);

}


// ============================================================
// DRAW PLAYING GAME SCREEN
// ============================================================
void drawGame()
{
    drawWalls();
    drawBricks();
    drawPaddle();
    drawBall();
    drawPerkItems();

    drawBullets();
}


// ============================================================
// DISPLAY CALLBACK
// ============================================================
void display()
{
    glClear(GL_COLOR_BUFFER_BIT);
    glLoadIdentity();

    if (gameState == STATE_MENU) drawMenu();
    else if (gameState == STATE_PLAYING)   { drawGame(); }


    glutSwapBuffers();
}
// ============================================================
// RESHAPE CALLBACK
// ============================================================
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
// ============================================================
// MAIN FUNCTION
// ============================================================
int main(int argc, char** argv)
{
    srand((unsigned int)time(NULL));

    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
    glutInitWindowSize(windowWidth, windowHeight);
    glutInitWindowPosition(100, 50);
    glutCreateWindow("DX Ball Game - OpenGL");


    // Setup OpenGL
    glClearColor(0, 0, 0, 1);


    // Setup projection (2D)
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0, windowWidth, 0, windowHeight);
    glMatrixMode(GL_MODELVIEW);


    // Register callbacks
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);

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
