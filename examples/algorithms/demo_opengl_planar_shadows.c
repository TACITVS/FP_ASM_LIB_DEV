/**
 * demo_opengl_planar_shadows.c
 *
 * ENHANCEMENT #1: Planar Shadows (Geometry Projection)
 *
 * Adds real-time dynamic shadows using planar shadow projection:
 * - Projects geometry onto ground plane based on light position
 * - Uses stencil buffer to prevent double-blending
 * - Simple, fast, and works with basic OpenGL 1.1
 * - No extensions required!
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
#define NUM_CUBES 500

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
        light_angle += 10.0f * dt / 1000.0f;
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

// Calculate shadow projection matrix for planar shadows
void shadow_matrix(float shadow_mat[16], float ground_plane[4], float light_pos[4]) {
    float dot = ground_plane[0] * light_pos[0] +
                ground_plane[1] * light_pos[1] +
                ground_plane[2] * light_pos[2] +
                ground_plane[3] * light_pos[3];

    shadow_mat[0] = dot - light_pos[0] * ground_plane[0];
    shadow_mat[4] = 0.0f - light_pos[0] * ground_plane[1];
    shadow_mat[8] = 0.0f - light_pos[0] * ground_plane[2];
    shadow_mat[12] = 0.0f - light_pos[0] * ground_plane[3];

    shadow_mat[1] = 0.0f - light_pos[1] * ground_plane[0];
    shadow_mat[5] = dot - light_pos[1] * ground_plane[1];
    shadow_mat[9] = 0.0f - light_pos[1] * ground_plane[2];
    shadow_mat[13] = 0.0f - light_pos[1] * ground_plane[3];

    shadow_mat[2] = 0.0f - light_pos[2] * ground_plane[0];
    shadow_mat[6] = 0.0f - light_pos[2] * ground_plane[1];
    shadow_mat[10] = dot - light_pos[2] * ground_plane[2];
    shadow_mat[14] = 0.0f - light_pos[2] * ground_plane[3];

    shadow_mat[3] = 0.0f - light_pos[3] * ground_plane[0];
    shadow_mat[7] = 0.0f - light_pos[3] * ground_plane[1];
    shadow_mat[11] = 0.0f - light_pos[3] * ground_plane[2];
    shadow_mat[15] = dot - light_pos[3] * ground_plane[3];
}

void render_cubes_geometry() {
    // Draw all cubes geometry
    for (int i = 0; i < NUM_CUBES; i++) {
        glPushMatrix();

        glTranslatef(cubes[i].x, cubes[i].y, cubes[i].z);
        glRotatef(cubes[i].rot_x * 180.0f / PI, 1.0f, 0.0f, 0.0f);
        glRotatef(cubes[i].rot_y * 180.0f / PI, 0.0f, 1.0f, 0.0f);
        glRotatef(cubes[i].rot_z * 180.0f / PI, 0.0f, 0.0f, 1.0f);

        draw_cube();

        glPopMatrix();
    }
}

void render_cubes_lit() {
    // Draw all cubes with lighting and materials
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

void render() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    glLoadIdentity();

    // Camera transform
    glRotatef(-camera_pitch, 1.0f, 0.0f, 0.0f);
    glRotatef(-camera_yaw, 0.0f, 1.0f, 0.0f);
    glTranslatef(-camera_x, -camera_y, -camera_z);

    // Update light position
    float light_pos[] = {light_x, light_y, light_z, 1.0f};
    glLightfv(GL_LIGHT0, GL_POSITION, light_pos);

    // Draw ground first
    draw_ground();

    // Draw shadows using planar projection
    glDisable(GL_LIGHTING);
    glDisable(GL_DEPTH_TEST);

    // Enable stencil test to prevent double-blending shadows
    glEnable(GL_STENCIL_TEST);
    glStencilFunc(GL_EQUAL, 1, 0xFFFFFFFF);
    glStencilOp(GL_KEEP, GL_KEEP, GL_INCR);

    // Setup shadow projection matrix
    float ground_plane[4] = {0.0f, 1.0f, 0.0f, 20.0f};  // y = -20
    float shadow_mat[16];
    shadow_matrix(shadow_mat, ground_plane, light_pos);

    glPushMatrix();
    glMultMatrixf(shadow_mat);

    // Draw shadows as dark geometry
    glColor4f(0.0f, 0.0f, 0.0f, 0.5f);  // Semi-transparent black
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    render_cubes_geometry();

    glDisable(GL_BLEND);
    glPopMatrix();

    // Restore state
    glDisable(GL_STENCIL_TEST);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);

    // Draw cubes with proper lighting
    render_cubes_lit();

    // Draw light source indicator
    glDisable(GL_LIGHTING);
    glColor3f(1.0f, 1.0f, 0.0f);
    glPushMatrix();
    glTranslatef(light_x, light_y, light_z);
    glScalef(2.0f, 2.0f, 2.0f);
    draw_cube();
    glPopMatrix();
    glEnable(GL_LIGHTING);

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
        sprintf(title, "FP-ASM Planar Shadows | %d cubes | %.1f FPS | Light: %s | Pos: (%.1f, %.1f, %.1f)",
                NUM_CUBES, fps, light_rotating ? "Rotating" : "Fixed", camera_x, camera_y, camera_z);
        SetWindowTextA(hwnd, title);
    }
}

void setup_opengl() {
    // Enable depth testing
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);

    // Enable stencil buffer for shadow counting
    glClearStencil(0);

    // Enable lighting
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);

    // Setup light
    float light_ambient[] = {0.3f, 0.3f, 0.3f, 1.0f};  // Brighter ambient so shadows show better
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

    // Enable smooth shading
    glShadeModel(GL_SMOOTH);
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
    wc.lpszClassName = "FPASMPlanarShadows";
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    RegisterClassA(&wc);

    // Create window
    hwnd = CreateWindowExA(0, "FPASMPlanarShadows", "FP-ASM Planar Shadows Demo",
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
        24,  // 24-bit depth buffer
        8,   // 8-bit stencil buffer (IMPORTANT FOR SHADOWS!)
        0,
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
    printf("  FP-ASM Planar Shadows Demo\n");
    printf("==============================================\n");
    printf("Enhancement #1: Real-time Dynamic Shadows\n");
    printf("Technique: Planar Shadow Projection\n");
    printf("\n");
    printf("Controls:\n");
    printf("  W/A/S/D - Move camera\n");
    printf("  Q/E     - Move up/down\n");
    printf("  L       - Toggle light rotation\n");
    printf("  ESC     - Exit\n");
    printf("\n");
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
    wglMakeCurrent(NULL, NULL);
    wglDeleteContext(hglrc);
    ReleaseDC(hwnd, hdc);
    free(cubes);

    return 0;
}
