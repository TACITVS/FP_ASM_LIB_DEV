/**
 * fp_lighting.c - Lighting System for 3D Rendering
 *
 * Implements Phong and Blinn-Phong shading models
 * Supports directional lights, point lights, and ambient lighting
 */

#include "fp_core.h"
#include <math.h>

/* ========== LIGHT TYPES ========== */

typedef struct {
    Vec3f direction;  // Light direction (should be normalized)
    Vec3f color;      // RGB color (0-1 range)
    float intensity;  // Light intensity multiplier
} DirectionalLight;

typedef struct {
    Vec3f position;       // Light position in world space
    Vec3f color;          // RGB color (0-1 range)
    float intensity;      // Light intensity
    float constant_atten; // Constant attenuation
    float linear_atten;   // Linear attenuation
    float quadratic_atten; // Quadratic attenuation
} PointLight;

typedef struct {
    Vec3f color;      // Ambient color (0-1 range)
    float intensity;  // Ambient intensity
} AmbientLight;

/* ========== MATERIAL PROPERTIES ========== */

typedef struct {
    Vec3f ambient;    // Ambient reflectance
    Vec3f diffuse;    // Diffuse reflectance
    Vec3f specular;   // Specular reflectance
    float shininess;  // Specular exponent (1-128)
} Material;

/* ========== SHADING FUNCTIONS ========== */

void lighting_compute_directional(Vec3f* out_color,
                                  const DirectionalLight* light,
                                  const Material* material,
                                  const Vec3f* normal,
                                  const Vec3f* view_dir) {
    // Diffuse component: max(N · L, 0)
    Vec3f light_dir_neg;
    vec3_scale(&light_dir_neg, &light->direction, -1.0f);  // Negate because light.direction points away
    float diffuse_factor = vec3_compute_diffuse(&light_dir_neg, normal);

    // Specular component: Blinn-Phong
    float specular_factor = vec3_compute_specular(&light_dir_neg, view_dir, normal, material->shininess);

    // Combine components
    out_color->x = light->color.x * light->intensity *
                   (material->diffuse.x * diffuse_factor + material->specular.x * specular_factor);
    out_color->y = light->color.y * light->intensity *
                   (material->diffuse.y * diffuse_factor + material->specular.y * specular_factor);
    out_color->z = light->color.z * light->intensity *
                   (material->diffuse.z * diffuse_factor + material->specular.z * specular_factor);
    out_color->_pad = 0.0f;
}

void lighting_compute_point(Vec3f* out_color,
                            const PointLight* light,
                            const Material* material,
                            const Vec3f* position,
                            const Vec3f* normal,
                            const Vec3f* view_dir) {
    // Calculate light direction (from surface to light)
    Vec3f light_dir;
    vec3_sub(&light_dir, &light->position, position);
    float distance = vec3_length(&light_dir);
    vec3_normalize(&light_dir, &light_dir);

    // Calculate attenuation
    float attenuation = 1.0f / (light->constant_atten +
                                light->linear_atten * distance +
                                light->quadratic_atten * distance * distance);

    // Diffuse component
    float diffuse_factor = vec3_compute_diffuse(&light_dir, normal);

    // Specular component
    float specular_factor = vec3_compute_specular(&light_dir, view_dir, normal, material->shininess);

    // Combine with attenuation
    float effective_intensity = light->intensity * attenuation;
    out_color->x = light->color.x * effective_intensity *
                   (material->diffuse.x * diffuse_factor + material->specular.x * specular_factor);
    out_color->y = light->color.y * effective_intensity *
                   (material->diffuse.y * diffuse_factor + material->specular.y * specular_factor);
    out_color->z = light->color.z * effective_intensity *
                   (material->diffuse.z * diffuse_factor + material->specular.z * specular_factor);
    out_color->_pad = 0.0f;
}

void lighting_compute_ambient(Vec3f* out_color,
                              const AmbientLight* ambient,
                              const Material* material) {
    out_color->x = ambient->color.x * ambient->intensity * material->ambient.x;
    out_color->y = ambient->color.y * ambient->intensity * material->ambient.y;
    out_color->z = ambient->color.z * ambient->intensity * material->ambient.z;
    out_color->_pad = 0.0f;
}

/* ========== COMBINED LIGHTING (Full Phong Model) ========== */

void lighting_compute_phong(Vec3f* out_color,
                            const AmbientLight* ambient,
                            const DirectionalLight* dir_light,
                            const PointLight* point_lights,
                            int num_point_lights,
                            const Material* material,
                            const Vec3f* position,
                            const Vec3f* normal,
                            const Vec3f* view_dir) {
    // Start with ambient
    Vec3f ambient_color;
    lighting_compute_ambient(&ambient_color, ambient, material);

    out_color->x = ambient_color.x;
    out_color->y = ambient_color.y;
    out_color->z = ambient_color.z;

    // Add directional light
    if (dir_light) {
        Vec3f dir_color;
        lighting_compute_directional(&dir_color, dir_light, material, normal, view_dir);
        out_color->x += dir_color.x;
        out_color->y += dir_color.y;
        out_color->z += dir_color.z;
    }

    // Add point lights
    for (int i = 0; i < num_point_lights; i++) {
        Vec3f point_color;
        lighting_compute_point(&point_color, &point_lights[i], material, position, normal, view_dir);
        out_color->x += point_color.x;
        out_color->y += point_color.y;
        out_color->z += point_color.z;
    }

    // Clamp to [0, 1]
    if (out_color->x > 1.0f) out_color->x = 1.0f;
    if (out_color->y > 1.0f) out_color->y = 1.0f;
    if (out_color->z > 1.0f) out_color->z = 1.0f;

    out_color->_pad = 0.0f;
}

/* ========== BATCHED LIGHTING (for performance) ========== */

void lighting_shade_vertices_batch(Vec3f* out_colors,
                                   const Vec3f* positions,
                                   const Vec3f* normals,
                                   int count,
                                   const AmbientLight* ambient,
                                   const DirectionalLight* dir_light,
                                   const Vec3f* camera_pos) {
    for (int i = 0; i < count; i++) {
        // Calculate view direction
        Vec3f view_dir;
        vec3_sub(&view_dir, camera_pos, &positions[i]);
        vec3_normalize(&view_dir, &view_dir);

        // Compute lighting (simplified - just ambient + directional)
        Vec3f ambient_color, dir_color;

        // Default material for now
        Material default_mat = {
            {0.1f, 0.1f, 0.1f, 0.0f},  // ambient
            {0.8f, 0.8f, 0.8f, 0.0f},  // diffuse
            {0.5f, 0.5f, 0.5f, 0.0f},  // specular
            32.0f                       // shininess
        };

        lighting_compute_ambient(&ambient_color, ambient, &default_mat);

        if (dir_light) {
            lighting_compute_directional(&dir_color, dir_light, &default_mat, &normals[i], &view_dir);
            out_colors[i].x = ambient_color.x + dir_color.x;
            out_colors[i].y = ambient_color.y + dir_color.y;
            out_colors[i].z = ambient_color.z + dir_color.z;
        } else {
            out_colors[i] = ambient_color;
        }

        // Clamp
        if (out_colors[i].x > 1.0f) out_colors[i].x = 1.0f;
        if (out_colors[i].y > 1.0f) out_colors[i].y = 1.0f;
        if (out_colors[i].z > 1.0f) out_colors[i].z = 1.0f;
        out_colors[i]._pad = 0.0f;
    }
}
