#ifndef FP_SHADOW_MAPPING_H
#define FP_SHADOW_MAPPING_H

#include "gl_extensions.h"

#define SHADOW_WIDTH 1024
#define SHADOW_HEIGHT 1024

typedef struct {
    GLuint depth_map_fbo;
    GLuint depth_map_texture;
} FP_ShadowMap;

FP_ShadowMap fp_shadow_create_map();
void fp_shadow_destroy_map(FP_ShadowMap* shadow_map);

#endif // FP_SHADOW_MAPPING_H
