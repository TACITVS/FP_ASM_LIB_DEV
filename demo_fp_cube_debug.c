/**
 * demo_fp_cube_debug.c
 *
 * Find EXACTLY which line crashes
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
    printf("FP Cube Debug - Finding crash point\n\n");

    // Create window
    WNDCLASS wc = {0};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.lpszClassName = "FPCUBE";
    wc.style = CS_OWNDC;
    RegisterClass(&wc);

    hWnd = CreateWindow("FPCUBE", "FP Cube Debug", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                        100, 100, 800, 600, NULL, NULL, GetModuleHandle(NULL), NULL);

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
    printf("OpenGL initialized\n");

    const float vertices[] = {
        -1,-1,-1,  1,-1,-1,  1,1,-1,  -1,1,-1,
        -1,-1, 1,  1,-1, 1,  1,1, 1,  -1,1, 1
    };

    float angle = 0;
    MSG msg;

    printf("\nEntering loop...\n");
    fflush(stdout);

    int iteration = 0;
    while (iteration < 10) {  // Just 10 iterations for testing
        printf("Iteration %d:\n", iteration);
        fflush(stdout);

        printf("  PeekMessage...\n");
        fflush(stdout);

        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) break;
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        printf("  glClear...\n");
        fflush(stdout);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        printf("  glEnable(GL_DEPTH_TEST)...\n");
        fflush(stdout);
        glEnable(GL_DEPTH_TEST);

        printf("  cosf/sinf...\n");
        fflush(stdout);
        float c = cosf(angle);
        float s = sinf(angle);
        printf("    c=%.3f, s=%.3f\n", c, s);
        fflush(stdout);

        printf("  glLoadIdentity...\n");
        fflush(stdout);
        glLoadIdentity();

        printf("  glTranslatef...\n");
        fflush(stdout);
        glTranslatef(0, 0, -5);

        printf("  glBegin(GL_QUADS)...\n");
        fflush(stdout);
        glBegin(GL_QUADS);

        printf("  Drawing front face...\n");
        fflush(stdout);
        glColor3f(0, 1, 1);

        printf("    Vertex loop...\n");
        fflush(stdout);

        for (int i = 4; i < 8; i++) {
            printf("      i=%d: accessing vertices[%d], [%d], [%d]\n",
                   i, i*3, i*3+1, i*3+2);
            fflush(stdout);

            float x = vertices[i*3] * c - vertices[i*3+2] * s;
            float y = vertices[i*3+1];
            float z = vertices[i*3] * s + vertices[i*3+2] * c;

            printf("      Computed: x=%.2f, y=%.2f, z=%.2f\n", x, y, z);
            fflush(stdout);

            printf("      Calling glVertex3f...\n");
            fflush(stdout);
            glVertex3f(x, y, z);
            printf("      glVertex3f OK\n");
            fflush(stdout);
        }

        printf("  glEnd...\n");
        fflush(stdout);
        glEnd();

        printf("  SwapBuffers...\n");
        fflush(stdout);
        SwapBuffers(hDC);

        printf("  Updating angle...\n");
        fflush(stdout);
        angle += 0.02f;

        printf("Iteration %d complete!\n\n", iteration);
        fflush(stdout);

        iteration++;
        Sleep(100);  // Slow it down so we can see output
    }

    printf("\nTest completed successfully!\n");
    printf("Cleaning up...\n");

    wglMakeCurrent(NULL, NULL);
    wglDeleteContext(hRC);
    ReleaseDC(hWnd, hDC);
    DestroyWindow(hWnd);

    printf("Done!\n");
    return 0;
}
