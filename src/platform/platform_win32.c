#include "platform.h"
#include "renderer.h"
#include "application.h"
#include "fp_mesh_generation.h" // For fp_mesh_free
#include <windows.h>
#include <stdio.h>
#include <math.h>

// WGL extension constants
#define WGL_CONTEXT_MAJOR_VERSION_ARB 0x2091
#define WGL_CONTEXT_MINOR_VERSION_ARB 0x2092
#define WGL_CONTEXT_PROFILE_MASK_ARB 0x9126
#define WGL_CONTEXT_CORE_PROFILE_BIT_ARB 0x00000001

// Globals for platform layer
static UserInput g_user_input = {0};
static HWND hwnd;
static HDC hdc;
static HGLRC hglrc;

// Forward declarations for functions in this file
LRESULT CALLBACK WindowProc(HWND, UINT, WPARAM, LPARAM);
BOOL init_window_and_opengl(HINSTANCE);
void shutdown_opengl();

// Forward declarations for application logic (from demo_fp_opengl.c)
AppState update_app_state(AppState s, const UserInput* i);
AppState init_app_state();

int platform_run() {
    HINSTANCE hInstance = GetModuleHandle(NULL);
    AllocConsole();
    freopen("CONOUT$", "w", stdout);

    if (!init_window_and_opengl(hInstance)) return 1;

    AppState s = init_app_state();
    OpenGLResources res = renderer_init(&s);
    if (res.lighting_shader == 0) return 1;

    s = update_app_state(s, &g_user_input); // Initial update

    MSG msg;
    BOOL running = TRUE;
    while (running) {
        g_user_input.dx = 0; g_user_input.dy = 0; g_user_input.scroll_delta = 0;
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) running = FALSE;
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        s = update_app_state(s, &g_user_input);
        renderer_render(&s, &res, hdc);
    }

    renderer_shutdown(&res);
    fp_mesh_destroy(&s.cube_mesh);
    fp_mesh_destroy(&s.plane_mesh);
    shutdown_opengl();
    Sleep(5000); // Add a delay to allow error messages to be captured
    return 0;
}

