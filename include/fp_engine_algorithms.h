#ifndef FP_ENGINE_ALGORITHMS_H
#define FP_ENGINE_ALGORITHMS_H

#include "fp_engine_types.h"
#include "fp_core.h" // For math functions

// Function to update the camera based on controls
// Takes a const AppState* and camera control parameters, returns a new AppState
FP_AppState fp_update_camera(const FP_AppState* current_state,
                             float camera_yaw_rad, float camera_pitch_rad,
                             float camera_distance, Vec3f camera_target);

// Function to update light matrices (e.g., shadow matrices)
// Takes a const AppState*, returns a new AppState
FP_AppState fp_update_light_matrices(const FP_AppState* current_state);

// Function to animate scene objects
// Takes a const AppState* and delta time, returns a new AppState
FP_AppState fp_animate_objects(const FP_AppState* current_state, float dt);

// Function to update the local and world matrices of a transform
void fp_transform_update_matrix(FP_Transform* transform);

#endif // FP_ENGINE_ALGORITHMS_H
