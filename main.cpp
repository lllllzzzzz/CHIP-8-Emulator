#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctype.h>
#include <GL/freeglut.h>
// #include <SFML/Audio.hpp>
#include "chip8.hpp"

#define MIN_GOOD_ARGS 2
#define MAX_GOOD_ARGS 3
#define DEBUG
#define timerMs(x)  ((x * 1000) / timerFreq)

// Globals
namespace {
    const unsigned int WINDOW_WIDTH    = 64;
    const unsigned int WINDOW_HEIGHT   = 32;
    const unsigned int WINDOW_X        = 100;
    const unsigned int WINDOW_Y        = 100;
    int                screenWidth     = 640;
    int                screenHeight    = 320;
    const unsigned int BG_COLOUR       = 0x00000000;
    const unsigned int FG_COLOUR       = 0xFFFFFFFF;
    int                cyclesPerSecond = 600
    bool               isPaused        = false;
    unsigned char      videoBuffer[WINDOW_HEIGHT][WINDOW_WIDTH][3];
    Chip8              g_Chip8;
}

// GLUT callbacks
void setupTexture();
void updateTexture();
void renderFrame();
void reshapeWindow(GLsizei w, GLsizei h);
void keyboardDown(unsigned char key, int x, int y);
void keyboardUp(unsigned char key, int x, int y);

int main(int argc, char *argv[])
{
    if (argc < MIN_GOOD_ARGS || argc > MAX_GOOD_ARGS) {
        std::cerr << "CHIP-8 Emulator by Luke Zimmerer\nUsage: " 
                  << argv[0] << " [filename] [-f]\n\n"
                  << "Options:\n"
                  << "  -f    fullscreen\n";
        return EXIT_FAILURE;
    }

    bool isFullScreen = (argc == MAX_GOOD_ARGS && !strcmp(argv[2], "-f"));

    if (!g_Chip8.Initialize()) {
        std::cerr << "Error initializing emulator!\n";
        return EXIT_FAILURE;
    }

    if (!g_Chip8.LoadRom(argv[1])) {
        std::cerr << "Error loading rom!\n";
        return EXIT_FAILURE;
    }

    const int glut_argc = 2;
    const char *glut_argv[] = {"foo", "bar"};
    glutInit(&glut_argc, const_cast<char**>
      (reinterpret_cast<const char**>(glut_argv)));

    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);
    glutInitWindowSize(screenWidth, screenHeight);
    glutInitWindowPosition(WINDOW_X, WINDOW_Y);
    glutCreateWindow(argv[1]);

    if (isFullScreen) {
        glutFullScreen();
    }
    
    glutDisplayFunc(renderFrame);
    glutIdleFunc(renderFrame);
    glutReshapeFunc(reshapeWindow);
    glutKeyboardFunc(keyboardDown);
    glutKeyboardUpFunc(keyboardUp);

    setupTexture();

    // SOUND NOT YET IMPLEMENTED
    // Load sound
    // sf::SoundBuffer buffer;
    // if (!buffer.loadFromFile("beep.wav")) {
    //     std::cerr << "Error loading beep.wav!\n";
    // }

    glutMainLoop();
}

void setupTexture()
{
    for (int y = 0; y < WINDOW_HEIGHT; y++) {
        for (int x = 0; x < WINDOW_WIDTH; x++) {
            videoBuffer[y][x][0] = 0;
            videoBuffer[y][x][1] = 0;
            videoBuffer[y][x][2] = 0;
        }
    }

    glTexImage2D(GL_TEXTURE_2D, 0, 3, WINDOW_WIDTH, WINDOW_HEIGHT, 
                 0, GL_RGB, GL_UNSIGNED_BYTE, 
                 reinterpret_cast<GLvoid*>(videoBuffer));

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

    glEnable(GL_TEXTURE_2D);
}

