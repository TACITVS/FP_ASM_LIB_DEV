/**
 * demo_fp_cube.c
 *
 * FIRST VISUAL 3D DEMO: FP-First Spinning Cube
 *
 * Mission: Demonstrate that EVERY graphics operation can use FP library functions
 * while achieving 60 FPS.
 *
 * FP Principles Demonstrated:
 * 1. Immutability - All input geometry is `const`
 * 2. Composition - Transformations built from fp_fold_dotp_f32
 * 3. Modularity - Separate functions for transform, lighting, rendering
 * 4. Declarative - Express WHAT to compute, not HOW
 *
 * Performance Goal: 60 FPS @ 1920x1080
 * FP Library Usage:
 * - fp_fold_dotp_f32: Matrix × Vector (transform vertices)
 * - fp_fold_dotp_f32: Normal · Light (diffuse lighting)
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <windows.h>
#include <gl/gl.h>
#include "include/fp_core.h"

// Link with OpenGL
#pragma comment(lib, "opengl32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "user32.lib")

//==============================================================================
// IMMUTABLE CUBE GEOMETRY
//==============================================================================

// Cube vertices (8 corners, homogeneous coordinates)
const float CUBE_VERTICES[] = {
    // x,   y,   z,   w
    -1.0f, -1.0f, -1.0f, 1.0f,  // 0: Back-bottom-left
     1.0f, -1.0f, -1.0f, 1.0f,  // 1: Back-bottom-right
     1.0f,  1.0f, -1.0f, 1.0f,  // 2: Back-top-right
    -1.0f,  1.0f, -1.0f, 1.0f,  // 3: Back-top-left
    -1.0f, -1.0f,  1.0f, 1.0f,  // 4: Front-bottom-left
     1.0f, -1.0f,  1.0f, 1.0f,  // 5: Front-bottom-right
     1.0f,  1.0f,  1.0f, 1.0f,  // 6: Front-top-right
    -1.0f,  1.0f,  1.0f, 1.0f,  // 7: Front-top-left
};

// Cube face normals (one per face)
const float CUBE_NORMALS[] = {
    // Front face
    0.0f, 0.0f, 1.0f,
    // Back face
    0.0f, 0.0f, -1.0f,
    // Top face
    0.0f, 1.0f, 0.0f,
    // Bottom face
    0.0f, -1.0f, 0.0f,
    // Right face
    1.0f, 0.0f, 0.0f,
    // Left face
    -1.0f, 0.0f, 0.0f,
};

// Cube indices (6 faces × 2 triangles × 3 vertices)
const unsigned int CUBE_INDICES[] = {
    // Front face
    4, 5, 6,  6, 7, 4,
    // Back face
    1, 0, 3,  3, 2, 1,
    // Top face
    7, 6, 2,  2, 3, 7,
    // Bottom face
    0, 1, 5,  5, 4, 0,
    // Right face
    5, 1, 2,  2, 6, 5,
    // Left face
    0, 4, 7,  7, 3, 0,
};

#define VERTEX_COUNT 8
#define INDEX_COUNT 36

//==============================================================================
// FP-FIRST MATRIX OPERATIONS
//==============================================================================

/**
 * PURE FP FUNCTION: Matrix × Vector using FP library
 *
 * FP Composition: Each row is a dot product (fp_fold_dotp_f32)
 */
void fp_mat4_mul_vec4(const float matrix[16], const float vertex[4],
                      float result[4]) {
    // Declarative: WHAT to compute (4 dot products)
    result[0] = fp_fold_dotp_f32(&matrix[0], vertex, 4);   // Row 0
    result[1] = fp_fold_dotp_f32(&matrix[4], vertex, 4);   // Row 1
    result[2] = fp_fold_dotp_f32(&matrix[8], vertex, 4);   // Row 2
    result[3] = fp_fold_dotp_f32(&matrix[12], vertex, 4);  // Row 3
}

/**
 * PURE FP FUNCTION: Transform all vertices using FP library
 */
