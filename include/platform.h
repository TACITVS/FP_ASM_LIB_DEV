#ifndef PLATFORM_H
#define PLATFORM_H

#include <windows.h> // For BOOL

#define WINDOW_WIDTH 1280
#define WINDOW_HEIGHT 720

typedef struct {
    int dx, dy;
    int scroll_delta;
    BOOL left_mouse_down;
    BOOL right_mouse_down;
} UserInput;

// The platform layer is responsible for running the main loop
// and feeding input to the application.
int platform_run();

#endif // PLATFORM_H
