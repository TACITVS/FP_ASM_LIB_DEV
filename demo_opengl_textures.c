/**
 * demo_opengl_textures.c
 *
 * ENHANCEMENT #1c: Procedural Textures
 *
 * Adds texture mapping with procedurally generated patterns:
 * - Checkerboard pattern
 * - Noise texture
 * - Gradient texture
 * - Brick pattern
 * - All generated in code (no image files needed!)
 *
 * Controls:
 *   W/S - Move forward/backward
 *   A/D - Strafe left/right
 *   Q/E - Move up/down
 *   1-4 - Switch texture type
 *   T   - Toggle textures on/off
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
#define NUM_CUBES 300
#define TEXTURE_SIZE 256

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

GLuint textures[4];  // Multiple texture patterns
int current_texture = 0;
int textures_enabled = 1;

int frame_count = 0;
clock_t last_time;
float fps = 0.0f;

HWND hwnd;
HDC hdc;
HGLRC hglrc;

// Procedural texture generators
void generate_checkerboard(unsigned char* data, int size) {
    for (int y = 0; y < size; y++) {
        for (int x = 0; x < size; x++) {
            int checker = ((x / 32) + (y / 32)) & 1;
            unsigned char color = checker ? 255 : 64;

            int idx = (y * size + x) * 3;
            data[idx + 0] = color;
            data[idx + 1] = color;
            data[idx + 2] = color;
        }
    }
}

float noise(int x, int y) {
    int n = x + y * 57;
    n = (n << 13) ^ n;
    return (1.0f - ((n * (n * n * 15731 + 789221) + 1376312589) & 0x7fffffff) / 1073741824.0f);
}

void generate_noise_texture(unsigned char* data, int size) {
    for (int y = 0; y < size; y++) {
        for (int x = 0; x < size; x++) {
            float n = (noise(x, y) + 1.0f) * 0.5f;
            unsigned char color = (unsigned char)(n * 255);

            int idx = (y * size + x) * 3;
            data[idx + 0] = color;
            data[idx + 1] = color;
            data[idx + 2] = color;
        }
    }
}

void generate_gradient(unsigned char* data, int size) {
    for (int y = 0; y < size; y++) {
        for (int x = 0; x < size; x++) {
            float fx = (float)x / size;
            float fy = (float)y / size;

            int idx = (y * size + x) * 3;
            data[idx + 0] = (unsigned char)(fx * 255);       // Red gradient
            data[idx + 1] = (unsigned char)(fy * 255);       // Green gradient
            data[idx + 2] = (unsigned char)((1.0f - fx) * 255); // Blue gradient
        }
    }
}

void generate_brick_pattern(unsigned char* data, int size) {
    const int brick_width = 64;
    const int brick_height = 32;
    const int mortar = 4;

    for (int y = 0; y < size; y++) {
        for (int x = 0; x < size; x++) {
            int brick_row = y / brick_height;
            int offset = (brick_row & 1) ? brick_width / 2 : 0;

            int local_x = (x + offset) % brick_width;
            int local_y = y % brick_height;

            int is_mortar = (local_x < mortar || local_y < mortar);

            int idx = (y * size + x) * 3;
            if (is_mortar) {
                // Gray mortar
                data[idx + 0] = 100;
                data[idx + 1] = 100;
                data[idx + 2] = 100;
            } else {
                // Red brick with variation
                float variation = (noise(x / 4, y / 4) + 1.0f) * 0.5f;
                data[idx + 0] = 150 + (unsigned char)(variation * 80);
                data[idx + 1] = 50 + (unsigned char)(variation * 30);
                data[idx + 2] = 30;
            }
        }
    }
}

void create_textures() {
    unsigned char* tex_data = malloc(TEXTURE_SIZE * TEXTURE_SIZE * 3);

    glGenTextures(4, textures);

    // Texture 0: Checkerboard
    generate_checkerboard(tex_data, TEXTURE_SIZE);
    glBindTexture(GL_TEXTURE_2D, textures[0]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, TEXTURE_SIZE, TEXTURE_SIZE, 0,
                 GL_RGB, GL_UNSIGNED_BYTE, tex_data);

    // Texture 1: Noise
    generate_noise_texture(tex_data, TEXTURE_SIZE);
    glBindTexture(GL_TEXTURE_2D, textures[1]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, TEXTURE_SIZE, TEXTURE_SIZE, 0,
                 GL_RGB, GL_UNSIGNED_BYTE, tex_data);

    // Texture 2: Gradient
    generate_gradient(tex_data, TEXTURE_SIZE);
    glBindTexture(GL_TEXTURE_2D, textures[2]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, TEXTURE_SIZE, TEXTURE_SIZE, 0,
                 GL_RGB, GL_UNSIGNED_BYTE, tex_data);

    // Texture 3: Brick
    generate_brick_pattern(tex_data, TEXTURE_SIZE);
    glBindTexture(GL_TEXTURE_2D, textures[3]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, TEXTURE_SIZE, TEXTURE_SIZE, 0,
                 GL_RGB, GL_UNSIGNED_BYTE, tex_data);

    free(tex_data);
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

        cubes[i].rot_speed_x = ((float)(rand() % 100) / 1000.0f) - 0.05f;
        cubes[i].rot_speed_y = ((float)(rand() % 100) / 1000.0f) - 0.05f;
        cubes[i].rot_speed_z = ((float)(rand() % 100) / 1000.0f) - 0.05f;

        // Lighter colors to show textures better
        cubes[i].color.x = 0.7f + (float)(rand() % 30) / 100.0f;
        cubes[i].color.y = 0.7f + (float)(rand() % 30) / 100.0f;
        cubes[i].color.z = 0.7f + (float)(rand() % 30) / 100.0f;
    }
}

void update_cubes(float dt) {
    for (int i = 0; i < NUM_CUBES; i++) {
        cubes[i].rot_x += cubes[i].rot_speed_x * dt;
        cubes[i].rot_y += cubes[i].rot_speed_y * dt;
        cubes[i].rot_z += cubes[i].rot_speed_z * dt;
    }
}

void draw_textured_cube() {
    glBegin(GL_QUADS);

    // Front face
    glNormal3f(0.0f, 0.0f, 1.0f);
    glTexCoord2f(0.0f, 0.0f); glVertex3f(-0.5f, -0.5f, 0.5f);
    glTexCoord2f(1.0f, 0.0f); glVertex3f( 0.5f, -0.5f, 0.5f);
    glTexCoord2f(1.0f, 1.0f); glVertex3f( 0.5f,  0.5f, 0.5f);
    glTexCoord2f(0.0f, 1.0f); glVertex3f(-0.5f,  0.5f, 0.5f);

    // Back face
    glNormal3f(0.0f, 0.0f, -1.0f);
    glTexCoord2f(1.0f, 0.0f); glVertex3f(-0.5f, -0.5f, -0.5f);
    glTexCoord2f(1.0f, 1.0f); glVertex3f(-0.5f,  0.5f, -0.5f);
    glTexCoord2f(0.0f, 1.0f); glVertex3f( 0.5f,  0.5f, -0.5f);
    glTexCoord2f(0.0f, 0.0f); glVertex3f( 0.5f, -0.5f, -0.5f);

    // Top face
    glNormal3f(0.0f, 1.0f, 0.0f);
    glTexCoord2f(0.0f, 1.0f); glVertex3f(-0.5f, 0.5f, -0.5f);
    glTexCoord2f(0.0f, 0.0f); glVertex3f(-0.5f, 0.5f,  0.5f);
    glTexCoord2f(1.0f, 0.0f); glVertex3f( 0.5f, 0.5f,  0.5f);
    glTexCoord2f(1.0f, 1.0f); glVertex3f( 0.5f, 0.5f, -0.5f);

    // Bottom face
    glNormal3f(0.0f, -1.0f, 0.0f);
    glTexCoord2f(1.0f, 1.0f); glVertex3f(-0.5f, -0.5f, -0.5f);
    glTexCoord2f(0.0f, 1.0f); glVertex3f( 0.5f, -0.5f, -0.5f);
    glTexCoord2f(0.0f, 0.0f); glVertex3f( 0.5f, -0.5f,  0.5f);
    glTexCoord2f(1.0f, 0.0f); glVertex3f(-0.5f, -0.5f,  0.5f);

    // Right face
    glNormal3f(1.0f, 0.0f, 0.0f);
    glTexCoord2f(1.0f, 0.0f); glVertex3f(0.5f, -0.5f, -0.5f);
    glTexCoord2f(1.0f, 1.0f); glVertex3f(0.5f,  0.5f, -0.5f);
    glTexCoord2f(0.0f, 1.0f); glVertex3f(0.5f,  0.5f,  0.5f);
    glTexCoord2f(0.0f, 0.0f); glVertex3f(0.5f, -0.5f,  0.5f);

    // Left face
    glNormal3f(-1.0f, 0.0f, 0.0f);
    glTexCoord2f(0.0f, 0.0f); glVertex3f(-0.5f, -0.5f, -0.5f);
    glTexCoord2f(1.0f, 0.0f); glVertex3f(-0.5f, -0.5f,  0.5f);
    glTexCoord2f(1.0f, 1.0f); glVertex3f(-0.5f,  0.5f,  0.5f);
    glTexCoord2f(0.0f, 1.0f); glVertex3f(-0.5f,  0.5f, -0.5f);

    glEnd();
}

void draw_ground() {
    glPushMatrix();
    glTranslatef(0.0f, -20.0f, 0.0f);
    glScalef(100.0f, 0.1f, 100.0f);

    float ground_color[] = {0.3f, 0.35f, 0.3f, 1.0f};
    float ambient[] = {0.1f, 0.12f, 0.1f, 1.0f};
    glMaterialfv(GL_FRONT, GL_DIFFUSE, ground_color);
    glMaterialfv(GL_FRONT, GL_AMBIENT, ambient);

    // Ground uses checkerboard texture
    if (textures_enabled) {
        glBindTexture(GL_TEXTURE_2D, textures[0]);
        glBegin(GL_QUADS);
        glNormal3f(0.0f, 1.0f, 0.0f);
        glTexCoord2f(0.0f, 0.0f); glVertex3f(-0.5f, 0.5f, -0.5f);
        glTexCoord2f(0.0f, 10.0f); glVertex3f(-0.5f, 0.5f,  0.5f);
        glTexCoord2f(10.0f, 10.0f); glVertex3f( 0.5f, 0.5f,  0.5f);
        glTexCoord2f(10.0f, 0.0f); glVertex3f( 0.5f, 0.5f, -0.5f);
        glEnd();
    } else {
        glBegin(GL_QUADS);
        glNormal3f(0.0f, 1.0f, 0.0f);
        glVertex3f(-0.5f, 0.5f, -0.5f);
        glVertex3f(-0.5f, 0.5f,  0.5f);
        glVertex3f( 0.5f, 0.5f,  0.5f);
        glVertex3f( 0.5f, 0.5f, -0.5f);
        glEnd();
    }

    glPopMatrix();
}

void render() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();

    // Camera transform
    glRotatef(-camera_pitch, 1.0f, 0.0f, 0.0f);
    glRotatef(-camera_yaw, 0.0f, 1.0f, 0.0f);
    glTranslatef(-camera_x, -camera_y, -camera_z);

    // Update light position
    float light_pos[] = {30.0f, 50.0f, 30.0f, 1.0f};
    glLightfv(GL_LIGHT0, GL_POSITION, light_pos);

    // Enable texturing
    if (textures_enabled) {
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, textures[current_texture]);
        glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    }

    // Draw ground
    draw_ground();

    // Draw all cubes
    for (int i = 0; i < NUM_CUBES; i++) {
        glPushMatrix();

        glTranslatef(cubes[i].x, cubes[i].y, cubes[i].z);
        glRotatef(cubes[i].rot_x * 180.0f / PI, 1.0f, 0.0f, 0.0f);
        glRotatef(cubes[i].rot_y * 180.0f / PI, 0.0f, 1.0f, 0.0f);
        glRotatef(cubes[i].rot_z * 180.0f / PI, 0.0f, 0.0f, 1.0f);

        // Set material color (tints the texture)
        float ambient[] = {cubes[i].color.x * 0.2f, cubes[i].color.y * 0.2f, cubes[i].color.z * 0.2f, 1.0f};
        float diffuse[] = {cubes[i].color.x, cubes[i].color.y, cubes[i].color.z, 1.0f};
        float specular[] = {0.5f, 0.5f, 0.5f, 1.0f};
        glMaterialfv(GL_FRONT, GL_AMBIENT, ambient);
        glMaterialfv(GL_FRONT, GL_DIFFUSE, diffuse);
        glMaterialfv(GL_FRONT, GL_SPECULAR, specular);
        glMaterialf(GL_FRONT, GL_SHININESS, 32.0f);

        draw_textured_cube();

        glPopMatrix();
    }

    if (textures_enabled) {
        glDisable(GL_TEXTURE_2D);
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

        const char* texture_names[] = {"Checkerboard", "Noise", "Gradient", "Brick"};
        char title[256];
        sprintf(title, "FP-ASM Textures | %s | %d cubes | %.1f FPS | Textures: %s",
                texture_names[current_texture], NUM_CUBES, fps,
                textures_enabled ? "ON" : "OFF");
        SetWindowTextA(hwnd, title);
    }
}

void setup_opengl() {
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);

    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);

    float light_ambient[] = {0.2f, 0.2f, 0.2f, 1.0f};
    float light_diffuse[] = {1.0f, 1.0f, 1.0f, 1.0f};
    float light_specular[] = {1.0f, 1.0f, 1.0f, 1.0f};

    glLightfv(GL_LIGHT0, GL_AMBIENT, light_ambient);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, light_diffuse);
    glLightfv(GL_LIGHT0, GL_SPECULAR, light_specular);

    glViewport(0, 0, 1280, 720);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(60.0, 1280.0 / 720.0, 0.1, 1000.0);
    glMatrixMode(GL_MODELVIEW);

    glClearColor(0.1f, 0.1f, 0.2f, 1.0f);
    glShadeModel(GL_SMOOTH);

    // Create procedural textures
    create_textures();
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_CLOSE:
            PostQuitMessage(0);
            return 0;
        case WM_KEYDOWN:
            if (wParam == VK_ESCAPE) {
                PostQuitMessage(0);
            } else if (wParam == 'T') {
                textures_enabled = !textures_enabled;
            } else if (wParam == '1') {
                current_texture = 0;
            } else if (wParam == '2') {
                current_texture = 1;
            } else if (wParam == '3') {
                current_texture = 2;
            } else if (wParam == '4') {
                current_texture = 3;
            }
            return 0;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    WNDCLASSA wc = {0};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = "FPASMTextures";
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    RegisterClassA(&wc);

    hwnd = CreateWindowExA(0, "FPASMTextures", "FP-ASM Texture Mapping Demo",
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

    setup_opengl();
    init_cubes();

    last_time = clock();

    printf("==============================================\n");
    printf("  FP-ASM Texture Mapping Demo\n");
    printf("==============================================\n");
    printf("Enhancement #1c: Procedural Textures\n");
    printf("\n");
    printf("Controls:\n");
    printf("  W/A/S/D - Move camera\n");
    printf("  Q/E     - Move up/down\n");
    printf("  1       - Checkerboard texture\n");
    printf("  2       - Noise texture\n");
    printf("  3       - Gradient texture\n");
    printf("  4       - Brick texture\n");
    printf("  T       - Toggle textures on/off\n");
    printf("  ESC     - Exit\n");
    printf("\n");
    printf("Number of Cubes: %d\n", NUM_CUBES);
    printf("Texture Size: %dx%d\n", TEXTURE_SIZE, TEXTURE_SIZE);
    printf("==============================================\n\n");

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

        update_cubes(16.0f);
        render();

        Sleep(1);
    }

    glDeleteTextures(4, textures);
    wglMakeCurrent(NULL, NULL);
    wglDeleteContext(hglrc);
    ReleaseDC(hwnd, hdc);
    free(cubes);

    return 0;
}
