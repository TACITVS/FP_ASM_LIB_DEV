/**
 * ray_tracer.cl
 *
 * OpenCL Ray Tracing Kernel
 * Each GPU thread renders exactly ONE pixel
 *
 * Target: GT 730M (384 CUDA cores, Kepler architecture)
 * Expected: 30-100 FPS @ 1920x1080
 */

// ============================================================================
// Data Structures (must match C host code)
// ============================================================================

typedef struct {
    float x, y, z;
    float _pad;
} Vec3;

typedef struct {
    Vec3 origin;
    Vec3 direction;
} Ray;

typedef struct {
    Vec3 center;
    float radius;
    Vec3 color;
    float specular;
    float reflective;
} Sphere;

typedef struct {
    Vec3 normal;
    float distance;
    Vec3 color;
    float specular;
} Plane;

typedef struct {
    Vec3 position;
    Vec3 color;
    float intensity;
} Light;

typedef struct {
    int hit;
    float t;
    Vec3 point;
    Vec3 normal;
    Vec3 color;
    float specular;
    float reflective;
} Hit;

typedef struct {
    Vec3 position;
    Vec3 look_at;
    Vec3 up;
    float fov;
    float aspect;
} Camera;

// ============================================================================
// Vector Math (GPU implementations)
// ============================================================================

inline Vec3 vec3_make(float x, float y, float z) {
    Vec3 v;
    v.x = x;
    v.y = y;
    v.z = z;
    v._pad = 0.0f;
    return v;
}

inline Vec3 vec3_add(Vec3 a, Vec3 b) {
    return vec3_make(a.x + b.x, a.y + b.y, a.z + b.z);
}

inline Vec3 vec3_sub(Vec3 a, Vec3 b) {
    return vec3_make(a.x - b.x, a.y - b.y, a.z - b.z);
}

inline Vec3 vec3_scale(Vec3 v, float s) {
    return vec3_make(v.x * s, v.y * s, v.z * s);
}

