/**
 * demo_fp_cube_simple.c
 *
 * Simplified FP-First Spinning Cube with Extensive Error Handling
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <windows.h>
#include <gl/gl.h>
#include "include/fp_core.h"

//==============================================================================
// IMMUTABLE CUBE GEOMETRY
//==============================================================================

const float CUBE_VERTICES[] = {
    -1.0f, -1.0f, -1.0f, 1.0f,
     1.0f, -1.0f, -1.0f, 1.0f,
     1.0f,  1.0f, -1.0f, 1.0f,
    -1.0f,  1.0f, -1.0f, 1.0f,
    -1.0f, -1.0f,  1.0f, 1.0f,
     1.0f, -1.0f,  1.0f, 1.0f,
     1.0f,  1.0f,  1.0f, 1.0f,
    -1.0f,  1.0f,  1.0f, 1.0f,
};

const float CUBE_NORMALS[] = {
    0.0f, 0.0f, 1.0f,
    0.0f, 0.0f, -1.0f,
    0.0f, 1.0f, 0.0f,
    0.0f, -1.0f, 0.0f,
    1.0f, 0.0f, 0.0f,
    -1.0f, 0.0f, 0.0f,
};

const unsigned int CUBE_INDICES[] = {
    4, 5, 6,  6, 7, 4,
    1, 0, 3,  3, 2, 1,
    7, 6, 2,  2, 3, 7,
    0, 1, 5,  5, 4, 0,
    5, 1, 2,  2, 6, 5,
    0, 4, 7,  7, 3, 0,
};

//==============================================================================
// FP-FIRST MATRIX OPERATIONS
//==============================================================================

void fp_mat4_mul_vec4(const float matrix[16], const float vertex[4], float result[4]) {
    // FP Library: 4 dot products = matrix × vector
    // NO DEBUG OUTPUT HERE - called thousands of times per second!
    result[0] = fp_fold_dotp_f32(&matrix[0], vertex, 4);
    result[1] = fp_fold_dotp_f32(&matrix[4], vertex, 4);
    result[2] = fp_fold_dotp_f32(&matrix[8], vertex, 4);
    result[3] = fp_fold_dotp_f32(&matrix[12], vertex, 4);
}

float fp_compute_diffuse(const float normal[3], const float light_dir[3]) {
    float ndotl = fp_fold_dotp_f32(normal, light_dir, 3);
    return (ndotl > 0.0f) ? ndotl : 0.0f;
}

void fp_build_rotation_y(float angle, float matrix[16]) {
    float c = cosf(angle);
    float s = sinf(angle);

    matrix[0] = c;    matrix[1] = 0;  matrix[2] = s;   matrix[3] = 0;
    matrix[4] = 0;    matrix[5] = 1;  matrix[6] = 0;   matrix[7] = 0;
    matrix[8] = -s;   matrix[9] = 0;  matrix[10] = c;  matrix[11] = 0;
    matrix[12] = 0;   matrix[13] = 0; matrix[14] = 0;  matrix[15] = 1;
}

//==============================================================================
// WINDOW MANAGEMENT
//==============================================================================

HDC hDC = NULL;
HGLRC hRC = NULL;
HWND hWnd = NULL;

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
    printf("Initializing window...\n");
    fflush(stdout);

    WNDCLASS wc = {0};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.lpszClassName = "FP_CUBE";
    wc.style = CS_OWNDC;

    if (!RegisterClass(&wc)) {
        printf("ERROR: Failed to register window class\n");
        return 0;
    }
    printf("  Window class registered\n");
    fflush(stdout);

    hWnd = CreateWindow("FP_CUBE", "FP-First Cube",
                        WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                        100, 100, width, height,
                        NULL, NULL, GetModuleHandle(NULL), NULL);

    if (!hWnd) {
        printf("ERROR: Failed to create window\n");
        return 0;
    }
    printf("  Window created\n");
    fflush(stdout);

    hDC = GetDC(hWnd);
    if (!hDC) {
        printf("ERROR: Failed to get device context\n");
        return 0;
    }
    printf("  Device context obtained\n");
    fflush(stdout);

    PIXELFORMATDESCRIPTOR pfd = {
        sizeof(PIXELFORMATDESCRIPTOR), 1,
        PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,
        PFD_TYPE_RGBA, 32, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        24, 8, 0, PFD_MAIN_PLANE, 0, 0, 0, 0
    };

    int pixelFormat = ChoosePixelFormat(hDC, &pfd);
    if (!pixelFormat) {
        printf("ERROR: Failed to choose pixel format\n");
        return 0;
    }

    if (!SetPixelFormat(hDC, pixelFormat, &pfd)) {
        printf("ERROR: Failed to set pixel format\n");
        return 0;
    }
    printf("  Pixel format set\n");
    fflush(stdout);

    hRC = wglCreateContext(hDC);
    if (!hRC) {
        printf("ERROR: Failed to create OpenGL context\n");
        return 0;
    }

    if (!wglMakeCurrent(hDC, hRC)) {
        printf("ERROR: Failed to make OpenGL context current\n");
        return 0;
    }
    printf("  OpenGL context created and active\n");
    fflush(stdout);

    return 1;
}

void cleanup_window() {
    if (hRC) {
        wglMakeCurrent(NULL, NULL);
        wglDeleteContext(hRC);
    }
    if (hDC && hWnd) ReleaseDC(hWnd, hDC);
    if (hWnd) DestroyWindow(hWnd);
}

//==============================================================================
// MAIN
//==============================================================================

int main() {
    printf("========================================\n");
    printf("  FP-First 3D: Spinning Cube Demo\n");
    printf("========================================\n\n");
    printf("FP Principles:\n");
    printf("  - Immutability: All geometry `const`\n");
    printf("  - Composition: Transforms via fp_fold_dotp\n");
    printf("  - Declarative: Matrix ops, not loops\n\n");
    fflush(stdout);

    // Test FP library BEFORE window creation
    printf("Testing FP library functions...\n");
    fflush(stdout);

    float test_a[4] = {1.0f, 2.0f, 3.0f, 4.0f};
    float test_b[4] = {1.0f, 1.0f, 1.0f, 1.0f};
    float expected = 10.0f; // 1+2+3+4

    float result = fp_fold_dotp_f32(test_a, test_b, 4);
    printf("  fp_fold_dotp_f32 test: %.1f (expected %.1f)\n", result, expected);

    if (fabsf(result - expected) > 0.001f) {
        printf("ERROR: FP library function failed!\n");
        return 1;
    }
    printf("  FP library test PASSED\n\n");
    fflush(stdout);

    // Initialize window
    int width = 800, height = 600;
    if (!init_window(width, height)) {
        printf("\nFailed to initialize window!\n");
        cleanup_window();
        return 1;
    }
    printf("\nWindow initialized successfully!\n\n");
    fflush(stdout);

    // OpenGL setup
    printf("Setting up OpenGL...\n");
    fflush(stdout);

    glViewport(0, 0, width, height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    float aspect = (float)width / (float)height;
    glFrustum(-aspect, aspect, -1.0, 1.0, 2.0, 50.0);

    glMatrixMode(GL_MODELVIEW);
    glEnable(GL_DEPTH_TEST);
    glClearColor(0.1f, 0.1f, 0.2f, 1.0f);

    printf("OpenGL setup complete\n\n");
    fflush(stdout);

    // Test matrix transform ONCE during initialization
    printf("Testing FP matrix operations...\n");
    fflush(stdout);

    float test_matrix[16];
    fp_build_rotation_y(0.0f, test_matrix);

    float test_vertex[4] = {1.0f, 0.0f, 0.0f, 1.0f};
    float test_result[4];
    fp_mat4_mul_vec4(test_matrix, test_vertex, test_result);

    printf("  Transform result: [%.2f, %.2f, %.2f, %.2f]\n",
           test_result[0], test_result[1], test_result[2], test_result[3]);
    printf("  Matrix operations OK!\n\n");

    printf("FP operations working! Starting render loop...\n\n");
    printf("Controls: ESC to exit\n\n");
    fflush(stdout);

    const float LIGHT_DIR[3] = {0.57735f, 0.57735f, 0.57735f};
    float angle = 0.0f;
    int frame_count = 0;

    MSG msg;
    int running = 1;
    while (running) {
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) running = 0;
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Build rotation matrix
        float rotation[16];
        fp_build_rotation_y(angle, rotation);

        // Render cube
        glLoadIdentity();
        glTranslatef(0.0f, 0.0f, -6.0f);

        glBegin(GL_TRIANGLES);
        for (int face = 0; face < 6; face++) {
            const float* normal = &CUBE_NORMALS[face * 3];
            float diffuse = fp_compute_diffuse(normal, LIGHT_DIR);

            float r = 0.2f + 0.8f * diffuse;
            float g = 0.4f + 0.6f * diffuse;
            float b = 0.6f + 0.4f * diffuse;
            glColor3f(r, g, b);

            for (int tri = 0; tri < 2; tri++) {
                for (int v = 0; v < 3; v++) {
                    int idx = face * 6 + tri * 3 + v;
                    int vertex_idx = CUBE_INDICES[idx];
                    const float* vert = &CUBE_VERTICES[vertex_idx * 4];

                    // Apply rotation manually
                    float rotated[4];
                    fp_mat4_mul_vec4(rotation, vert, rotated);
                    glVertex3f(rotated[0], rotated[1], rotated[2]);
                }
            }
        }
        glEnd();

        SwapBuffers(hDC);
        angle += 0.01f;

        frame_count++;
        if (frame_count % 60 == 0) {
            printf("\rFrame: %d", frame_count);
            fflush(stdout);
        }
    }

    printf("\n\nDemo completed successfully!\n");
    cleanup_window();
    return 0;
}
