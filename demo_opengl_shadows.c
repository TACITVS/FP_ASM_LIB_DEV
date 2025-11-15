/**
 * demo_opengl_shadows.c
 *
 * ENHANCEMENT #1: Shadow Mapping
 *
 * Adds real-time dynamic shadows to the OpenGL showcase using:
 * - Shadow mapping technique (render from light's POV)
 * - Depth texture for shadow buffer
 * - Two-pass rendering (shadow pass + main pass)
 * - PCF (Percentage Closer Filtering) for soft shadows
 *
 * Controls:
 *   W/S - Move forward/backward
 *   A/D - Strafe left/right
 *   Q/E - Move up/down
 *   L   - Toggle light rotation
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
#define NUM_CUBES 500  // Reduced for shadow performance
#define SHADOW_MAP_SIZE 2048

// Extension function pointers (for shadow mapping)
typedef void (WINAPI *PFNGLACTIVETEXTUREPROC)(GLenum texture);
PFNGLACTIVETEXTUREPROC glActiveTextureARB = NULL;

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

// Light state
float light_angle = 45.0f;
float light_x = 30.0f, light_y = 50.0f, light_z = 30.0f;
int light_rotating = 1;

// Shadow map
GLuint shadow_texture;
GLuint shadow_fbo = 0;  // We'll use aux buffer for simplicity

int frame_count = 0;
clock_t last_time;
float fps = 0.0f;

// OpenGL window handle
HWND hwnd;
HDC hdc;
HGLRC hglrc;

void init_cubes() {
    cubes = malloc(NUM_CUBES * sizeof(Cube));
    srand(42);

    int grid = (int)cbrt((float)NUM_CUBES) + 1;
    for (int i = 0; i < NUM_CUBES; i++) {
        cubes[i].x = ((float)(i % grid) - grid/2.0f) * 6.0f;
        cubes[i].y = ((float)((i / grid) % grid) - grid/2.0f) * 6.0f;
        cubes[i].z = ((float)(i / (grid * grid)) - grid/2.0f) * 6.0f;

        cubes[i].rot_x = (float)(rand() % 360) * PI / 180.0f;
        cubes[i].rot_y = (float)(rand() % 360) * PI / 180.0f;
        cubes[i].rot_z = (float)(rand() % 360) * PI / 180.0f;

        cubes[i].rot_speed_x = ((float)(rand() % 100) / 1000.0f) - 0.05f;
        cubes[i].rot_speed_y = ((float)(rand() % 100) / 1000.0f) - 0.05f;
        cubes[i].rot_speed_z = ((float)(rand() % 100) / 1000.0f) - 0.05f;

        // Random colors
        cubes[i].color.x = 0.5f + (float)(rand() % 50) / 100.0f;
        cubes[i].color.y = 0.5f + (float)(rand() % 50) / 100.0f;
        cubes[i].color.z = 0.5f + (float)(rand() % 50) / 100.0f;
    }
}

void update_cubes(float dt) {
    for (int i = 0; i < NUM_CUBES; i++) {
        cubes[i].rot_x += cubes[i].rot_speed_x * dt;
        cubes[i].rot_y += cubes[i].rot_speed_y * dt;
        cubes[i].rot_z += cubes[i].rot_speed_z * dt;
    }
}

void update_light(float dt) {
    if (light_rotating) {
        light_angle += 10.0f * dt / 1000.0f;  // Slow rotation
        light_x = 40.0f * cosf(light_angle * PI / 180.0f);
        light_z = 40.0f * sinf(light_angle * PI / 180.0f);
    }
}

void draw_cube() {
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

void draw_ground() {
    // Large ground plane to receive shadows
    glPushMatrix();
    glTranslatef(0.0f, -20.0f, 0.0f);
    glScalef(100.0f, 0.1f, 100.0f);

    float ground_color[] = {0.3f, 0.35f, 0.3f, 1.0f};
    float ambient[] = {0.1f, 0.12f, 0.1f, 1.0f};
    glMaterialfv(GL_FRONT, GL_DIFFUSE, ground_color);
    glMaterialfv(GL_FRONT, GL_AMBIENT, ambient);

    draw_cube();
    glPopMatrix();
}

void render_scene() {
    // Draw ground
    draw_ground();

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
}

void render_shadow_pass() {
    // Render from light's point of view to create shadow map
    glViewport(0, 0, SHADOW_MAP_SIZE, SHADOW_MAP_SIZE);

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluPerspective(90.0, 1.0, 1.0, 200.0);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    gluLookAt(light_x, light_y, light_z,  // Light position
              0.0, 0.0, 0.0,               // Look at center
              0.0, 1.0, 0.0);              // Up vector

    // Disable lighting and color for depth-only pass
    glDisable(GL_LIGHTING);
    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);

    glClear(GL_DEPTH_BUFFER_BIT);

    // Render scene geometry
    render_scene();

    // Copy depth buffer to shadow texture
    glBindTexture(GL_TEXTURE_2D, shadow_texture);
    glCopyTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, 0, 0,
                     SHADOW_MAP_SIZE, SHADOW_MAP_SIZE, 0);

    // Restore state
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glEnable(GL_LIGHTING);

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
}

void render_main_pass() {
    // Restore viewport
    glViewport(0, 0, 1280, 720);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();

    // Camera transform
    glRotatef(-camera_pitch, 1.0f, 0.0f, 0.0f);
    glRotatef(-camera_yaw, 0.0f, 1.0f, 0.0f);
    glTranslatef(-camera_x, -camera_y, -camera_z);

    // Update light position
    float light_pos[] = {light_x, light_y, light_z, 1.0f};
    glLightfv(GL_LIGHT0, GL_POSITION, light_pos);

    // Setup shadow texture matrix for projective texturing
    glMatrixMode(GL_TEXTURE);
    glLoadIdentity();
    glTranslatef(0.5f, 0.5f, 0.5f);  // Bias and scale to [0,1]
    glScalef(0.5f, 0.5f, 0.5f);
    gluPerspective(90.0, 1.0, 1.0, 200.0);
    gluLookAt(light_x, light_y, light_z,
              0.0, 0.0, 0.0,
              0.0, 1.0, 0.0);
    glMatrixMode(GL_MODELVIEW);

    // Enable shadow mapping
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, shadow_texture);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

    // Enable automatic texture coordinate generation (eye-linear)
    glEnable(GL_TEXTURE_GEN_S);
    glEnable(GL_TEXTURE_GEN_T);
    glEnable(GL_TEXTURE_GEN_R);
    glEnable(GL_TEXTURE_GEN_Q);
    glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR);
    glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR);
    glTexGeni(GL_R, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR);
    glTexGeni(GL_Q, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR);

    // Enable shadow comparison
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_R_TO_TEXTURE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
    glTexParameteri(GL_TEXTURE_2D, GL_DEPTH_TEXTURE_MODE, GL_INTENSITY);

    // Render scene with shadows
    render_scene();

    // Disable shadow mapping
    glDisable(GL_TEXTURE_GEN_S);
    glDisable(GL_TEXTURE_GEN_T);
    glDisable(GL_TEXTURE_GEN_R);
    glDisable(GL_TEXTURE_GEN_Q);
    glDisable(GL_TEXTURE_2D);

    // Draw light source indicator
    glDisable(GL_LIGHTING);
    glColor3f(1.0f, 1.0f, 0.0f);
    glPushMatrix();
    glTranslatef(light_x, light_y, light_z);
    glScalef(2.0f, 2.0f, 2.0f);
    draw_cube();
    glPopMatrix();
    glEnable(GL_LIGHTING);

    glMatrixMode(GL_TEXTURE);
    glLoadIdentity();
    glMatrixMode(GL_MODELVIEW);
}

void render() {
    // Two-pass rendering: shadow pass, then main pass
    render_shadow_pass();
    render_main_pass();

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
        sprintf(title, "FP-ASM Shadow Demo | %d cubes | %.1f FPS | Light: %s | Pos: (%.1f, %.1f, %.1f)",
                NUM_CUBES, fps, light_rotating ? "Rotating" : "Fixed", camera_x, camera_y, camera_z);
        SetWindowTextA(hwnd, title);
    }
}

void setup_shadow_map() {
    // Create shadow map texture
    glGenTextures(1, &shadow_texture);
    glBindTexture(GL_TEXTURE_2D, shadow_texture);

    // Shadow map parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // Allocate depth texture storage
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SHADOW_MAP_SIZE, SHADOW_MAP_SIZE,
                 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE, NULL);
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
    float light_ambient[] = {0.2f, 0.2f, 0.2f, 1.0f};
    float light_diffuse[] = {1.0f, 1.0f, 1.0f, 1.0f};
    float light_specular[] = {1.0f, 1.0f, 1.0f, 1.0f};

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

    // Setup shadow mapping
    setup_shadow_map();
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_CLOSE:
            PostQuitMessage(0);
            return 0;
        case WM_KEYDOWN:
            if (wParam == VK_ESCAPE) {
                PostQuitMessage(0);
            } else if (wParam == 'L') {
                light_rotating = !light_rotating;
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
    wc.lpszClassName = "FPASMShadows";
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    RegisterClassA(&wc);

    // Create window
    hwnd = CreateWindowExA(0, "FPASMShadows", "FP-ASM Shadow Mapping Demo",
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

    printf("==============================================\n");
    printf("  FP-ASM Shadow Mapping Demo\n");
    printf("==============================================\n");
    printf("Enhancement #1: Real-time Dynamic Shadows\n");
    printf("\n");
    printf("Controls:\n");
    printf("  W/A/S/D - Move camera\n");
    printf("  Q/E     - Move up/down\n");
    printf("  L       - Toggle light rotation\n");
    printf("  ESC     - Exit\n");
    printf("\n");
    printf("Shadow Map Resolution: %dx%d\n", SHADOW_MAP_SIZE, SHADOW_MAP_SIZE);
    printf("Number of Cubes: %d\n", NUM_CUBES);
    printf("==============================================\n\n");

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

        update_light(16.0f);
        update_cubes(16.0f);
        render();

        Sleep(1);
    }

    // Cleanup
    glDeleteTextures(1, &shadow_texture);
    wglMakeCurrent(NULL, NULL);
    wglDeleteContext(hglrc);
    ReleaseDC(hwnd, hdc);
    free(cubes);

    return 0;
}
