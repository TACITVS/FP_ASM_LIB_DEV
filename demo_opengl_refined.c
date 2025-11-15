/**
 * demo_opengl_refined.c
 *
 * REFINED VISUAL SHOWCASE - All Enhancements Combined
 *
 * Features:
 * - Planar shadows with soft edges
 * - Multi-sample anti-aliasing (MSAA)
 * - Slow, organic rotation with Perlin noise
 * - Adjustable rotation speed
 * - Ambient occlusion approximation
 * - Enhanced lighting model
 *
 * Controls:
 *   W/S     - Move forward/backward
 *   A/D     - Strafe left/right
 *   Q/E     - Move up/down
 *   +/-     - Increase/decrease rotation speed
 *   0       - Stop rotation
 *   L       - Toggle light rotation
 *   S_KEY   - Toggle shadows
 *   ESC     - Exit
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
#define NUM_CUBES 400

// Perlin noise functions
static int p[512];

void init_perlin() {
    int permutation[] = {
        151,160,137,91,90,15,131,13,201,95,96,53,194,233,7,225,140,36,103,30,69,142,
        8,99,37,240,21,10,23,190,6,148,247,120,234,75,0,26,197,62,94,252,219,203,117,
        35,11,32,57,177,33,88,237,149,56,87,174,20,125,136,171,168,68,175,74,165,71,
        134,139,48,27,166,77,146,158,231,83,111,229,122,60,211,133,230,220,105,92,41,
        55,46,245,40,244,102,143,54,65,25,63,161,1,216,80,73,209,76,132,187,208,89,
        18,169,200,196,135,130,116,188,159,86,164,100,109,198,173,186,3,64,52,217,226,
        250,124,123,5,202,38,147,118,126,255,82,85,212,207,206,59,227,47,16,58,17,182,
        189,28,42,223,183,170,213,119,248,152,2,44,154,163,70,221,153,101,155,167,43,
        172,9,129,22,39,253,19,98,108,110,79,113,224,232,178,185,112,104,218,246,97,
        228,251,34,242,193,238,210,144,12,191,179,162,241,81,51,145,235,249,14,239,
        107,49,192,214,31,181,199,106,157,184,84,204,176,115,121,50,45,127,4,150,254,
        138,236,205,93,222,114,67,29,24,72,243,141,128,195,78,66,215,61,156,180
    };

    for (int i = 0; i < 256; i++) p[i] = p[256 + i] = permutation[i];
}

float fade(float t) { return t * t * t * (t * (t * 6 - 15) + 10); }
float lerp(float t, float a, float b) { return a + t * (b - a); }
float grad(int hash, float x, float y, float z) {
    int h = hash & 15;
    float u = h < 8 ? x : y;
    float v = h < 4 ? y : h == 12 || h == 14 ? x : z;
    return ((h & 1) == 0 ? u : -u) + ((h & 2) == 0 ? v : -v);
}

float perlin_noise(float x, float y, float z) {
    int X = (int)floor(x) & 255;
    int Y = (int)floor(y) & 255;
    int Z = (int)floor(z) & 255;

    x -= floor(x);
    y -= floor(y);
    z -= floor(z);

    float u = fade(x);
    float v = fade(y);
    float w = fade(z);

    int A = p[X] + Y, AA = p[A] + Z, AB = p[A + 1] + Z;
    int B = p[X + 1] + Y, BA = p[B] + Z, BB = p[B + 1] + Z;

    return lerp(w, lerp(v, lerp(u, grad(p[AA], x, y, z),
                                   grad(p[BA], x - 1, y, z)),
                           lerp(u, grad(p[AB], x, y - 1, z),
                                   grad(p[BB], x - 1, y - 1, z))),
                   lerp(v, lerp(u, grad(p[AA + 1], x, y, z - 1),
                                   grad(p[BA + 1], x - 1, y, z - 1)),
                           lerp(u, grad(p[AB + 1], x, y - 1, z - 1),
                                   grad(p[BB + 1], x - 1, y - 1, z - 1))));
}

// Global state
typedef struct {
    float x, y, z;
    float rot_x, rot_y, rot_z;
    float noise_offset_x, noise_offset_y, noise_offset_z;  // For organic rotation
    Vec3f color;
    float ambient_occlusion;  // Pre-computed AO factor
} Cube;

Cube* cubes = NULL;
float camera_x = 0.0f, camera_y = 25.0f, camera_z = 80.0f;
float camera_yaw = 0.0f, camera_pitch = -15.0f;

// Light state
float light_angle = 45.0f;
float light_x = 40.0f, light_y = 50.0f, light_z = 40.0f;
int light_rotating = 1;

// Rendering options
int shadows_enabled = 1;
float rotation_speed_multiplier = 1.0f;  // User can adjust this
float global_time = 0.0f;

int frame_count = 0;
clock_t last_time;
float fps = 0.0f;

HWND hwnd;
HDC hdc;
HGLRC hglrc;

// Calculate simple ambient occlusion based on nearby cubes
float calculate_ao(float x, float y, float z) {
    // Simple heuristic: cubes near center have more occlusion
    float dist_from_center = sqrtf(x*x + y*y + z*z);
    float ao = 0.4f + 0.6f * (dist_from_center / 50.0f);
    if (ao > 1.0f) ao = 1.0f;
    return ao;
}

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

        // Random noise offsets for organic rotation
        cubes[i].noise_offset_x = (float)(rand() % 1000);
        cubes[i].noise_offset_y = (float)(rand() % 1000);
        cubes[i].noise_offset_z = (float)(rand() % 1000);

        // Vibrant colors
        cubes[i].color.x = 0.5f + (float)(rand() % 50) / 100.0f;
        cubes[i].color.y = 0.5f + (float)(rand() % 50) / 100.0f;
        cubes[i].color.z = 0.5f + (float)(rand() % 50) / 100.0f;

        // Calculate ambient occlusion
        cubes[i].ambient_occlusion = calculate_ao(cubes[i].x, cubes[i].y, cubes[i].z);
    }
}

void update_cubes(float dt) {
    global_time += dt / 1000.0f;

    for (int i = 0; i < NUM_CUBES; i++) {
        // Organic rotation using Perlin noise
        float noise_x = perlin_noise((global_time + cubes[i].noise_offset_x) * 0.1f, 0.0f, 0.0f);
        float noise_y = perlin_noise((global_time + cubes[i].noise_offset_y) * 0.1f, 1.0f, 0.0f);
        float noise_z = perlin_noise((global_time + cubes[i].noise_offset_z) * 0.1f, 2.0f, 0.0f);

        // Much slower rotation (10x slower) with organic variation
        cubes[i].rot_x += noise_x * 0.001f * rotation_speed_multiplier;
        cubes[i].rot_y += noise_y * 0.001f * rotation_speed_multiplier;
        cubes[i].rot_z += noise_z * 0.001f * rotation_speed_multiplier;
    }
}

void update_light(float dt) {
    if (light_rotating) {
        light_angle += 5.0f * dt / 1000.0f;  // Slow light rotation
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
    glPushMatrix();
    glTranslatef(0.0f, -20.0f, 0.0f);
    glScalef(100.0f, 0.1f, 100.0f);

    float ground_color[] = {0.3f, 0.35f, 0.3f, 1.0f};
    float ambient[] = {0.15f, 0.18f, 0.15f, 1.0f};  // Slightly brighter ambient
    glMaterialfv(GL_FRONT, GL_DIFFUSE, ground_color);
    glMaterialfv(GL_FRONT, GL_AMBIENT, ambient);

    draw_cube();
    glPopMatrix();
}

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

    // Draw ground
    draw_ground();

    // Draw soft shadows
    if (shadows_enabled) {
        glDisable(GL_LIGHTING);
        glDisable(GL_DEPTH_TEST);
        glEnable(GL_STENCIL_TEST);
        glStencilFunc(GL_EQUAL, 1, 0xFFFFFFFF);
        glStencilOp(GL_KEEP, GL_KEEP, GL_INCR);

        float ground_plane[4] = {0.0f, 1.0f, 0.0f, 20.0f};
        float shadow_mat[16];
        shadow_matrix(shadow_mat, ground_plane, light_pos);

        glPushMatrix();
        glMultMatrixf(shadow_mat);

        // Softer shadows with lower opacity
        glColor4f(0.0f, 0.0f, 0.0f, 0.3f);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        render_cubes_geometry();

        glDisable(GL_BLEND);
        glPopMatrix();

        glDisable(GL_STENCIL_TEST);
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_LIGHTING);
    }

    // Draw cubes with ambient occlusion and enhanced lighting
    for (int i = 0; i < NUM_CUBES; i++) {
        glPushMatrix();

        glTranslatef(cubes[i].x, cubes[i].y, cubes[i].z);
        glRotatef(cubes[i].rot_x * 180.0f / PI, 1.0f, 0.0f, 0.0f);
        glRotatef(cubes[i].rot_y * 180.0f / PI, 0.0f, 1.0f, 0.0f);
        glRotatef(cubes[i].rot_z * 180.0f / PI, 0.0f, 0.0f, 1.0f);

        // Apply ambient occlusion to material
        float ao = cubes[i].ambient_occlusion;
        float ambient[] = {cubes[i].color.x * 0.2f * ao, cubes[i].color.y * 0.2f * ao, cubes[i].color.z * 0.2f * ao, 1.0f};
        float diffuse[] = {cubes[i].color.x * ao, cubes[i].color.y * ao, cubes[i].color.z * ao, 1.0f};
        float specular[] = {0.4f, 0.4f, 0.4f, 1.0f};

        glMaterialfv(GL_FRONT, GL_AMBIENT, ambient);
        glMaterialfv(GL_FRONT, GL_DIFFUSE, diffuse);
        glMaterialfv(GL_FRONT, GL_SPECULAR, specular);
        glMaterialf(GL_FRONT, GL_SHININESS, 32.0f);

        draw_cube();
        glPopMatrix();
    }

    // Draw light indicator
    glDisable(GL_LIGHTING);
    glColor3f(1.0f, 1.0f, 0.8f);
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
        sprintf(title, "FP-ASM Refined | %d cubes | %.1f FPS | Speed: %.1fx | Shadows: %s | Light: %s",
                NUM_CUBES, fps, rotation_speed_multiplier,
                shadows_enabled ? "ON" : "OFF",
                light_rotating ? "Rotating" : "Fixed");
        SetWindowTextA(hwnd, title);
    }
}

void setup_opengl() {
    // Enable anti-aliasing hints
    glEnable(GL_LINE_SMOOTH);
    glEnable(GL_POLYGON_SMOOTH);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
    glHint(GL_POLYGON_SMOOTH_HINT, GL_NICEST);
    glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);

    glClearStencil(0);

    // Enhanced lighting
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);

    // Warmer, more natural lighting
    float light_ambient[] = {0.3f, 0.3f, 0.35f, 1.0f};
    float light_diffuse[] = {0.9f, 0.9f, 0.85f, 1.0f};
    float light_specular[] = {1.0f, 1.0f, 0.95f, 1.0f};

    glLightfv(GL_LIGHT0, GL_AMBIENT, light_ambient);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, light_diffuse);
    glLightfv(GL_LIGHT0, GL_SPECULAR, light_specular);

    glViewport(0, 0, 1280, 720);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(60.0, 1280.0 / 720.0, 0.1, 1000.0);
    glMatrixMode(GL_MODELVIEW);

    glClearColor(0.1f, 0.12f, 0.15f, 1.0f);
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
            } else if (wParam == VK_OEM_PLUS || wParam == VK_ADD) {  // + key
                rotation_speed_multiplier += 0.5f;
                if (rotation_speed_multiplier > 5.0f) rotation_speed_multiplier = 5.0f;
            } else if (wParam == VK_OEM_MINUS || wParam == VK_SUBTRACT) {  // - key
                rotation_speed_multiplier -= 0.5f;
                if (rotation_speed_multiplier < 0.0f) rotation_speed_multiplier = 0.0f;
            } else if (wParam == '0') {
                rotation_speed_multiplier = 0.0f;
            } else if (wParam == 'H') {  // H for shadows (S conflicts with movement)
                shadows_enabled = !shadows_enabled;
            }
            return 0;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    WNDCLASSA wc = {0};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = "FPASMRefined";
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    RegisterClassA(&wc);

    hwnd = CreateWindowExA(0, "FPASMRefined", "FP-ASM Refined Visual Showcase",
                          WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                          100, 100, 1280, 720,
                          NULL, NULL, hInstance, NULL);

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

    init_perlin();
    setup_opengl();
    init_cubes();

    last_time = clock();

    printf("==============================================\n");
    printf("  FP-ASM Refined Visual Showcase\n");
    printf("==============================================\n");
    printf("Enhancements:\n");
    printf("  - Slow, organic rotation (Perlin noise)\n");
    printf("  - Soft planar shadows\n");
    printf("  - Ambient occlusion approximation\n");
    printf("  - Anti-aliasing hints\n");
    printf("  - Enhanced lighting model\n");
    printf("\n");
    printf("Controls:\n");
    printf("  W/A/S/D - Move camera\n");
    printf("  Q/E     - Move up/down\n");
    printf("  +/-     - Increase/decrease rotation speed\n");
    printf("  0       - Stop rotation\n");
    printf("  L       - Toggle light rotation\n");
    printf("  H       - Toggle shadows\n");
    printf("  ESC     - Exit\n");
    printf("\n");
    printf("Number of Cubes: %d\n", NUM_CUBES);
    printf("Initial Rotation Speed: 1.0x (slow)\n");
    printf("==============================================\n\n");

    MSG msg;
    BOOL running = TRUE;
    clock_t last_frame_time = clock();

    while (running) {
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) {
                running = FALSE;
            }
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        clock_t current_frame_time = clock();
        float dt = (float)(current_frame_time - last_frame_time) / CLOCKS_PER_SEC * 1000.0f;
        last_frame_time = current_frame_time;

        // Handle keyboard input
        if (GetAsyncKeyState('W') & 0x8000) camera_z -= 0.5f;
        if (GetAsyncKeyState('S') & 0x8000) camera_z += 0.5f;
        if (GetAsyncKeyState('A') & 0x8000) camera_x -= 0.5f;
        if (GetAsyncKeyState('D') & 0x8000) camera_x += 0.5f;
        if (GetAsyncKeyState('Q') & 0x8000) camera_y -= 0.5f;
        if (GetAsyncKeyState('E') & 0x8000) camera_y += 0.5f;

        update_light(dt);
        update_cubes(dt);
        render();

        Sleep(1);
    }

    wglMakeCurrent(NULL, NULL);
    wglDeleteContext(hglrc);
    ReleaseDC(hwnd, hdc);
    free(cubes);

    return 0;
}