BOOL init_window_and_opengl(HINSTANCE hInstance) {
    WNDCLASSA wc = {0};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = "FPEngineDemo";
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    if (!RegisterClassA(&wc)) return FALSE;

    hwnd = CreateWindowExA(0, "FPEngineDemo", "FP Engine Lighting Demo", WS_OVERLAPPEDWINDOW | WS_VISIBLE, 100, 100, WINDOW_WIDTH, WINDOW_HEIGHT, NULL, NULL, hInstance, NULL);
    if (!hwnd) return FALSE;
    hdc = GetDC(hwnd);

    PIXELFORMATDESCRIPTOR pfd_temp = {sizeof(pfd_temp), 1, PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER, PFD_TYPE_RGBA, 32};
    int pf_temp = ChoosePixelFormat(hdc, &pfd_temp);
    if (!pf_temp || !SetPixelFormat(hdc, pf_temp, &pfd_temp)) return FALSE;
    HGLRC temp_ctx = wglCreateContext(hdc);
    wglMakeCurrent(hdc, temp_ctx);
    if (!gl_load_extensions()) {
        wglDeleteContext(temp_ctx);
        return FALSE;
    }

    const int pixel_attribs[] = {
        WGL_DRAW_TO_WINDOW_ARB, GL_TRUE,
        WGL_SUPPORT_OPENGL_ARB, GL_TRUE,
        WGL_DOUBLE_BUFFER_ARB, GL_TRUE,
        WGL_PIXEL_TYPE_ARB, WGL_TYPE_RGBA_ARB,
        WGL_COLOR_BITS_ARB, 32,
        WGL_DEPTH_BITS_ARB, 24,
        WGL_STENCIL_BITS_ARB, 8,
        // WGL_SAMPLE_BUFFERS_ARB, 1, // Disabled MSAA for debugging
        // WGL_SAMPLES_ARB, 4,       // Disabled MSAA for debugging
        0
    };
    int pixel_format;
    UINT num_formats;
    if (!wglChoosePixelFormatARB(hdc, pixel_attribs, NULL, 1, &pixel_format, &num_formats) || num_formats == 0) {
        printf("wglChoosePixelFormatARB failed. MSAA not supported?\n");
        wglDeleteContext(temp_ctx);
        return FALSE;
    }

    PIXELFORMATDESCRIPTOR desired_pfd = {0};
    desired_pfd.nSize = sizeof(desired_pfd);
    desired_pfd.nVersion = 1;
    if (!DescribePixelFormat(hdc, pixel_format, sizeof(desired_pfd), &desired_pfd)) {
        printf("DescribePixelFormat failed.\n");
        wglMakeCurrent(NULL, NULL);
        wglDeleteContext(temp_ctx);
        ReleaseDC(hwnd, hdc);
        return FALSE;
    }

    wglMakeCurrent(NULL, NULL);
    wglDeleteContext(temp_ctx);
    ReleaseDC(hwnd, hdc);
    DestroyWindow(hwnd);
    hwnd = CreateWindowExA(0, "FPEngineDemo", "FP Engine Lighting Demo", WS_OVERLAPPEDWINDOW | WS_VISIBLE, 100, 100, WINDOW_WIDTH, WINDOW_HEIGHT, NULL, NULL, hInstance, NULL);
    if (!hwnd) return FALSE;
    hdc = GetDC(hwnd);
    if (!SetPixelFormat(hdc, pixel_format, &desired_pfd)) {
        printf("SetPixelFormat failed for final window.\n");
        return FALSE;
    }

    const int attribs_core[] = {WGL_CONTEXT_MAJOR_VERSION_ARB, 3, WGL_CONTEXT_MINOR_VERSION_ARB, 3, WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_CORE_PROFILE_BIT_ARB, 0};
    hglrc = wglCreateContextAttribsARB(hdc, 0, attribs_core);
    if (!hglrc) {
        printf("wglCreateContextAttribsARB failed.\n");
        return FALSE;
    }

    wglMakeCurrent(hdc, hglrc);

    return TRUE;
}

void shutdown_opengl() {
    wglMakeCurrent(NULL, NULL);
    wglDeleteContext(hglrc);
    ReleaseDC(hwnd, hdc);
}

LRESULT CALLBACK WindowProc(HWND hwnd_proc, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    static int last_x = 0, last_y = 0;
    switch (uMsg) {
        case WM_DESTROY:
            if (hwnd_proc == hwnd) { PostQuitMessage(0); }
            return 0;
        case WM_KEYDOWN: if (wParam == VK_ESCAPE) PostQuitMessage(0); return 0;
        case WM_LBUTTONDOWN: g_user_input.left_mouse_down = TRUE; last_x = LOWORD(lParam); last_y = HIWORD(lParam); SetCapture(hwnd_proc); return 0;
        case WM_LBUTTONUP: g_user_input.left_mouse_down = FALSE; ReleaseCapture(); return 0;
        case WM_RBUTTONDOWN: g_user_input.right_mouse_down = TRUE; last_x = LOWORD(lParam); last_y = HIWORD(lParam); SetCapture(hwnd_proc); return 0;
        case WM_RBUTTONUP: g_user_input.right_mouse_down = FALSE; ReleaseCapture(); return 0;
        case WM_MOUSEMOVE:
            if (g_user_input.left_mouse_down || g_user_input.right_mouse_down) {
                int x = LOWORD(lParam), y = HIWORD(lParam);
                g_user_input.dx = x - last_x;
                g_user_input.dy = y - last_y;
                last_x = x;
                last_y = y;
            }
            return 0;
        case WM_MOUSEWHEEL: g_user_input.scroll_delta = GET_WHEEL_DELTA_WPARAM(wParam); return 0;
    }
    return DefWindowProc(hwnd_proc, uMsg, wParam, lParam);
}
