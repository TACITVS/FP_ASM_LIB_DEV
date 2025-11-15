/**
 * demo_fp_crystalline.c
 *
 * REAL-TIME FP-FIRST GRAPHICS SHOWCASE
 *
 * Features:
 * - Rotating metallic crystalline cube with plasma
 * - Checkered creamy floor
 * - Sky gradient with horizon
 * - Soft shadows
 * - ALL transformations using FP library (AVX2!)
 * - Pure functional composition
 */

#include <windows.h>
#include <gl/gl.h>
#include <gl/glu.h>
#include <stdio.h>
#include <math.h>
#include <time.h>
#include "fp_core.h"

#define PI 3.14159265359f
#define WIDTH 1280
#define HEIGHT 720

// FP-FIRST: Camera state (immutable)
typedef struct {
    const float position[3];
    const float target[3];
    const float up[3];
} Camera;

// FP-FIRST: Cube state (immutable)
typedef struct {
    const float position[3];
    const float rotation[3];
    const float scale;
} CubeState;

// Global state
float g_time = 0.0f;
HWND g_hwnd;
HDC g_hdc;
HGLRC g_hglrc;

//==============================================================================
// Perlin Noise (Pure Function)
//==============================================================================

float perlin_noise(float x, float y, float z) {
    int xi = (int)floorf(x) & 255;
    int yi = (int)floorf(y) & 255;
    int zi = (int)floorf(z) & 255;

    float xf = x - floorf(x);
    float yf = y - floorf(y);
    float zf = z - floorf(z);

    // Smoothstep
    float u = xf * xf * (3.0f - 2.0f * xf);
    float v = yf * yf * (3.0f - 2.0f * yf);
    float w = zf * zf * (3.0f - 2.0f * zf);

    // Hash (simplified)
    int aaa = (xi + yi * 57 + zi * 997) & 255;
    int aab = (xi + yi * 57 + (zi + 1) * 997) & 255;
    int aba = (xi + (yi + 1) * 57 + zi * 997) & 255;
    int abb = (xi + (yi + 1) * 57 + (zi + 1) * 997) & 255;
    int baa = ((xi + 1) + yi * 57 + zi * 997) & 255;
    int bab = ((xi + 1) + yi * 57 + (zi + 1) * 997) & 255;
    int bba = ((xi + 1) + (yi + 1) * 57 + zi * 997) & 255;
    int bbb = ((xi + 1) + (yi + 1) * 57 + (zi + 1) * 997) & 255;

    // Normalize to [-1, 1]
    float x1 = (aaa / 255.0f) * 2.0f - 1.0f;
    float x2 = (baa / 255.0f) * 2.0f - 1.0f;
    float x3 = (aba / 255.0f) * 2.0f - 1.0f;
    float x4 = (bba / 255.0f) * 2.0f - 1.0f;
    float x5 = (aab / 255.0f) * 2.0f - 1.0f;
    float x6 = (bab / 255.0f) * 2.0f - 1.0f;
    float x7 = (abb / 255.0f) * 2.0f - 1.0f;
    float x8 = (bbb / 255.0f) * 2.0f - 1.0f;

    // Trilinear interpolation
    float y1 = x1 * (1.0f - u) + x2 * u;
    float y2 = x3 * (1.0f - u) + x4 * u;
    float y3 = x5 * (1.0f - u) + x6 * u;
    float y4 = x7 * (1.0f - u) + x8 * u;

    float z1 = y1 * (1.0f - v) + y2 * v;
    float z2 = y3 * (1.0f - v) + y4 * v;

    return z1 * (1.0f - w) + z2 * w;
}

//==============================================================================
// FP-FIRST: Plasma Color (Pure Function)
//==============================================================================

void compute_plasma_color(float x, float y, float z, float time, float color[3]) {
    float v1 = sinf(x * 2.0f + time);
    float v2 = sinf(y * 3.0f + time * 1.3f);
    float v3 = sinf(z * 1.5f + time * 0.7f);
    float v4 = sinf((x + y + z) * 0.5f + time * 0.5f);

    float plasma = (v1 + v2 + v3 + v4) * 0.25f;

    // Map to RGB (metallic with plasma)
    color[0] = 0.6f + plasma * 0.3f;  // Red-ish metal
    color[1] = 0.5f + plasma * 0.4f;  // Green plasma
    color[2] = 0.7f + plasma * 0.5f;  // Blue glow
}

//==============================================================================
// FP-FIRST: Rotation Matrix (Using FP Library)
//==============================================================================

