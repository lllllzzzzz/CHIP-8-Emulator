#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctype.h>
#include <GL/freeglut.h>
// #include <SFML/Audio.hpp>
#include "chip8.hpp"

#define DEBUG
#define WIN_WIDTH	64
#define WIN_HEIGHT	32
#define timerMs(x)	((x*1000) / timerFreq)

// Globals
Chip8 			g_Chip8;
bool 			isPaused = false;
unsigned int 	cyclesPerSecond = 600;
unsigned int 	screenWidth = 640;
unsigned int 	screenHeight = 320;
unsigned char 	videoBuffer[WIN_HEIGHT][WIN_WIDTH][3];

// GLUT callbacks
void setupTexture();
void updateTexture();
void renderFrame();
void reshapeWindow(GLsizei w, GLsizei h);
void keyboardDown(unsigned char key, int x, int y);
void keyboardUp(unsigned char key, int x, int y);

int main(int argc, char *argv[])
{
    // Read command line args
    if (argc < 2 || argc > 3) {
        std::cerr << "CHIP-8 Emulator by Luke Zimmerer\nUsage: " 
                  << argv[0] << " [filename] [-f]\n\n"
                  << "Options:\n"
                  << "  -f    fullscreen\n";
        return EXIT_FAILURE;
    }

    bool isFullScreen = false;
    if (argc == 3 && !strcmp(argv[2], "-f")) {
        isFullScreen = true;
    }

    if(!g_Chip8.Initialize()) {
        std::cerr << "Error initializing emulator!\n";
        return EXIT_FAILURE;
    }

    if(!g_Chip8.LoadRom(argv[1])) {
        std::cerr << "Error loading rom!\n";
        return EXIT_FAILURE;
    }

    int glut_argc = 2;
    const char *glut_argv[] = {"foo", "bar"};
    glutInit(&glut_argc, const_cast<char**>
    	(reinterpret_cast<const char**>(glut_argv)));

    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);
    glutInitWindowSize(screenWidth, screenHeight);
    glutInitWindowPosition(100, 100);
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

    // Load sound
    // sf::SoundBuffer buffer;
    // if (!buffer.loadFromFile("beep.wav")) {
    //     std::cerr << "Error loading beep.wav!\n";
    // }

    glutMainLoop();
    // return EXIT_SUCCESS;
}

void setupTexture()
{
    for(int y = 0; y < WIN_HEIGHT; y++) {
        for(int x = 0; x < WIN_WIDTH; x++) {
            videoBuffer[y][x][0] = 0;
            videoBuffer[y][x][1] = 0;
            videoBuffer[y][x][2] = 0;
        }
    }

    glTexImage2D(GL_TEXTURE_2D, 0, 3, WIN_WIDTH, WIN_HEIGHT, 
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

    for(int y = 0; y < WIN_HEIGHT; y++) {
        for(int x = 0; x < WIN_WIDTH; x++) {
            if(gfx[(y * WIN_WIDTH) + x] == 0) {
                videoBuffer[y][x][0] = 0;
                videoBuffer[y][x][1] = 0;
                videoBuffer[y][x][2] = 0;
            } else {
                videoBuffer[y][x][0] = 255;
                videoBuffer[y][x][1] = 255;
                videoBuffer[y][x][2] = 255;
            }
        }
    }

    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, WIN_WIDTH, WIN_HEIGHT, 
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
    // Execute cycles and tick timers
    g_Chip8.EmulateCycles(cyclesPerSecond / 60);
    g_Chip8.TickDelayTimer();
    g_Chip8.TickSoundTimer();

    // If draw flag is set, update screen
    if(g_Chip8.GetFlag(CPU_FLAG_DRAW)) {
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
    if (key == 0x1B) { // ESC key
        //exit(0);
        glutLeaveMainLoop(); // ~ key
    } else if (key == 0x60) {
        //g_Chip8.Reset();
        g_Chip8.ToggleFlag(CPU_FLAG_PAUSED);
    } else if (key == 0x31) { // 1 key
        g_Chip8.DumpRegisters();
    } else if (key == 0x35) { // 5 key
        g_Chip8.Step();
    }

    //printf("%#02X\n", key);

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
		0x56  // 'V'
	};

    char keyState[16];
    memset(keyState, 0, 16);
    // Read keypad input
	for (int i = 0; i < 16; i++) {
        if (toupper(key) == keyList[i]) {
            g_Chip8.SetFlag(CPU_FLAG_KEYDOWN);
            keyState[i] = 1;
            break;
        }
    }

    // int i = 0;
    // while (keyState[i++] = (toupper(key) == keyList[i])) {
    //     g_Chip8.SetFlag(CPU_FLAG_KEYDOWN);
    // }

    g_Chip8.SetKeys(keyState);
}

void keyboardUp(unsigned char key, int x, int y)
{
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

    char keyState[16];
    memset (keyState, 0, 16);
    // Read keypad input
    for (int i = 0; i < 16; i++) {
        if(key == keyList[i]) {
            g_Chip8.ResetFlag(CPU_FLAG_KEYDOWN);
            keyState[i] = 0;
            break;
        }
    }

	// for (int i = 0; i < 16; i++) {
 //        if (g_Chip8.GetFlag(CPU_FLAG_KEYDOWN)) {
 //            g_Chip8.ResetFlag(CPU_FLAG_KEYDOWN);
 //            keyState[i] = key == keyList[i];
 //        }
        
 //    }

    g_Chip8.SetKeys(keyState);
}