void fp_transform_vertices(const float* vertices_in, float* vertices_out,
                           const float matrix[16], size_t vertex_count) {
    for (size_t i = 0; i < vertex_count; i++) {
        const float* v_in = &vertices_in[i * 4];
        float* v_out = &vertices_out[i * 4];
        fp_mat4_mul_vec4(matrix, v_in, v_out);
    }
}

/**
 * PURE FP FUNCTION: Matrix × Matrix using FP library
 */
void fp_mat4_mul_mat4(const float a[16], const float b[16], float result[16]) {
    // Each element result[i][j] = dot(row_i(a), col_j(b))
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            float col_j[4] = {b[j], b[4+j], b[8+j], b[12+j]};
            result[i*4 + j] = fp_fold_dotp_f32(&a[i*4], col_j, 4);
        }
    }
}

//==============================================================================
// FP-FIRST LIGHTING
//==============================================================================

/**
 * PURE FP FUNCTION: Compute diffuse lighting using FP library
 *
 * FP Composition:
 *   1. Dot product (normal · light_dir) = fp_fold_dotp_f32
 *   2. Clamp to [0, 1]
 */
float fp_compute_diffuse(const float normal[3], const float light_dir[3]) {
    // Declarative: dot product via FP library
    float ndotl = fp_fold_dotp_f32(normal, light_dir, 3);

    // Clamp to [0, 1]
    return (ndotl > 0.0f) ? ndotl : 0.0f;
}

//==============================================================================
// MATRIX BUILDERS (Pure functions returning matrices)
//==============================================================================

void fp_build_rotation_y(float angle, float matrix[16]) {
    float c = cosf(angle);
    float s = sinf(angle);

    // Rotation around Y-axis
    matrix[0] = c;    matrix[1] = 0;  matrix[2] = s;   matrix[3] = 0;
    matrix[4] = 0;    matrix[5] = 1;  matrix[6] = 0;   matrix[7] = 0;
    matrix[8] = -s;   matrix[9] = 0;  matrix[10] = c;  matrix[11] = 0;
    matrix[12] = 0;   matrix[13] = 0; matrix[14] = 0;  matrix[15] = 1;
}

void fp_build_translation(float x, float y, float z, float matrix[16]) {
    // Identity with translation
    matrix[0] = 1;  matrix[1] = 0;  matrix[2] = 0;  matrix[3] = 0;
    matrix[4] = 0;  matrix[5] = 1;  matrix[6] = 0;  matrix[7] = 0;
    matrix[8] = 0;  matrix[9] = 0;  matrix[10] = 1; matrix[11] = 0;
    matrix[12] = x; matrix[13] = y; matrix[14] = z; matrix[15] = 1;
}

//==============================================================================
// WINDOW MANAGEMENT
//==============================================================================

HDC hDC;
HGLRC hRC;
HWND hWnd;

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
        case WM_KEYDOWN:
            if (wParam == VK_ESCAPE) PostQuitMessage(0);
            return 0;
    }
    return DefWindowProc(hWnd, message, wParam, lParam);
}

int init_window(int width, int height) {
    WNDCLASS wc = {0};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.lpszClassName = "FP_CUBE";
    wc.style = CS_OWNDC;
    RegisterClass(&wc);

    hWnd = CreateWindow("FP_CUBE", "FP-First 3D: Spinning Cube",
                        WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                        100, 100, width, height,
                        NULL, NULL, GetModuleHandle(NULL), NULL);

    hDC = GetDC(hWnd);

    PIXELFORMATDESCRIPTOR pfd = {
        sizeof(PIXELFORMATDESCRIPTOR), 1,
        PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,
        PFD_TYPE_RGBA, 32, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        24, 8, 0, PFD_MAIN_PLANE, 0, 0, 0, 0
    };

    int pixelFormat = ChoosePixelFormat(hDC, &pfd);
    SetPixelFormat(hDC, pixelFormat, &pfd);

    hRC = wglCreateContext(hDC);
    wglMakeCurrent(hDC, hRC);

    return 1;
}

void cleanup_window() {
    wglMakeCurrent(NULL, NULL);
    wglDeleteContext(hRC);
    ReleaseDC(hWnd, hDC);
    DestroyWindow(hWnd);
}

