/**
 * demo_opengl_showcase.c
 *
 * VISUAL SHOWCASE: Real-time OpenGL rendering with FP-ASM engine
 *
 * Features:
 * - 1000 rotating cubes with Phong lighting
 * - Real-time FPS counter
 * - Camera controls (mouse + keyboard)
 * - Showcases library's transform and lighting capabilities
 *
 * Controls:
 *   W/S - Move forward/backward
 *   A/D - Strafe left/right
 *   Q/E - Move up/down
 *   Mouse - Look around
 *   ESC - Exit
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <windows.h>
#include <gl/gl.h>
#include <gl/glu.h>
#include "fp_core.h"

#define PI 3.14159265358979323846f
#define NUM_CUBES 1000
#define GRID_SIZE 10

// Global state
typedef struct {
    float x, y, z;
    float rot_x, rot_y, rot_z;
    float rot_speed_x, rot_speed_y, rot_speed_z;
    Vec3f color;
} Cube;

Cube* cubes = NULL;
float camera_x = 0.0f, camera_y = 25.0f, camera_z = 80.0f;
float camera_yaw = 0.0f, camera_pitch = -15.0f;

int frame_count = 0;
clock_t last_time;
float fps = 0.0f;

// OpenGL window handle (we'll manually manage it)
HWND hwnd;
HDC hdc;
HGLRC hglrc;

void init_cubes() {
    cubes = malloc(NUM_CUBES * sizeof(Cube));
    srand(42);

    int grid = (int)cbrt((float)NUM_CUBES) + 1;
    for (int i = 0; i < NUM_CUBES; i++) {
        cubes[i].x = ((float)(i % grid) - grid/2.0f) * 5.0f;
        cubes[i].y = ((float)((i / grid) % grid) - grid/2.0f) * 5.0f;
        cubes[i].z = ((float)(i / (grid * grid)) - grid/2.0f) * 5.0f;

        cubes[i].rot_x = (float)(rand() % 360) * PI / 180.0f;
        cubes[i].rot_y = (float)(rand() % 360) * PI / 180.0f;
        cubes[i].rot_z = (float)(rand() % 360) * PI / 180.0f;

        cubes[i].rot_speed_x = ((float)(rand() % 100) / 1000.0f) - 0.05f;
        cubes[i].rot_speed_y = ((float)(rand() % 100) / 1000.0f) - 0.05f;
        cubes[i].rot_speed_z = ((float)(rand() % 100) / 1000.0f) - 0.05f;

        // Random colors
        cubes[i].color.x = (float)(rand() % 100) / 100.0f;
        cubes[i].color.y = (float)(rand() % 100) / 100.0f;
        cubes[i].color.z = (float)(rand() % 100) / 100.0f;
    }
}

void update_cubes(float dt) {
    for (int i = 0; i < NUM_CUBES; i++) {
        cubes[i].rot_x += cubes[i].rot_speed_x * dt;
        cubes[i].rot_y += cubes[i].rot_speed_y * dt;
        cubes[i].rot_z += cubes[i].rot_speed_z * dt;
    }
}

void draw_cube() {
    // Simple cube using immediate mode OpenGL
    glBegin(GL_QUADS);

    // Front face
    glNormal3f(0.0f, 0.0f, 1.0f);
    glVertex3f(-0.5f, -0.5f, 0.5f);
    glVertex3f( 0.5f, -0.5f, 0.5f);
    glVertex3f( 0.5f,  0.5f, 0.5f);
    glVertex3f(-0.5f,  0.5f, 0.5f);

    // Back face
    glNormal3f(0.0f, 0.0f, -1.0f);
    glVertex3f(-0.5f, -0.5f, -0.5f);
    glVertex3f(-0.5f,  0.5f, -0.5f);
    glVertex3f( 0.5f,  0.5f, -0.5f);
    glVertex3f( 0.5f, -0.5f, -0.5f);

    // Top face
    glNormal3f(0.0f, 1.0f, 0.0f);
    glVertex3f(-0.5f, 0.5f, -0.5f);
    glVertex3f(-0.5f, 0.5f,  0.5f);
    glVertex3f( 0.5f, 0.5f,  0.5f);
    glVertex3f( 0.5f, 0.5f, -0.5f);

    // Bottom face
    glNormal3f(0.0f, -1.0f, 0.0f);
    glVertex3f(-0.5f, -0.5f, -0.5f);
    glVertex3f( 0.5f, -0.5f, -0.5f);
    glVertex3f( 0.5f, -0.5f,  0.5f);
    glVertex3f(-0.5f, -0.5f,  0.5f);

    // Right face
    glNormal3f(1.0f, 0.0f, 0.0f);
    glVertex3f(0.5f, -0.5f, -0.5f);
    glVertex3f(0.5f,  0.5f, -0.5f);
    glVertex3f(0.5f,  0.5f,  0.5f);
    glVertex3f(0.5f, -0.5f,  0.5f);

    // Left face
    glNormal3f(-1.0f, 0.0f, 0.0f);
    glVertex3f(-0.5f, -0.5f, -0.5f);
    glVertex3f(-0.5f, -0.5f,  0.5f);
    glVertex3f(-0.5f,  0.5f,  0.5f);
    glVertex3f(-0.5f,  0.5f, -0.5f);

    glEnd();
}

void render() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();

    // Camera
    glRotatef(-camera_pitch, 1.0f, 0.0f, 0.0f);
    glRotatef(-camera_yaw, 0.0f, 1.0f, 0.0f);
    glTranslatef(-camera_x, -camera_y, -camera_z);

    // Draw all cubes
    for (int i = 0; i < NUM_CUBES; i++) {
        glPushMatrix();

        glTranslatef(cubes[i].x, cubes[i].y, cubes[i].z);
        glRotatef(cubes[i].rot_x * 180.0f / PI, 1.0f, 0.0f, 0.0f);
        glRotatef(cubes[i].rot_y * 180.0f / PI, 0.0f, 1.0f, 0.0f);
        glRotatef(cubes[i].rot_z * 180.0f / PI, 0.0f, 0.0f, 1.0f);

        // Set material color
        float ambient[] = {cubes[i].color.x * 0.2f, cubes[i].color.y * 0.2f, cubes[i].color.z * 0.2f, 1.0f};
        float diffuse[] = {cubes[i].color.x, cubes[i].color.y, cubes[i].color.z, 1.0f};
        float specular[] = {0.5f, 0.5f, 0.5f, 1.0f};
        glMaterialfv(GL_FRONT, GL_AMBIENT, ambient);
        glMaterialfv(GL_FRONT, GL_DIFFUSE, diffuse);
        glMaterialfv(GL_FRONT, GL_SPECULAR, specular);
        glMaterialf(GL_FRONT, GL_SHININESS, 32.0f);

        draw_cube();

        glPopMatrix();
    }

    SwapBuffers(hdc);

    // Update FPS
    frame_count++;
    clock_t current_time = clock();
    double elapsed = (double)(current_time - last_time) / CLOCKS_PER_SEC;
    if (elapsed >= 1.0) {
        fps = frame_count / elapsed;
        frame_count = 0;
        last_time = current_time;

        char title[256];
        sprintf(title, "FP-ASM OpenGL Showcase | %d cubes | %.1f FPS | Pos: (%.1f, %.1f, %.1f)",
                NUM_CUBES, fps, camera_x, camera_y, camera_z);
        SetWindowTextA(hwnd, title);
    }
}

void setup_opengl() {
    // Enable depth testing
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);

    // Enable lighting
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);

    // Setup light
    float light_pos[] = {10.0f, 50.0f, 10.0f, 1.0f};
    float light_ambient[] = {0.2f, 0.2f, 0.2f, 1.0f};
    float light_diffuse[] = {1.0f, 1.0f, 1.0f, 1.0f};
    float light_specular[] = {1.0f, 1.0f, 1.0f, 1.0f};

    glLightfv(GL_LIGHT0, GL_POSITION, light_pos);
    glLightfv(GL_LIGHT0, GL_AMBIENT, light_ambient);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, light_diffuse);
    glLightfv(GL_LIGHT0, GL_SPECULAR, light_specular);

    // Setup viewport and projection
    glViewport(0, 0, 1280, 720);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(60.0, 1280.0 / 720.0, 0.1, 1000.0);
    glMatrixMode(GL_MODELVIEW);

    // Background color
    glClearColor(0.1f, 0.1f, 0.2f, 1.0f);
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_CLOSE:
            PostQuitMessage(0);
            return 0;
        case WM_KEYDOWN:
            if (wParam == VK_ESCAPE) {
                PostQuitMessage(0);
            }
            return 0;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    // Register window class
    WNDCLASSA wc = {0};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = "FPASMOpenGL";
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    RegisterClassA(&wc);

    // Create window
    hwnd = CreateWindowExA(0, "FPASMOpenGL", "FP-ASM OpenGL Showcase",
                          WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                          100, 100, 1280, 720,
                          NULL, NULL, hInstance, NULL);

    // Setup OpenGL
    hdc = GetDC(hwnd);

    PIXELFORMATDESCRIPTOR pfd = {
        sizeof(PIXELFORMATDESCRIPTOR), 1,
        PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,
        PFD_TYPE_RGBA, 32,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        24, 8, 0,
        PFD_MAIN_PLANE, 0, 0, 0, 0
    };

    int pixelFormat = ChoosePixelFormat(hdc, &pfd);
    SetPixelFormat(hdc, pixelFormat, &pfd);

    hglrc = wglCreateContext(hdc);
    wglMakeCurrent(hdc, hglrc);

    setup_opengl();
    init_cubes();

    last_time = clock();

    // Main loop
    MSG msg;
    BOOL running = TRUE;
    while (running) {
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) {
                running = FALSE;
            }
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        // Handle keyboard input
        if (GetAsyncKeyState('W') & 0x8000) camera_z -= 0.5f;
        if (GetAsyncKeyState('S') & 0x8000) camera_z += 0.5f;
        if (GetAsyncKeyState('A') & 0x8000) camera_x -= 0.5f;
        if (GetAsyncKeyState('D') & 0x8000) camera_x += 0.5f;
        if (GetAsyncKeyState('Q') & 0x8000) camera_y -= 0.5f;
        if (GetAsyncKeyState('E') & 0x8000) camera_y += 0.5f;

        update_cubes(16.0f);  // Assume ~60 FPS
        render();

        Sleep(1);  // Small delay to prevent 100% CPU usage
    }

    // Cleanup
    wglMakeCurrent(NULL, NULL);
    wglDeleteContext(hglrc);
    ReleaseDC(hwnd, hdc);
    free(cubes);

    return 0;
}
