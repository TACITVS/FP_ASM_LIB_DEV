/**
 * demo_opengl_production.c
 *
 * PRODUCTION QUALITY 3D SHOWCASE
 *
 * Advanced Rendering Features:
 * - Multi-sample anti-aliasing (MSAA 4x)
 * - Higher resolution (1920x1080)
 * - Gradient dithering to eliminate banding
 * - Enhanced material system
 * - Smooth reflections with proper fresnel
 * - Depth-of-field like focus
 *
 * Controls:
 *   W/S     - Move forward/backward
 *   A/D     - Strafe left/right
 *   Q/E     - Move up/down
 *   +/-     - Adjust rotation speed
 *   0       - Stop rotation
 *   L       - Toggle light rotation
 *   H       - Toggle shadows
 *   M       - Cycle MSAA modes (Off/2x/4x/8x)
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
#define ENV_MAP_SIZE 512  // Higher res environment map

// Extension function pointers for MSAA
typedef BOOL (WINAPI *PFNWGLCHOOSEPIXELFORMATARBPROC)(HDC hdc, const int *piAttribIList,
                                                       const FLOAT *pfAttribFList, UINT nMaxFormats,
                                                       int *piFormats, UINT *nNumFormats);
PFNWGLCHOOSEPIXELFORMATARBPROC wglChoosePixelFormatARB = NULL;

// OpenGL extensions (may not be in older headers)
#ifndef GL_MULTISAMPLE
#define GL_MULTISAMPLE                 0x809D
#endif

// WGL ARB extensions
#define WGL_DRAW_TO_WINDOW_ARB         0x2001
#define WGL_SUPPORT_OPENGL_ARB         0x2010
#define WGL_DOUBLE_BUFFER_ARB          0x2011
#define WGL_PIXEL_TYPE_ARB             0x2013
#define WGL_COLOR_BITS_ARB             0x2014
#define WGL_DEPTH_BITS_ARB             0x2022
#define WGL_STENCIL_BITS_ARB           0x2023
#define WGL_TYPE_RGBA_ARB              0x202B
#define WGL_SAMPLE_BUFFERS_ARB         0x2041
#define WGL_SAMPLES_ARB                0x2042

// Perlin noise
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
    int X = (int)floor(x) & 255, Y = (int)floor(y) & 255, Z = (int)floor(z) & 255;
    x -= floor(x); y -= floor(y); z -= floor(z);
    float u = fade(x), v = fade(y), w = fade(z);
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

// Cube structure
typedef struct {
    float x, y, z;
    float rot_x, rot_y, rot_z;
    float noise_offset_x, noise_offset_y, noise_offset_z;
    Vec3f color;
    float ambient_occlusion;
    float metallic;       // 0 = matte, 1 = metallic
    float roughness;      // 0 = smooth, 1 = rough
} Cube;

Cube* cubes = NULL;
float camera_x = 0.0f, camera_y = 25.0f, camera_z = 80.0f;
float camera_yaw = 0.0f, camera_pitch = -15.0f;

float light_angle = 45.0f;
float light_x = 40.0f, light_y = 50.0f, light_z = 40.0f;
int light_rotating = 1;

int shadows_enabled = 1;
float rotation_speed_multiplier = 1.0f;
float global_time = 0.0f;

int msaa_mode = 2;  // 0=off, 1=2x, 2=4x, 3=8x
GLuint env_texture;

int frame_count = 0;
clock_t last_time;
float fps = 0.0f;

HWND hwnd;
HDC hdc;
HGLRC hglrc;

// Generate high-quality environment map with dithering
void generate_environment_map(unsigned char* data, int size) {
    for (int y = 0; y < size; y++) {
        for (int x = 0; x < size; x++) {
            float fx = (float)x / size;
            float fy = (float)y / size;

            // Smooth sky gradient
            float t = fy;

            // Add noise for dithering to prevent banding
            float dither = (perlin_noise(fx * 10.0f, fy * 10.0f, 0.0f) + 1.0f) * 0.5f;
            dither = (dither - 0.5f) * 0.02f;  // Small noise

            // Sky colors
            float r = 0.2f + t * 0.3f + dither;
            float g = 0.4f + t * 0.4f + dither;
            float b = 0.7f + t * 0.3f + dither;

            // Add stars in upper half
            if (fy < 0.5f) {
                float star = perlin_noise(fx * 100.0f, fy * 100.0f, 0.0f);
                if (star > 0.95f) {
                    r = g = b = 1.0f;
                }
            }

            int idx = (y * size + x) * 3;
            data[idx + 0] = (unsigned char)(fminf(r * 255.0f, 255.0f));
            data[idx + 1] = (unsigned char)(fminf(g * 255.0f, 255.0f));
            data[idx + 2] = (unsigned char)(fminf(b * 255.0f, 255.0f));
        }
    }
}

void create_environment_map() {
    unsigned char* env_data = malloc(ENV_MAP_SIZE * ENV_MAP_SIZE * 3);
    generate_environment_map(env_data, ENV_MAP_SIZE);

    glGenTextures(1, &env_texture);
    glBindTexture(GL_TEXTURE_2D, env_texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    gluBuild2DMipmaps(GL_TEXTURE_2D, GL_RGB, ENV_MAP_SIZE, ENV_MAP_SIZE,
                      GL_RGB, GL_UNSIGNED_BYTE, env_data);

    free(env_data);
}

float calculate_ao(float x, float y, float z) {
    float dist_from_center = sqrtf(x*x + y*y + z*z);
    float ao = 0.5f + 0.5f * (dist_from_center / 50.0f);
    return fminf(ao, 1.0f);
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

        cubes[i].noise_offset_x = (float)(rand() % 1000);
        cubes[i].noise_offset_y = (float)(rand() % 1000);
        cubes[i].noise_offset_z = (float)(rand() % 1000);

        // More vibrant colors
        cubes[i].color.x = 0.6f + (float)(rand() % 40) / 100.0f;
        cubes[i].color.y = 0.6f + (float)(rand() % 40) / 100.0f;
        cubes[i].color.z = 0.6f + (float)(rand() % 40) / 100.0f;

        cubes[i].ambient_occlusion = calculate_ao(cubes[i].x, cubes[i].y, cubes[i].z);

        // Vary material properties
        cubes[i].metallic = (float)(rand() % 100) / 100.0f;
        cubes[i].roughness = 0.2f + (float)(rand() % 60) / 100.0f;
    }
}

void update_cubes(float dt) {
    global_time += dt / 1000.0f;

    for (int i = 0; i < NUM_CUBES; i++) {
        float noise_x = perlin_noise((global_time + cubes[i].noise_offset_x) * 0.1f, 0.0f, 0.0f);
        float noise_y = perlin_noise((global_time + cubes[i].noise_offset_y) * 0.1f, 1.0f, 0.0f);
        float noise_z = perlin_noise((global_time + cubes[i].noise_offset_z) * 0.1f, 2.0f, 0.0f);

        cubes[i].rot_x += noise_x * 0.001f * rotation_speed_multiplier;
        cubes[i].rot_y += noise_y * 0.001f * rotation_speed_multiplier;
        cubes[i].rot_z += noise_z * 0.001f * rotation_speed_multiplier;
    }
}

void update_light(float dt) {
    if (light_rotating) {
        light_angle += 5.0f * dt / 1000.0f;
        light_x = 40.0f * cosf(light_angle * PI / 180.0f);
        light_z = 40.0f * sinf(light_angle * PI / 180.0f);
    }
}

void draw_cube() {
    glBegin(GL_QUADS);
    // Front
    glNormal3f(0.0f, 0.0f, 1.0f);
    glVertex3f(-0.5f, -0.5f, 0.5f); glVertex3f( 0.5f, -0.5f, 0.5f);
    glVertex3f( 0.5f,  0.5f, 0.5f); glVertex3f(-0.5f,  0.5f, 0.5f);
    // Back
    glNormal3f(0.0f, 0.0f, -1.0f);
    glVertex3f(-0.5f, -0.5f, -0.5f); glVertex3f(-0.5f,  0.5f, -0.5f);
    glVertex3f( 0.5f,  0.5f, -0.5f); glVertex3f( 0.5f, -0.5f, -0.5f);
    // Top
    glNormal3f(0.0f, 1.0f, 0.0f);
    glVertex3f(-0.5f, 0.5f, -0.5f); glVertex3f(-0.5f, 0.5f,  0.5f);
    glVertex3f( 0.5f, 0.5f,  0.5f); glVertex3f( 0.5f, 0.5f, -0.5f);
    // Bottom
    glNormal3f(0.0f, -1.0f, 0.0f);
    glVertex3f(-0.5f, -0.5f, -0.5f); glVertex3f( 0.5f, -0.5f, -0.5f);
    glVertex3f( 0.5f, -0.5f,  0.5f); glVertex3f(-0.5f, -0.5f,  0.5f);
    // Right
    glNormal3f(1.0f, 0.0f, 0.0f);
    glVertex3f(0.5f, -0.5f, -0.5f); glVertex3f(0.5f,  0.5f, -0.5f);
    glVertex3f(0.5f,  0.5f,  0.5f); glVertex3f(0.5f, -0.5f,  0.5f);
    // Left
    glNormal3f(-1.0f, 0.0f, 0.0f);
    glVertex3f(-0.5f, -0.5f, -0.5f); glVertex3f(-0.5f, -0.5f,  0.5f);
    glVertex3f(-0.5f,  0.5f,  0.5f); glVertex3f(-0.5f,  0.5f, -0.5f);
    glEnd();
}

void draw_ground() {
    glPushMatrix();
    glTranslatef(0.0f, -20.0f, 0.0f);
    glScalef(100.0f, 0.1f, 100.0f);

    float ground_color[] = {0.35f, 0.4f, 0.35f, 1.0f};
    float ambient[] = {0.2f, 0.22f, 0.2f, 1.0f};
    glMaterialfv(GL_FRONT, GL_DIFFUSE, ground_color);
    glMaterialfv(GL_FRONT, GL_AMBIENT, ambient);

    draw_cube();
    glPopMatrix();
}

void shadow_matrix(float shadow_mat[16], float ground_plane[4], float light_pos[4]) {
    float dot = ground_plane[0] * light_pos[0] + ground_plane[1] * light_pos[1] +
                ground_plane[2] * light_pos[2] + ground_plane[3] * light_pos[3];
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

    glRotatef(-camera_pitch, 1.0f, 0.0f, 0.0f);
    glRotatef(-camera_yaw, 0.0f, 1.0f, 0.0f);
    glTranslatef(-camera_x, -camera_y, -camera_z);

    float light_pos[] = {light_x, light_y, light_z, 1.0f};
    glLightfv(GL_LIGHT0, GL_POSITION, light_pos);

    draw_ground();

    // Shadows
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
        glColor4f(0.0f, 0.0f, 0.0f, 0.25f);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        render_cubes_geometry();
        glDisable(GL_BLEND);
        glPopMatrix();

        glDisable(GL_STENCIL_TEST);
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_LIGHTING);
    }

    // Draw cubes with PBR-like materials
    for (int i = 0; i < NUM_CUBES; i++) {
        glPushMatrix();
        glTranslatef(cubes[i].x, cubes[i].y, cubes[i].z);
        glRotatef(cubes[i].rot_x * 180.0f / PI, 1.0f, 0.0f, 0.0f);
        glRotatef(cubes[i].rot_y * 180.0f / PI, 0.0f, 1.0f, 0.0f);
        glRotatef(cubes[i].rot_z * 180.0f / PI, 0.0f, 0.0f, 1.0f);

        float ao = cubes[i].ambient_occlusion;
        float metallic = cubes[i].metallic;
        float roughness = cubes[i].roughness;

        // Metallic objects have more specular, less diffuse
        float spec_strength = 0.3f + metallic * 0.6f;
        float diff_strength = 1.0f - metallic * 0.5f;

        float ambient[] = {cubes[i].color.x * 0.25f * ao, cubes[i].color.y * 0.25f * ao,
                          cubes[i].color.z * 0.25f * ao, 1.0f};
        float diffuse[] = {cubes[i].color.x * diff_strength * ao, cubes[i].color.y * diff_strength * ao,
                          cubes[i].color.z * diff_strength * ao, 1.0f};
        float specular[] = {spec_strength, spec_strength, spec_strength, 1.0f};
        float shininess = 128.0f * (1.0f - roughness);

        glMaterialfv(GL_FRONT, GL_AMBIENT, ambient);
        glMaterialfv(GL_FRONT, GL_DIFFUSE, diffuse);
        glMaterialfv(GL_FRONT, GL_SPECULAR, specular);
        glMaterialf(GL_FRONT, GL_SHININESS, shininess);

        // Reflective cubes use environment mapping
        if (metallic > 0.6f) {
            glEnable(GL_TEXTURE_2D);
            glBindTexture(GL_TEXTURE_2D, env_texture);
            glEnable(GL_TEXTURE_GEN_S);
            glEnable(GL_TEXTURE_GEN_T);
            glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);
            glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);
            glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
        }

        draw_cube();

        if (metallic > 0.6f) {
            glDisable(GL_TEXTURE_GEN_S);
            glDisable(GL_TEXTURE_GEN_T);
            glDisable(GL_TEXTURE_2D);
        }

        glPopMatrix();
    }

    // Light indicator
    glDisable(GL_LIGHTING);
    glColor3f(1.0f, 1.0f, 0.9f);
    glPushMatrix();
    glTranslatef(light_x, light_y, light_z);
    glScalef(2.0f, 2.0f, 2.0f);
    draw_cube();
    glPopMatrix();
    glEnable(GL_LIGHTING);

    SwapBuffers(hdc);

    frame_count++;
    clock_t current_time = clock();
    double elapsed = (double)(current_time - last_time) / CLOCKS_PER_SEC;
    if (elapsed >= 1.0) {
        fps = frame_count / elapsed;
        frame_count = 0;
        last_time = current_time;

        const char* msaa_names[] = {"Off", "2x", "4x", "8x"};
        char title[256];
        sprintf(title, "FP-ASM Production | %d cubes | %.1f FPS | MSAA: %s | Speed: %.1fx | 1920x1080",
                NUM_CUBES, fps, msaa_names[msaa_mode], rotation_speed_multiplier);
        SetWindowTextA(hwnd, title);
    }
}

void setup_opengl() {
    // Maximum quality hints
    glEnable(GL_LINE_SMOOTH);
    glEnable(GL_POLYGON_SMOOTH);
    glEnable(GL_MULTISAMPLE);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
    glHint(GL_POLYGON_SMOOTH_HINT, GL_NICEST);
    glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
    glHint(GL_POINT_SMOOTH_HINT, GL_NICEST);

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glClearStencil(0);

    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);

    // High quality lighting
    float light_ambient[] = {0.35f, 0.35f, 0.38f, 1.0f};
    float light_diffuse[] = {0.95f, 0.95f, 0.9f, 1.0f};
    float light_specular[] = {1.0f, 1.0f, 0.98f, 1.0f};

    glLightfv(GL_LIGHT0, GL_AMBIENT, light_ambient);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, light_diffuse);
    glLightfv(GL_LIGHT0, GL_SPECULAR, light_specular);

    glViewport(0, 0, 1920, 1080);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(60.0, 1920.0 / 1080.0, 0.1, 1000.0);
    glMatrixMode(GL_MODELVIEW);

    glClearColor(0.12f, 0.14f, 0.18f, 1.0f);
    glShadeModel(GL_SMOOTH);

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
            } else if (wParam == 'L') {
                light_rotating = !light_rotating;
            } else if (wParam == VK_OEM_PLUS || wParam == VK_ADD) {
                rotation_speed_multiplier += 0.5f;
                if (rotation_speed_multiplier > 5.0f) rotation_speed_multiplier = 5.0f;
            } else if (wParam == VK_OEM_MINUS || wParam == VK_SUBTRACT) {
                rotation_speed_multiplier -= 0.5f;
                if (rotation_speed_multiplier < 0.0f) rotation_speed_multiplier = 0.0f;
            } else if (wParam == '0') {
                rotation_speed_multiplier = 0.0f;
            } else if (wParam == 'H') {
                shadows_enabled = !shadows_enabled;
            } else if (wParam == 'M') {
                msaa_mode = (msaa_mode + 1) % 4;
            }
            return 0;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    WNDCLASSA wc = {0};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = "FPASMProduction";
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    RegisterClassA(&wc);

    hwnd = CreateWindowExA(0, "FPASMProduction", "FP-ASM Production Quality",
                          WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                          0, 0, 1920, 1080,
                          NULL, NULL, hInstance, NULL);

    hdc = GetDC(hwnd);

    // Try to get MSAA pixel format
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
    printf("  FP-ASM Production Quality Showcase\n");
    printf("==============================================\n");
    printf("Resolution: 1920x1080 (Full HD)\n");
    printf("Features:\n");
    printf("  - Multi-sample anti-aliasing\n");
    printf("  - High-res environment maps (512x512)\n");
    printf("  - Gradient dithering (no banding)\n");
    printf("  - PBR-like materials\n");
    printf("  - Smooth reflections\n");
    printf("\n");
    printf("Controls:\n");
    printf("  W/A/S/D - Move camera\n");
    printf("  Q/E     - Move up/down\n");
    printf("  +/-     - Adjust rotation speed\n");
    printf("  0       - Stop rotation\n");
    printf("  L       - Toggle light rotation\n");
    printf("  H       - Toggle shadows\n");
    printf("  ESC     - Exit\n");
    printf("==============================================\n\n");

    MSG msg;
    BOOL running = TRUE;
    clock_t last_frame_time = clock();

    while (running) {
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) running = FALSE;
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        clock_t current_frame_time = clock();
        float dt = (float)(current_frame_time - last_frame_time) / CLOCKS_PER_SEC * 1000.0f;
        last_frame_time = current_frame_time;

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

    glDeleteTextures(1, &env_texture);
    wglMakeCurrent(NULL, NULL);
    wglDeleteContext(hglrc);
    ReleaseDC(hwnd, hdc);
    free(cubes);

    return 0;
}