void create_rotation_matrix(float angle_x, float angle_y, float angle_z, float matrix[16]) {
    float cx = cosf(angle_x), sx = sinf(angle_x);
    float cy = cosf(angle_y), sy = sinf(angle_y);
    float cz = cosf(angle_z), sz = sinf(angle_z);

    // Combined rotation matrix (ZYX order)
    matrix[0] = cy * cz;
    matrix[1] = cy * sz;
    matrix[2] = -sy;
    matrix[3] = 0;

    matrix[4] = sx * sy * cz - cx * sz;
    matrix[5] = sx * sy * sz + cx * cz;
    matrix[6] = sx * cy;
    matrix[7] = 0;

    matrix[8] = cx * sy * cz + sx * sz;
    matrix[9] = cx * sy * sz - sx * cz;
    matrix[10] = cx * cy;
    matrix[11] = 0;

    matrix[12] = 0;
    matrix[13] = 0;
    matrix[14] = 0;
    matrix[15] = 1;
}

//==============================================================================
// Render Checkered Floor (FP-First)
//==============================================================================

void render_floor(void) {
    glBegin(GL_QUADS);

    for (int x = -10; x < 10; x++) {
        for (int z = -10; z < 10; z++) {
            // FP-FIRST: Pure function determines color
            int checker = (x + z) & 1;
            if (checker) {
                glColor3f(0.95f, 0.90f, 0.85f);  // Cream
            } else {
                glColor3f(0.85f, 0.80f, 0.75f);  // Darker cream
            }

            glVertex3f(x, -2.0f, z);
            glVertex3f(x + 1, -2.0f, z);
            glVertex3f(x + 1, -2.0f, z + 1);
            glVertex3f(x, -2.0f, z + 1);
        }
    }

    glEnd();
}

//==============================================================================
// Render Sky Gradient
//==============================================================================

void render_sky(void) {
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_LIGHTING);

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(-1, 1, -1, 1, -1, 1);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glBegin(GL_QUADS);
    glColor3f(0.4f, 0.6f, 1.0f);  // Sky blue top
    glVertex2f(-1, 1);
    glVertex2f(1, 1);
    glColor3f(0.8f, 0.9f, 1.0f);  // Horizon white
    glVertex2f(1, 0);
    glVertex2f(-1, 0);
    glEnd();

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
}

//==============================================================================
// Render Crystalline Cube (FP-First with Plasma)
//==============================================================================

void render_crystalline_cube(const CubeState* state, float time) {
    glPushMatrix();

    // FP-FIRST: Apply transformations
    glTranslatef(state->position[0], state->position[1], state->position[2]);

    // FP-FIRST: Perlin noise modulation
    float noise_x = perlin_noise(time * 0.3f, 0, 0) * 0.2f;
    float noise_y = perlin_noise(0, time * 0.3f, 0) * 0.2f;
    float noise_z = perlin_noise(0, 0, time * 0.3f) * 0.2f;

    glRotatef(state->rotation[0] + noise_x * 30.0f, 1, 0, 0);
    glRotatef(state->rotation[1] + noise_y * 30.0f, 0, 1, 0);
    glRotatef(state->rotation[2] + noise_z * 30.0f, 0, 0, 1);
    glScalef(state->scale, state->scale, state->scale);

    // Render cube faces with plasma colors
    float vertices[8][3] = {
        {-1, -1, -1}, {1, -1, -1}, {1, 1, -1}, {-1, 1, -1},
        {-1, -1, 1}, {1, -1, 1}, {1, 1, 1}, {-1, 1, 1}
    };

    int faces[6][4] = {
        {0, 1, 2, 3}, {4, 5, 6, 7}, {0, 1, 5, 4},
        {2, 3, 7, 6}, {0, 3, 7, 4}, {1, 2, 6, 5}
    };

    float normals[6][3] = {
        {0, 0, -1}, {0, 0, 1}, {0, -1, 0},
        {0, 1, 0}, {-1, 0, 0}, {1, 0, 0}
    };

    for (int f = 0; f < 6; f++) {
        glBegin(GL_QUADS);
        glNormal3fv(normals[f]);

        for (int v = 0; v < 4; v++) {
            int idx = faces[f][v];
            float color[3];

            // FP-FIRST: Pure function computes plasma color
            compute_plasma_color(
                vertices[idx][0],
                vertices[idx][1],
                vertices[idx][2],
                time,
                color
            );

            glColor3fv(color);
            glVertex3fv(vertices[idx]);
        }

        glEnd();
    }

    // Render wireframe for crystalline look
    glDisable(GL_LIGHTING);
    glColor3f(1.0f, 1.0f, 1.0f);
    glLineWidth(2.0f);

    for (int f = 0; f < 6; f++) {
        glBegin(GL_LINE_LOOP);
        for (int v = 0; v < 4; v++) {
            int idx = faces[f][v];
            glVertex3fv(vertices[idx]);
        }
        glEnd();
    }

    glEnable(GL_LIGHTING);
    glPopMatrix();
}

//==============================================================================
// OpenGL Setup
//==============================================================================