void updateTexture()
{
    unsigned char *gfx = g_Chip8.GetVRAM();
    if (!gfx) {
        return;
    }

    for (int y = 0; y < WINDOW_HEIGHT; y++) {
        for (int x = 0; x < WINDOW_WIDTH; x++) {
            if (gfx[(y * WINDOW_WIDTH) + x] == 0) {
                videoBuffer[y][x][0] = BG_COLOUR;
                videoBuffer[y][x][1] = BG_COLOUR;
                videoBuffer[y][x][2] = BG_COLOUR;
            } else {
                videoBuffer[y][x][0] = FG_COLOUR;
                videoBuffer[y][x][1] = FG_COLOUR;
                videoBuffer[y][x][2] = FG_COLOUR;
            }
        }
    }

    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, 
                    GL_RGB, GL_UNSIGNED_BYTE, 
                    reinterpret_cast<GLvoid*>(videoBuffer));

    glBegin(GL_QUADS);

    glTexCoord2d(0.0, 0.0);
    glVertex2d(0.0, 0.0);

    glTexCoord2d(1.0, 0.0);
    glVertex2d(screenWidth, 0.0);

    glTexCoord2d(1.0, 1.0);
    glVertex2d(screenWidth, screenHeight);

    glTexCoord2d(0.0, 1.0);
    glVertex2d(0.0, screenHeight);

    glEnd();
}

void renderFrame()
{
    g_Chip8.EmulateCycles(cyclesPerSecond / 60);
    g_Chip8.TickDelayTimer();
    g_Chip8.TickSoundTimer();

    if (g_Chip8.GetFlag(CPU_FLAG_DRAW)) {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        updateTexture();
        glutSwapBuffers();
        g_Chip8.SetFlag(CPU_FLAG_DRAW);
    }
}

void reshapeWindow(GLsizei w, GLsizei h)
{
    glClearColor(0.0f, 0.0f, 0.5f, 0.0f);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0, w, h, 0);
    glMatrixMode(GL_MODELVIEW);
    glViewport(0, 0, w, h);

    screenWidth = w;
    screenHeight = h;
}

void keyboardDown(unsigned char key, int x, int y)
{
  const int NUM_KEYS = 16;

  if (key == 0x1B) {                        // ESC
      glutLeaveMainLoop();                  
  } else if (key == 0x60) {                 // ~
      g_Chip8.ToggleFlag(CPU_FLAG_PAUSED);
  } else if (key == 0x31) {                 // 1
      g_Chip8.DumpRegisters();
  } else if (key == 0x35) {                 // 5
      g_Chip8.Step();
  }

  const int keyList[] = {
    0x31, // '1'
    0x32, // '2'
    0x33, // '3'
    0x34, // '4'
    0x51, // 'Q'
    0x57, // 'W'
    0x45, // 'E'
    0x52, // 'R'
    0x41, // 'A'
    0x53, // 'S'
    0x44, // 'D'
    0x46, // 'F'
    0x5A, // 'Z'
    0x58, // 'X'
    0x43, // 'C'
    0x56  // 'V
  };

  char keyState[NUM_KEYS] = {0};
  // Read keypad input
  for (int i = 0; i < NUM_KEYS; i++) {
      if (toupper(key) == keyList[i]) {
          g_Chip8.SetFlag(CPU_FLAG_KEYDOWN);
          keyState[i] = 1;
          break;
      }
  }

  g_Chip8.SetKeys(keyState);
}

void keyboardUp(unsigned char key, int x, int y)
{
  const int NUM_KEYS = 16;

  const int keyList[] = {
    0x31, // '1'
    0x32, // '2'
    0x33, // '3'
    0x34, // '4'
    0x51, // 'Q'
    0x57, // 'W'
    0x45, // 'E'
    0x52, // 'R'
    0x41, // 'A'
    0x53, // 'S'
    0x44, // 'D'
    0x46, // 'F'
    0x5A, // 'Z'
    0x58, // 'X'
    0x43, // 'C'
    0x56, // 'V'
  };

  char keyState[NUM_KEYS] = {0};
  // Read keypad input
  for (int i = 0; i < NUM_KEYS; i++) {
      if (key == keyList[i]) {
          g_Chip8.ResetFlag(CPU_FLAG_KEYDOWN);
          keyState[i] = 0;
          break;
      }
  }

  g_Chip8.SetKeys(keyState);
}
