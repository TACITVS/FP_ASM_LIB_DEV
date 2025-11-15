/**
 * demo_opengl_reflections.c
 *
 * ENHANCEMENT #1d: Environment Reflections
 *
 * Adds reflective surfaces using sphere mapping:
 * - Procedural environment map
 * - Sphere mapping for reflections
 * - Mix of reflective and matte materials
 * - Real-time reflection updates
 *
 * Controls:
 *   W/S - Move forward/backward
 *   A/D - Strafe left/right
 *   Q/E - Move up/down
 *   R   - Toggle reflections on/off
 *   1-3 - Adjust reflection intensity
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
#define NUM_CUBES 200
#define ENV_MAP_SIZE 256

// Global state
typedef struct {
    float x, y, z;
    float rot_x, rot_y, rot_z;
    float rot_speed_x, rot_speed_y, rot_speed_z;
    Vec3f color;
    float reflectivity;  // 0.0 = matte, 1.0 = fully reflective
} Cube;

Cube* cubes = NULL;
float camera_x = 0.0f, camera_y = 20.0f, camera_z = 70.0f;
float camera_yaw = 0.0f, camera_pitch = -10.0f;

GLuint env_map_texture;
int reflections_enabled = 1;
float reflection_intensity = 0.7f;

int frame_count = 0;
clock_t last_time;
float fps = 0.0f;

HWND hwnd;
HDC hdc;
HGLRC hglrc;

// Generate procedural environment map (sky gradient)
void generate_environment_map(unsigned char* data, int size) {
    for (int y = 0; y < size; y++) {
        for (int x = 0; x < size; x++) {
            // Convert to sphere coordinates
            float fx = (float)x / size * 2.0f - 1.0f;
            float fy = (float)y / size * 2.0f - 1.0f;

            // Sky gradient from blue to cyan
            float t = (fy + 1.0f) * 0.5f;

            // Add some stars (white noise)
            int star = ((x * 127 + y * 311) % 1000) < 5 ? 1 : 0;

            int idx = (y * size + x) * 3;
            if (star) {
                data[idx + 0] = 255;
                data[idx + 1] = 255;
                data[idx + 2] = 255;
            } else {
                data[idx + 0] = (unsigned char)(50 + t * 50);      // Red
                data[idx + 1] = (unsigned char)(100 + t * 100);    // Green
                data[idx + 2] = (unsigned char)(200 + t * 55);     // Blue
            }
        }
    }
}

void create_environment_map() {
    unsigned char* env_data = malloc(ENV_MAP_SIZE * ENV_MAP_SIZE * 3);
    generate_environment_map(env_data, ENV_MAP_SIZE);

    glGenTextures(1, &env_map_texture);
    glBindTexture(GL_TEXTURE_2D, env_map_texture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, ENV_MAP_SIZE, ENV_MAP_SIZE, 0,
                 GL_RGB, GL_UNSIGNED_BYTE, env_data);

    free(env_data);
}

void init_cubes() {
    cubes = malloc(NUM_CUBES * sizeof(Cube));
    srand(42);

    int grid = (int)cbrt((float)NUM_CUBES) + 1;
    for (int i = 0; i < NUM_CUBES; i++) {
        cubes[i].x = ((float)(i % grid) - grid/2.0f) * 7.0f;
        cubes[i].y = ((float)((i / grid) % grid) - grid/2.0f) * 7.0f;
        cubes[i].z = ((float)(i / (grid * grid)) - grid/2.0f) * 7.0f;

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

        // Vary reflectivity - some cubes very reflective, others matte
        cubes[i].reflectivity = (float)(rand() % 100) / 100.0f;
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

    float ground_color[] = {0.2f, 0.3f, 0.2f, 1.0f};
    float ambient[] = {0.1f, 0.15f, 0.1f, 1.0f};
    glMaterialfv(GL_FRONT, GL_DIFFUSE, ground_color);
    glMaterialfv(GL_FRONT, GL_AMBIENT, ambient);

    draw_cube();
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

    // Draw ground (non-reflective)
    draw_ground();

    // Draw cubes with varying reflectivity
    for (int i = 0; i < NUM_CUBES; i++) {
        glPushMatrix();

        glTranslatef(cubes[i].x, cubes[i].y, cubes[i].z);
        glRotatef(cubes[i].rot_x * 180.0f / PI, 1.0f, 0.0f, 0.0f);
        glRotatef(cubes[i].rot_y * 180.0f / PI, 0.0f, 1.0f, 0.0f);
        glRotatef(cubes[i].rot_z * 180.0f / PI, 0.0f, 0.0f, 1.0f);

        // Base material properties
        float reflective_factor = reflections_enabled ? cubes[i].reflectivity * reflection_intensity : 0.0f;

        // For reflective cubes, use sphere mapping
        if (reflective_factor > 0.1f) {
            // Enable sphere mapping
            glEnable(GL_TEXTURE_2D);
            glBindTexture(GL_TEXTURE_2D, env_map_texture);

            glEnable(GL_TEXTURE_GEN_S);
            glEnable(GL_TEXTURE_GEN_T);
            glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);
            glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);

            // Blend texture with material color
            glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

            // Increase specularity for reflective objects
            float ambient[] = {cubes[i].color.x * 0.3f, cubes[i].color.y * 0.3f, cubes[i].color.z * 0.3f, 1.0f};
            float diffuse[] = {cubes[i].color.x * 0.5f, cubes[i].color.y * 0.5f, cubes[i].color.z * 0.5f, 1.0f};
            float specular[] = {reflective_factor, reflective_factor, reflective_factor, 1.0f};

            glMaterialfv(GL_FRONT, GL_AMBIENT, ambient);
            glMaterialfv(GL_FRONT, GL_DIFFUSE, diffuse);
            glMaterialfv(GL_FRONT, GL_SPECULAR, specular);
            glMaterialf(GL_FRONT, GL_SHININESS, 128.0f);
        } else {
            // Matte material
            float ambient[] = {cubes[i].color.x * 0.2f, cubes[i].color.y * 0.2f, cubes[i].color.z * 0.2f, 1.0f};
            float diffuse[] = {cubes[i].color.x, cubes[i].color.y, cubes[i].color.z, 1.0f};
            float specular[] = {0.1f, 0.1f, 0.1f, 1.0f};

            glMaterialfv(GL_FRONT, GL_AMBIENT, ambient);
            glMaterialfv(GL_FRONT, GL_DIFFUSE, diffuse);
            glMaterialfv(GL_FRONT, GL_SPECULAR, specular);
            glMaterialf(GL_FRONT, GL_SHININESS, 16.0f);
        }

        draw_cube();

        // Disable sphere mapping
        if (reflective_factor > 0.1f) {
            glDisable(GL_TEXTURE_GEN_S);
            glDisable(GL_TEXTURE_GEN_T);
            glDisable(GL_TEXTURE_2D);
        }

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
        sprintf(title, "FP-ASM Reflections | %d cubes | %.1f FPS | Reflections: %s | Intensity: %.0f%%",
                NUM_CUBES, fps,
                reflections_enabled ? "ON" : "OFF",
                reflection_intensity * 100.0f);
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

    glClearColor(0.05f, 0.1f, 0.15f, 1.0f);
    glShadeModel(GL_SMOOTH);

    // Create environment map
    create_environment_map();
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_CLOSE:
            PostQuitMessage(0);
            return 0;
        case WM_KEYDOWN:
            if (wParam == VK_ESCAPE) {
                PostQuitMessage(0);
            } else if (wParam == 'R') {
                reflections_enabled = !reflections_enabled;
            } else if (wParam == '1') {
                reflection_intensity = 0.3f;
            } else if (wParam == '2') {
                reflection_intensity = 0.7f;
            } else if (wParam == '3') {
                reflection_intensity = 1.0f;
            }
            return 0;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    WNDCLASSA wc = {0};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = "FPASMReflections";
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    RegisterClassA(&wc);

    hwnd = CreateWindowExA(0, "FPASMReflections", "FP-ASM Environment Reflections",
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
    printf("  FP-ASM Environment Reflections Demo\n");
    printf("==============================================\n");
    printf("Enhancement #1d: Sphere-Mapped Reflections\n");
    printf("\n");
    printf("Controls:\n");
    printf("  W/A/S/D - Move camera\n");
    printf("  Q/E     - Move up/down\n");
    printf("  R       - Toggle reflections on/off\n");
    printf("  1       - Low reflection (30%%)\n");
    printf("  2       - Medium reflection (70%%)\n");
    printf("  3       - High reflection (100%%)\n");
    printf("  ESC     - Exit\n");
    printf("\n");
    printf("Number of Cubes: %d\n", NUM_CUBES);
    printf("Environment Map: %dx%d\n", ENV_MAP_SIZE, ENV_MAP_SIZE);
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

    glDeleteTextures(1, &env_map_texture);
    wglMakeCurrent(NULL, NULL);
    wglDeleteContext(hglrc);
    ReleaseDC(hwnd, hdc);
    free(cubes);

    return 0;
}