inline float vec3_dot(Vec3 a, Vec3 b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

inline Vec3 vec3_cross(Vec3 a, Vec3 b) {
    return vec3_make(
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x
    );
}

inline float vec3_length(Vec3 v) {
    return sqrt(vec3_dot(v, v));
}

inline Vec3 vec3_normalize(Vec3 v) {
    float len = vec3_length(v);
    if (len < 0.001f) {
        return vec3_make(0, 0, 0);
    }
    return vec3_scale(v, 1.0f / len);
}

inline Vec3 vec3_multiply(Vec3 a, Vec3 b) {
    return vec3_make(a.x * b.x, a.y * b.y, a.z * b.z);
}

inline Vec3 vec3_clamp(Vec3 v) {
    return vec3_make(
        fmax(0.0f, fmin(1.0f, v.x)),
        fmax(0.0f, fmin(1.0f, v.y)),
        fmax(0.0f, fmin(1.0f, v.z))
    );
}

// ============================================================================
// Ray Intersection (GPU)
// ============================================================================

inline int ray_sphere_intersect(__global const Sphere* sphere, const Ray* ray, Hit* hit) {
    Vec3 oc = vec3_sub(ray->origin, sphere->center);

    float a = vec3_dot(ray->direction, ray->direction);
    float b = 2.0f * vec3_dot(oc, ray->direction);
    float c = vec3_dot(oc, oc) - sphere->radius * sphere->radius;

    float discriminant = b * b - 4.0f * a * c;

    if (discriminant < 0) {
        return 0;  // No hit
    }

    float sqrt_disc = sqrt(discriminant);
    float t = (-b - sqrt_disc) / (2.0f * a);

    if (t < 0.001f) {
        t = (-b + sqrt_disc) / (2.0f * a);
        if (t < 0.001f) {
            return 0;  // Behind ray
        }
    }

    hit->hit = 1;
    hit->t = t;
    hit->point = vec3_add(ray->origin, vec3_scale(ray->direction, t));
    hit->normal = vec3_normalize(vec3_sub(hit->point, sphere->center));
    hit->color = sphere->color;
    hit->specular = sphere->specular;
    hit->reflective = sphere->reflective;

    return 1;
}

inline int ray_plane_intersect(__global const Plane* plane, const Ray* ray, Hit* hit) {
    float denom = vec3_dot(plane->normal, ray->direction);

    if (fabs(denom) < 0.001f) {
        return 0;  // Parallel
    }

    float t = -(vec3_dot(plane->normal, ray->origin) + plane->distance) / denom;

    if (t < 0.001f) {
        return 0;  // Behind ray
    }

    hit->hit = 1;
    hit->t = t;
    hit->point = vec3_add(ray->origin, vec3_scale(ray->direction, t));
    hit->normal = plane->normal;
    hit->color = plane->color;
    hit->specular = plane->specular;
    hit->reflective = 0.0f;

    return 1;
}

inline int ray_scene_intersect(
    const Ray* ray,
    __global const Sphere* spheres,
    int n_spheres,
    __global const Plane* planes,
    int n_planes,
    Hit* hit
) {
    int any_hit = 0;
    float closest_t = 1e30f;
    Hit temp_hit;

    // Test spheres
    for (int i = 0; i < n_spheres; i++) {
        if (ray_sphere_intersect(&spheres[i], ray, &temp_hit)) {
            if (temp_hit.t < closest_t) {
                closest_t = temp_hit.t;
                *hit = temp_hit;
                any_hit = 1;
            }
        }
    }

    // Test planes
    for (int i = 0; i < n_planes; i++) {
        if (ray_plane_intersect(&planes[i], ray, &temp_hit)) {
            if (temp_hit.t < closest_t) {
                closest_t = temp_hit.t;
                *hit = temp_hit;
                any_hit = 1;
            }
        }
    }

    hit->hit = any_hit;
    return any_hit;
}

// ============================================================================
// Shading (GPU)
// ============================================================================

inline Vec3 phong_shading(
    const Hit* hit,
    __global const Light* lights,
    int n_lights,
    Vec3 ambient,
    Vec3 view_dir
) {
    Vec3 color = ambient;

    for (int i = 0; i < n_lights; i++) {
        __global const Light* light = &lights[i];

        Vec3 light_dir = vec3_sub(light->position, hit->point);
        light_dir = vec3_normalize(light_dir);

        // Diffuse
        float n_dot_l = vec3_dot(hit->normal, light_dir);
        if (n_dot_l > 0) {
            Vec3 diffuse = vec3_multiply(hit->color, light->color);
            diffuse = vec3_scale(diffuse, light->intensity * n_dot_l);
            color = vec3_add(color, diffuse);
        }

        // Specular
        if (hit->specular > 0 && n_dot_l > 0) {
            Vec3 reflect_dir = vec3_sub(vec3_scale(hit->normal, 2.0f * n_dot_l), light_dir);
            float r_dot_v = vec3_dot(reflect_dir, view_dir);

            if (r_dot_v > 0) {
                float spec_intensity = pow(r_dot_v, hit->specular);
                Vec3 specular = vec3_scale(light->color, light->intensity * spec_intensity);
                color = vec3_add(color, specular);
            }
        }
    }

    return vec3_clamp(color);
}

// ============================================================================
// Camera Ray Generation (GPU)
// ============================================================================

inline Ray generate_camera_ray(const Camera* camera, float x, float y, int width, int height) {
    Vec3 forward = vec3_normalize(vec3_sub(camera->look_at, camera->position));
    Vec3 right = vec3_normalize(vec3_cross(forward, camera->up));
    Vec3 up = vec3_cross(right, forward);

    float fov_rad = camera->fov * 3.14159265359f / 180.0f;
    float viewport_height = 2.0f * tan(fov_rad / 2.0f);
    float viewport_width = viewport_height * camera->aspect;

    float u = (x / (width - 1)) * 2.0f - 1.0f;
    float v = (1.0f - y / (height - 1)) * 2.0f - 1.0f;

    Vec3 ray_dir = vec3_add(
        vec3_add(
            forward,
            vec3_scale(right, u * viewport_width / 2.0f)
        ),
        vec3_scale(up, v * viewport_height / 2.0f)
    );

    Ray ray;
    ray.origin = camera->position;
    ray.direction = vec3_normalize(ray_dir);

    return ray;
}

// ============================================================================
// Utility Functions
// ============================================================================

inline unsigned char float_to_byte(float f) {
    int i = (int)(f * 255.0f + 0.5f);
    return (unsigned char)fmax(0.0f, fmin(255.0f, (float)i));
}

inline float gamma_correct(float linear) {
    if (linear <= 0.0031308f) {
        return 12.92f * linear;
    } else {
        return 1.055f * pow(linear, 1.0f / 2.4f) - 0.055f;
    }
}

// ============================================================================
// Main Kernel: ONE THREAD PER PIXEL
// ============================================================================

__kernel void raytrace_primary(
    __global unsigned char* framebuffer,
    __global const Sphere* spheres,
    int n_spheres,
    __global const Plane* planes,
    int n_planes,
    __global const Light* lights,
    int n_lights,
    Camera camera,
    Vec3 ambient,
    int width,
    int height
) {
    // Each thread renders exactly ONE pixel
    int x = get_global_id(0);
    int y = get_global_id(1);

    // Bounds check
    if (x >= width || y >= height) {
        return;
    }

    // Generate ray for this pixel
    Ray ray = generate_camera_ray(&camera, (float)x, (float)y, width, height);

    // Intersect scene
    Hit hit;
    Vec3 color;

    if (!ray_scene_intersect(&ray, spheres, n_spheres, planes, n_planes, &hit)) {
        // Sky background
        color = vec3_make(0.5f, 0.7f, 1.0f);
    } else {
        // Compute shading
        Vec3 view_dir = vec3_normalize(vec3_scale(ray.direction, -1.0f));
        color = phong_shading(&hit, lights, n_lights, ambient, view_dir);
    }

    // Write to framebuffer (RGB24 format)
    int idx = (y * width + x) * 3;
    framebuffer[idx + 0] = float_to_byte(gamma_correct(color.x));
    framebuffer[idx + 1] = float_to_byte(gamma_correct(color.y));
    framebuffer[idx + 2] = float_to_byte(gamma_correct(color.z));
}