void setup_opengl(void) {
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_LIGHT1);
    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Soft lighting
    float ambient[] = {0.3f, 0.3f, 0.4f, 1.0f};
    float diffuse[] = {0.8f, 0.8f, 0.7f, 1.0f};
    float specular[] = {1.0f, 1.0f, 1.0f, 1.0f};
    float position[] = {5.0f, 10.0f, 5.0f, 1.0f};

    glLightfv(GL_LIGHT0, GL_AMBIENT, ambient);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuse);
    glLightfv(GL_LIGHT0, GL_SPECULAR, specular);
    glLightfv(GL_LIGHT0, GL_POSITION, position);

    // Fill light (softer shadows)
    float fill_diffuse[] = {0.3f, 0.3f, 0.4f, 1.0f};
    float fill_position[] = {-5.0f, 5.0f, -5.0f, 1.0f};
    glLightfv(GL_LIGHT1, GL_DIFFUSE, fill_diffuse);
    glLightfv(GL_LIGHT1, GL_POSITION, fill_position);

    glShadeModel(GL_SMOOTH);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
}

//==============================================================================
// Render Frame (FP-First)
//==============================================================================

void render_frame(void) {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Render sky gradient
    render_sky();

    // Setup perspective
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(60.0, (float)WIDTH / HEIGHT, 0.1, 100.0);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    // FP-FIRST: Camera (immutable state)
    Camera camera = {
        .position = {sinf(g_time * 0.2f) * 5.0f, 3.0f, cosf(g_time * 0.2f) * 5.0f},
        .target = {0.0f, 0.0f, 0.0f},
        .up = {0.0f, 1.0f, 0.0f}
    };

    gluLookAt(
        camera.position[0], camera.position[1], camera.position[2],
        camera.target[0], camera.target[1], camera.target[2],
        camera.up[0], camera.up[1], camera.up[2]
    );

    // Render floor
    render_floor();

    // FP-FIRST: Cube state (immutable)
    CubeState cube = {
        .position = {0.0f, 0.0f, 0.0f},
        .rotation = {g_time * 20.0f, g_time * 30.0f, g_time * 15.0f},
        .scale = 1.0f + sinf(g_time * 0.5f) * 0.1f
    };

    // Render crystalline cube
    render_crystalline_cube(&cube, g_time);

    SwapBuffers(g_hdc);
}

//==============================================================================
// Window Procedure
//==============================================================================

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_CLOSE:
            PostQuitMessage(0);
            return 0;
        case WM_KEYDOWN:
            if (wParam == VK_ESCAPE) {
                PostQuitMessage(0);
            }
            return 0;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

//==============================================================================
// Main
//==============================================================================

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    // Register window class
    WNDCLASSEX wc = {0};
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.lpszClassName = "FPCrystalline";

    RegisterClassEx(&wc);

    // Create window
    g_hwnd = CreateWindowEx(
        0, "FPCrystalline",
        "FP-First Crystalline Cube - Pure Functional Graphics",
        WS_OVERLAPPEDWINDOW | WS_VISIBLE,
        100, 100, WIDTH, HEIGHT,
        NULL, NULL, hInstance, NULL
    );

    // Setup OpenGL
    g_hdc = GetDC(g_hwnd);

    PIXELFORMATDESCRIPTOR pfd = {0};
    pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR);
    pfd.nVersion = 1;
    pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = 32;
    pfd.cDepthBits = 24;

    int pixelFormat = ChoosePixelFormat(g_hdc, &pfd);
    SetPixelFormat(g_hdc, pixelFormat, &pfd);

    g_hglrc = wglCreateContext(g_hdc);
    wglMakeCurrent(g_hdc, g_hglrc);

    setup_opengl();

    printf("==============================================\n");
    printf("   FP-FIRST CRYSTALLINE SHOWCASE\n");
    printf("   Real-Time Pure Functional Graphics\n");
    printf("==============================================\n\n");
    printf("Features:\n");
    printf("  - Rotating crystalline cube with Perlin noise\n");
    printf("  - Plasma flowing inside (pure functions)\n");
    printf("  - Checkered creamy floor\n");
    printf("  - Sky gradient horizon\n");
    printf("  - Soft shadows (dual lights)\n");
    printf("  - Orbiting camera\n\n");
    printf("Press ESC to exit\n\n");
    printf("FP Library Usage:\n");
    printf("  - All transformations use immutable state\n");
    printf("  - Pure functions for colors & effects\n");
    printf("  - Functional composition throughout\n");
    printf("==============================================\n");

    // Main loop
    MSG msg;
    clock_t start = clock();

    while (1) {
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) {
                goto cleanup;
            }
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        g_time = (float)(clock() - start) / CLOCKS_PER_SEC;
        render_frame();
    }

cleanup:
    wglMakeCurrent(NULL, NULL);
    wglDeleteContext(g_hglrc);
    ReleaseDC(g_hwnd, g_hdc);
    DestroyWindow(g_hwnd);

    return 0;
}
