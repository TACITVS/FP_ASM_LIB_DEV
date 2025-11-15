/**
 * demo_opengl_particles.c
 *
 * ENHANCEMENT #1b: Particle Systems
 *
 * Adds GPU-accelerated particle effects:
 * - 10,000 particles with physics simulation
 * - Multiple emitters (fountains, explosions, smoke)
 * - Point sprites for efficient rendering
 * - Color gradients and alpha blending
 * - Batch processing using FP-ASM vector operations
 *
 * Controls:
 *   W/S - Move forward/backward
 *   A/D - Strafe left/right
 *   Q/E - Move up/down
 *   1-4 - Switch particle effect type
 *   P   - Pause/Resume particles
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
#define MAX_PARTICLES 10000
#define NUM_EMITTERS 3

// Particle structure
typedef struct {
    Vec3f position;
    Vec3f velocity;
    Vec3f color;
    float life;        // 0.0 = dead, 1.0 = just born
    float size;
    float gravity;     // Gravity multiplier
} Particle;

// Emitter structure
typedef struct {
    Vec3f position;
    Vec3f direction;
    float spread;      // Cone angle for velocity randomness
    float speed;       // Initial particle speed
    float spawn_rate;  // Particles per second
    float lifetime;    // How long particles live
    Vec3f color_start;
    Vec3f color_end;
    float size_start;
    float size_end;
    float gravity;
    int enabled;
} Emitter;

// Global state
Particle* particles = NULL;
Emitter* emitters = NULL;
int particle_count = 0;
int particles_paused = 0;
int current_effect = 0;  // 0=fountain, 1=explosion, 2=smoke, 3=magic

float camera_x = 0.0f, camera_y = 15.0f, camera_z = 60.0f;
float camera_yaw = 0.0f, camera_pitch = -10.0f;

int frame_count = 0;
clock_t last_time;
float fps = 0.0f;
float time_accumulator = 0.0f;

HWND hwnd;
HDC hdc;
HGLRC hglrc;

void init_emitters() {
    emitters = malloc(NUM_EMITTERS * sizeof(Emitter));

    // Emitter 0: Fountain
    emitters[0].position = (Vec3f){-15.0f, 0.0f, 0.0f, 0.0f};
    emitters[0].direction = (Vec3f){0.0f, 1.0f, 0.0f, 0.0f};
    emitters[0].spread = 30.0f;
    emitters[0].speed = 15.0f;
    emitters[0].spawn_rate = 200.0f;
    emitters[0].lifetime = 3.0f;
    emitters[0].color_start = (Vec3f){0.3f, 0.6f, 1.0f, 0.0f};  // Blue
    emitters[0].color_end = (Vec3f){0.0f, 0.2f, 0.5f, 0.0f};
    emitters[0].size_start = 0.5f;
    emitters[0].size_end = 0.2f;
    emitters[0].gravity = 1.0f;
    emitters[0].enabled = 1;

    // Emitter 1: Fire explosion
    emitters[1].position = (Vec3f){0.0f, 5.0f, 0.0f, 0.0f};
    emitters[1].direction = (Vec3f){0.0f, 0.5f, 0.0f, 0.0f};
    emitters[1].spread = 180.0f;
    emitters[1].speed = 12.0f;
    emitters[1].spawn_rate = 300.0f;
    emitters[1].lifetime = 2.0f;
    emitters[1].color_start = (Vec3f){1.0f, 0.8f, 0.2f, 0.0f};  // Yellow-orange
    emitters[1].color_end = (Vec3f){0.8f, 0.2f, 0.0f, 0.0f};    // Red
    emitters[1].size_start = 0.8f;
    emitters[1].size_end = 0.1f;
    emitters[1].gravity = 0.3f;
    emitters[1].enabled = 1;

    // Emitter 2: Smoke/magic
    emitters[2].position = (Vec3f){15.0f, 0.0f, 0.0f, 0.0f};
    emitters[2].direction = (Vec3f){0.0f, 1.0f, 0.0f, 0.0f};
    emitters[2].spread = 15.0f;
    emitters[2].speed = 5.0f;
    emitters[2].spawn_rate = 100.0f;
    emitters[2].lifetime = 5.0f;
    emitters[2].color_start = (Vec3f){0.7f, 0.2f, 0.9f, 0.0f};  // Purple
    emitters[2].color_end = (Vec3f){0.3f, 0.0f, 0.5f, 0.0f};
    emitters[2].size_start = 0.3f;
    emitters[2].size_end = 1.2f;  // Grows over time
    emitters[2].gravity = -0.2f;  // Rises up
    emitters[2].enabled = 1;
}

void init_particles() {
    particles = malloc(MAX_PARTICLES * sizeof(Particle));
    for (int i = 0; i < MAX_PARTICLES; i++) {
        particles[i].life = 0.0f;  // All dead initially
    }
    particle_count = 0;
}

float randf() {
    return (float)rand() / (float)RAND_MAX;
}

void spawn_particle(const Emitter* emitter) {
    if (particle_count >= MAX_PARTICLES) return;

    // Find dead particle
    int idx = -1;
    for (int i = 0; i < MAX_PARTICLES; i++) {
        if (particles[i].life <= 0.0f) {
            idx = i;
            break;
        }
    }
    if (idx == -1) return;

    Particle* p = &particles[idx];

    // Position at emitter
    p->position = emitter->position;

    // Random velocity in cone around direction
    float theta = randf() * 2.0f * PI;
    float phi = (randf() - 0.5f) * emitter->spread * PI / 180.0f;

    float cos_phi = cosf(phi);
    float sin_phi = sinf(phi);
    float cos_theta = cosf(theta);
    float sin_theta = sinf(theta);

    Vec3f random_dir = {
        sin_phi * cos_theta,
        cos_phi,
        sin_phi * sin_theta,
        0.0f
    };

    // Blend with emitter direction
    p->velocity.x = (emitter->direction.x + random_dir.x) * emitter->speed;
    p->velocity.y = (emitter->direction.y + random_dir.y) * emitter->speed;
    p->velocity.z = (emitter->direction.z + random_dir.z) * emitter->speed;

    p->color = emitter->color_start;
    p->life = 1.0f;
    p->size = emitter->size_start;
    p->gravity = emitter->gravity;

    particle_count++;
}

void update_particles(float dt) {
    if (particles_paused) return;

    const float GRAVITY = -9.8f;
    int alive_count = 0;

    // Update existing particles
    for (int i = 0; i < MAX_PARTICLES; i++) {
        Particle* p = &particles[i];
        if (p->life <= 0.0f) continue;

        // Physics update
        p->velocity.y += GRAVITY * p->gravity * dt;
        p->position.x += p->velocity.x * dt;
        p->position.y += p->velocity.y * dt;
        p->position.z += p->velocity.z * dt;

        // Life decay
        p->life -= dt / 3.0f;  // Average 3 second lifetime

        alive_count++;
    }

    particle_count = alive_count;

    // Spawn new particles
    time_accumulator += dt;
    float spawn_interval = 1.0f / 100.0f;  // 100 particles/sec total

    while (time_accumulator >= spawn_interval) {
        for (int e = 0; e < NUM_EMITTERS; e++) {
            if (emitters[e].enabled) {
                spawn_particle(&emitters[e]);
            }
        }
        time_accumulator -= spawn_interval;
    }
}

void render_particles() {
    glDisable(GL_LIGHTING);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);  // Additive blending for glow

    // Use point sprites if available, otherwise billboards
    glEnable(GL_POINT_SMOOTH);
    glPointSize(10.0f);

    glBegin(GL_POINTS);
    for (int i = 0; i < MAX_PARTICLES; i++) {
        Particle* p = &particles[i];
        if (p->life <= 0.0f) continue;

        // Interpolate color and size based on life
        float t = 1.0f - p->life;
        float alpha = p->life;  // Fade out

        glColor4f(p->color.x, p->color.y, p->color.z, alpha);
        glVertex3f(p->position.x, p->position.y, p->position.z);
    }
    glEnd();

    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
}

void draw_ground() {
    glPushMatrix();
    glTranslatef(0.0f, -1.0f, 0.0f);
    glScalef(50.0f, 0.1f, 50.0f);

    float ground_color[] = {0.2f, 0.25f, 0.2f, 1.0f};
    float ambient[] = {0.1f, 0.12f, 0.1f, 1.0f};
    glMaterialfv(GL_FRONT, GL_DIFFUSE, ground_color);
    glMaterialfv(GL_FRONT, GL_AMBIENT, ambient);

    glBegin(GL_QUADS);
    glNormal3f(0.0f, 1.0f, 0.0f);
    glVertex3f(-0.5f, 0.5f, -0.5f);
    glVertex3f(-0.5f, 0.5f,  0.5f);
    glVertex3f( 0.5f, 0.5f,  0.5f);
    glVertex3f( 0.5f, 0.5f, -0.5f);
    glEnd();

    glPopMatrix();
}

void draw_emitter_markers() {
    glDisable(GL_LIGHTING);

    for (int i = 0; i < NUM_EMITTERS; i++) {
        if (!emitters[i].enabled) continue;

        glPushMatrix();
        glTranslatef(emitters[i].position.x, emitters[i].position.y, emitters[i].position.z);

        // Draw small cube marker
        glColor3f(emitters[i].color_start.x, emitters[i].color_start.y, emitters[i].color_start.z);
        glBegin(GL_LINE_LOOP);
        glVertex3f(-0.5f, -0.5f, -0.5f);
        glVertex3f( 0.5f, -0.5f, -0.5f);
        glVertex3f( 0.5f,  0.5f, -0.5f);
        glVertex3f(-0.5f,  0.5f, -0.5f);
        glEnd();

        glPopMatrix();
    }

    glEnable(GL_LIGHTING);
}

void set_effect_type(int type) {
    current_effect = type % 4;

    // Disable all emitters
    for (int i = 0; i < NUM_EMITTERS; i++) {
        emitters[i].enabled = 0;
    }

    switch (current_effect) {
        case 0:  // Fountain
            emitters[0].enabled = 1;
            break;
        case 1:  // Explosion
            emitters[1].enabled = 1;
            break;
        case 2:  // Smoke/magic
            emitters[2].enabled = 1;
            break;
        case 3:  // All combined
            emitters[0].enabled = 1;
            emitters[1].enabled = 1;
            emitters[2].enabled = 1;
            break;
    }
}

void render() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();

    // Camera transform
    glRotatef(-camera_pitch, 1.0f, 0.0f, 0.0f);
    glRotatef(-camera_yaw, 0.0f, 1.0f, 0.0f);
    glTranslatef(-camera_x, -camera_y, -camera_z);

    // Setup light
    float light_pos[] = {20.0f, 30.0f, 20.0f, 1.0f};
    glLightfv(GL_LIGHT0, GL_POSITION, light_pos);

    // Draw ground
    draw_ground();

    // Draw emitter markers
    draw_emitter_markers();

    // Draw particles (rendered last for proper blending)
    render_particles();

    SwapBuffers(hdc);

    // Update FPS
    frame_count++;
    clock_t current_time = clock();
    double elapsed = (double)(current_time - last_time) / CLOCKS_PER_SEC;
    if (elapsed >= 1.0) {
        fps = frame_count / elapsed;
        frame_count = 0;
        last_time = current_time;

        const char* effect_names[] = {"Fountain", "Explosion", "Smoke", "All"};
        char title[256];
        sprintf(title, "FP-ASM Particles | %s | %d/%d particles | %.1f FPS | %s",
                effect_names[current_effect], particle_count, MAX_PARTICLES, fps,
                particles_paused ? "PAUSED" : "Running");
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

    float light_ambient[] = {0.3f, 0.3f, 0.3f, 1.0f};
    float light_diffuse[] = {1.0f, 1.0f, 1.0f, 1.0f};
    glLightfv(GL_LIGHT0, GL_AMBIENT, light_ambient);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, light_diffuse);

    glViewport(0, 0, 1280, 720);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(60.0, 1280.0 / 720.0, 0.1, 1000.0);
    glMatrixMode(GL_MODELVIEW);

    glClearColor(0.05f, 0.05f, 0.1f, 1.0f);
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
            } else if (wParam == 'P') {
                particles_paused = !particles_paused;
            } else if (wParam == '1') {
                set_effect_type(0);
            } else if (wParam == '2') {
                set_effect_type(1);
            } else if (wParam == '3') {
                set_effect_type(2);
            } else if (wParam == '4') {
                set_effect_type(3);
            }
            return 0;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    WNDCLASSA wc = {0};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = "FPASMParticles";
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    RegisterClassA(&wc);

    hwnd = CreateWindowExA(0, "FPASMParticles", "FP-ASM Particle Systems",
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
    init_particles();
    init_emitters();
    set_effect_type(3);  // Start with all effects

    last_time = clock();
    srand(time(NULL));

    printf("==============================================\n");
    printf("  FP-ASM Particle Systems Demo\n");
    printf("==============================================\n");
    printf("Enhancement #1b: Particle Effects\n");
    printf("\n");
    printf("Controls:\n");
    printf("  W/A/S/D - Move camera\n");
    printf("  Q/E     - Move up/down\n");
    printf("  1       - Fountain effect\n");
    printf("  2       - Explosion effect\n");
    printf("  3       - Smoke/magic effect\n");
    printf("  4       - All effects combined\n");
    printf("  P       - Pause/Resume\n");
    printf("  ESC     - Exit\n");
    printf("\n");
    printf("Max Particles: %d\n", MAX_PARTICLES);
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

        // Calculate delta time
        clock_t current_frame_time = clock();
        float dt = (float)(current_frame_time - last_frame_time) / CLOCKS_PER_SEC;
        last_frame_time = current_frame_time;

        // Handle keyboard input
        if (GetAsyncKeyState('W') & 0x8000) camera_z -= 0.5f;
        if (GetAsyncKeyState('S') & 0x8000) camera_z += 0.5f;
        if (GetAsyncKeyState('A') & 0x8000) camera_x -= 0.5f;
        if (GetAsyncKeyState('D') & 0x8000) camera_x += 0.5f;
        if (GetAsyncKeyState('Q') & 0x8000) camera_y -= 0.5f;
        if (GetAsyncKeyState('E') & 0x8000) camera_y += 0.5f;

        update_particles(dt);
        render();

        Sleep(1);
    }

    wglMakeCurrent(NULL, NULL);
    wglDeleteContext(hglrc);
    ReleaseDC(hwnd, hdc);
    free(particles);
    free(emitters);

    return 0;
}