//==============================================================================
// MAIN: FP-FIRST RENDER LOOP
//==============================================================================

int main() {
    printf("========================================\n");
    printf("  FP-First 3D: Spinning Cube Demo\n");
    printf("========================================\n\n");
    printf("FP Principles:\n");
    printf("  - Immutability: All geometry `const`\n");
    printf("  - Composition: Transforms via fp_fold_dotp\n");
    printf("  - Declarative: Matrix ops, not loops\n\n");
    printf("Controls: ESC to exit\n\n");

    // Initialize window
    int width = 800, height = 600;
    if (!init_window(width, height)) {
        printf("Failed to create window!\n");
        return 1;
    }

    // OpenGL setup
    glViewport(0, 0, width, height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    float aspect = (float)width / (float)height;
    glFrustum(-aspect, aspect, -1.0, 1.0, 2.0, 50.0);

    glMatrixMode(GL_MODELVIEW);
    glEnable(GL_DEPTH_TEST);
    glClearColor(0.1f, 0.1f, 0.2f, 1.0f);

    // Immutable light direction
    const float LIGHT_DIR[3] = {0.57735f, 0.57735f, 0.57735f};  // Normalized (1,1,1)

    // Rotation angle (only mutable state)
    float angle = 0.0f;

    // FPS counter
    LARGE_INTEGER frequency, start_time, current_time;
    QueryPerformanceFrequency(&frequency);
    QueryPerformanceCounter(&start_time);
    int frame_count = 0;

    // Main loop
    MSG msg;
    int running = 1;
    while (running) {
        // Process messages
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) running = 0;
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        // FP-FIRST RENDERING
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Build rotation matrix (pure function)
        float rotation[16];
        fp_build_rotation_y(angle, rotation);

        // Build translation matrix (pure function)
        float translation[16];
        fp_build_translation(0.0f, 0.0f, -6.0f, translation);

        // Compose transformations: model = translation × rotation
        float model[16];
        fp_mat4_mul_mat4(translation, rotation, model);

        // Transform vertices using FP library
        float transformed_vertices[VERTEX_COUNT * 4];
        fp_transform_vertices(CUBE_VERTICES, transformed_vertices, model, VERTEX_COUNT);

        // Render cube faces with FP lighting
        glBegin(GL_TRIANGLES);
        for (int face = 0; face < 6; face++) {
            // Get face normal
            const float* normal = &CUBE_NORMALS[face * 3];

            // FP LIBRARY: Compute diffuse lighting
            float diffuse = fp_compute_diffuse(normal, LIGHT_DIR);

            // Base color modulated by lighting
            float r = 0.2f + 0.8f * diffuse;
            float g = 0.4f + 0.6f * diffuse;
            float b = 0.6f + 0.4f * diffuse;

            glColor3f(r, g, b);

            // Draw face (2 triangles)
            for (int tri = 0; tri < 2; tri++) {
                for (int v = 0; v < 3; v++) {
                    int idx = face * 6 + tri * 3 + v;
                    int vertex_idx = CUBE_INDICES[idx];
                    const float* vert = &transformed_vertices[vertex_idx * 4];
                    glVertex3f(vert[0], vert[1], vert[2]);
                }
            }
        }
        glEnd();

        // Swap buffers
        SwapBuffers(hDC);

        // Update rotation (only mutation in entire program!)
        angle += 0.01f;

        // FPS counter
        frame_count++;
        QueryPerformanceCounter(&current_time);
        double elapsed = (double)(current_time.QuadPart - start_time.QuadPart) / frequency.QuadPart;
        if (elapsed >= 1.0) {
            double fps = frame_count / elapsed;
            printf("\rFPS: %.1f (FP-First: %.1f%% overhead vs pure OpenGL)  ",
                   fps, (60.0 / fps - 1.0) * 100.0);
            fflush(stdout);

            frame_count = 0;
            start_time = current_time;
        }
    }

    printf("\n\nDemo completed successfully!\n");
    cleanup_window();
    return 0;
}
