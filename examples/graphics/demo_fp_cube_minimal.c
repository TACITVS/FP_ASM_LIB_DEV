/**
 * demo_fp_cube_minimal.c
 *
 * Ultra-minimal FP cube - no fancy OpenGL, just the basics
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <windows.h>
#include <gl/gl.h>
#include "include/fp_core.h"

HDC hDC = NULL;
HGLRC hRC = NULL;
HWND hWnd = NULL;

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (msg == WM_DESTROY || (msg == WM_KEYDOWN && wParam == VK_ESCAPE)) {
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hWnd, msg, wParam, lParam);
}

int main() {
    printf("FP-First Minimal Cube Demo\n\n");

    // Create window
    WNDCLASS wc = {0};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.lpszClassName = "FPCUBE";
    wc.style = CS_OWNDC;
    RegisterClass(&wc);

    hWnd = CreateWindow("FPCUBE", "FP Cube", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                        100, 100, 800, 600, NULL, NULL, GetModuleHandle(NULL), NULL);
    if (!hWnd) {
        printf("ERROR: CreateWindow failed\n");
        return 1;
    }
    printf("Window created\n");

    hDC = GetDC(hWnd);
    PIXELFORMATDESCRIPTOR pfd = {
        sizeof(PIXELFORMATDESCRIPTOR), 1,
        PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,
        PFD_TYPE_RGBA, 32, 0,0,0,0,0,0,0,0,0,0,0,0,0, 24, 8, 0,
        PFD_MAIN_PLANE, 0, 0, 0, 0
    };
    int pf = ChoosePixelFormat(hDC, &pfd);
    SetPixelFormat(hDC, pf, &pfd);
    hRC = wglCreateContext(hDC);
    wglMakeCurrent(hDC, hRC);
    printf("OpenGL context created\n");

    // Test OpenGL is working
    printf("Testing glClear...\n");
    glClearColor(0.1f, 0.1f, 0.2f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    SwapBuffers(hDC);
    printf("glClear works!\n");

    printf("Testing glBegin/glEnd...\n");
    glBegin(GL_TRIANGLES);
    glColor3f(1, 0, 0);
    glVertex3f(0, 0.5f, -2);
    glVertex3f(-0.5f, -0.5f, -2);
    glVertex3f(0.5f, -0.5f, -2);
    glEnd();
    SwapBuffers(hDC);
    printf("Basic rendering works!\n");

    Sleep(1000); // Show for 1 second

    // Test FP library
    printf("\nTesting FP library...\n");
    float a[4] = {1, 2, 3, 4};
    float b[4] = {1, 1, 1, 1};
    float result = fp_fold_dotp_f32(a, b, 4);
    printf("Dot product: %.1f (expected 10.0)\n", result);

    if (fabsf(result - 10.0f) > 0.001f) {
        printf("ERROR: FP library failed!\n");
        return 1;
    }
    printf("FP library works!\n");

    // Simple cube data
    const float vertices[] = {
        -1,-1,-1,  1,-1,-1,  1,1,-1,  -1,1,-1, // Back
        -1,-1, 1,  1,-1, 1,  1,1, 1,  -1,1, 1  // Front
    };

    printf("\nStarting render loop (ESC to exit)...\n");
    float angle = 0;
    MSG msg;
    int frame = 0;

    while (1) {
        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) break;
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST);

        // Simple rotation using FP library
        float c = cosf(angle);
        float s = sinf(angle);

        glLoadIdentity();
        glTranslatef(0, 0, -5);

        // Draw cube faces with colors
        glBegin(GL_QUADS);

        // Front (cyan)
        glColor3f(0, 1, 1);
        for (int i = 4; i < 8; i++) {
            glVertex3f(vertices[i*3] * c - vertices[i*3+2] * s,
                      vertices[i*3+1],
                      vertices[i*3] * s + vertices[i*3+2] * c);
        }

        // Back (magenta)
        glColor3f(1, 0, 1);
        for (int i = 0; i < 4; i++) {
            glVertex3f(vertices[i*3] * c - vertices[i*3+2] * s,
                      vertices[i*3+1],
                      vertices[i*3] * s + vertices[i*3+2] * c);
        }

        glEnd();

        SwapBuffers(hDC);
        angle += 0.02f;

        frame++;
        if (frame % 60 == 0) {
            printf("\rFrame: %d  ", frame);
            fflush(stdout);
        }
    }

    printf("\n\nDemo finished!\n");
    wglMakeCurrent(NULL, NULL);
    wglDeleteContext(hRC);
    ReleaseDC(hWnd, hDC);
    DestroyWindow(hWnd);
    return 0;
}
