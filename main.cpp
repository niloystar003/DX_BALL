
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
// DISPLAY CALLBACK
// ============================================================
void display()
{
    glClear(GL_COLOR_BUFFER_BIT);
    glLoadIdentity();

    if (gameState == STATE_MENU) drawMenu();


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
