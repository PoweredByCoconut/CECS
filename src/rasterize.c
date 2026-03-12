#include "rasterize.h"
#include <stdio.h>
#include <string.h>
#include "types.h"

u32 width = 256;
u32 height = 256;

typedef struct {
    float x, y;
} Vector2;

typedef struct {
    u8 r, g, b;
} Color;

typedef struct {
    union {
        Vector2 points[3];
        struct {
            Vector2 p1, p2, p3;
        };
    };
} Triangle;

_Static_assert(sizeof(Triangle) == sizeof(Vector2) * 3, "Unexpected padding");

typedef struct {
    u32 width, height;
    u8* data;
} Raster;

typedef struct {
    float top, bottom, left, right;
} Bounds;

static inline float max(float a, float b) {
    float diff = a - b;

    return a - ((diff) * (*(u32*)&diff >> 31));
}

static inline float min(float a, float b) {
    float diff = a - b;

    return b - ((b - a) * (*(u32*)&diff >> 31));
}

static inline Bounds get_bounds(Triangle* triangle) {
    return (Bounds) {
        .top = min(min(triangle->p1.y, triangle->p2.y), triangle->p3.y),
        .bottom = max(max(triangle->p1.y, triangle->p2.y), triangle->p3.y),
        .left = min(min(triangle->p1.x, triangle->p2.x), triangle->p3.x),
        .right = max(max(triangle->p1.x, triangle->p2.x), triangle->p3.x),
    };
}

static inline float cross(Vector2 v1, Vector2 v2) {
    return v1.x * v2.y - v1.y * v2.x;
}

static inline Vector2 subtract(Vector2 v1, Vector2 v2) {
    return (Vector2) {
        v1.x - v2.x,
        v1.y - v2.y,
    };
}

bool point_in_triangle(Vector2 point, Triangle triangle) {
    Vector2 AB, BC, CA, AP, BP, CP;
    AB = subtract(triangle.p1, triangle.p2);
    BC = subtract(triangle.p2, triangle.p3);
    CA = subtract(triangle.p3, triangle.p1);
    AP = subtract(triangle.p1, point);
    BP = subtract(triangle.p2, point);
    CP = subtract(triangle.p3, point);

    float cross_a, cross_b, cross_c;
    cross_a = cross(CA, AP);
    cross_b = cross(AB, BP);
    cross_c = cross(BC, CP);

    return (cross_a >= 0 && cross_b >= 0 && cross_c >= 0) || (cross_a <= 0 && cross_b <= 0 && cross_c <= 0);
}

void rasterize_triangle(Triangle* triangle, Color* color, Raster* raster) {
    Bounds triangle_bounds = get_bounds(triangle);

    u32 top = triangle_bounds.top * raster->height;
    u32 bottom = triangle_bounds.bottom * raster->height;
    u32 left = triangle_bounds.left * raster->width;
    u32 right = triangle_bounds.right * raster->width;

    for (u32 i = top; i <= bottom; i++) {
        for (u32 j = left; j <= right; j++) {
            float screen_x = (float) j / raster->width;
            float screen_y = (float) i / raster->height;
            if (point_in_triangle((Vector2) {screen_x, screen_y}, *triangle)) {
                raster->data[(i * raster->width + j) * 3 + 0] = color->r;
                raster->data[(i * raster->width + j) * 3 + 1] = color->g;
                raster->data[(i * raster->width + j) * 3 + 2] = color->b;
            }
        }
    }
}

void write_raster(Raster* raster) {
    FILE* file = fopen("test.ppm", "w");

    fprintf(file, "P6\n%d %d\n255\n", raster->width, raster->height);

    fwrite(raster->data, raster->width * raster->height * 3, sizeof(u8), file);

    fclose(file);
}

void rasterize(void) {
    Raster raster;
    raster.width = width;
    raster.height = height;
    u8 data[width * height * 3];
    memset(data, 0, width * height * 3);
    raster.data = data;

    Triangle triangle1 = {
        .points = {
            {0.25, 0.25},
            {0.75, 0.75},
            {0.3, 0.6}
        }
    };
    Triangle triangle2 = {
        .points = {
            {0.25, 0.25},
            {0.3, 0.6},
            {0.0, 1.0},
        }
    };

    Color color1 = {255, 0, 0};
    Color color2 = {0, 255, 0};

    rasterize_triangle(&triangle1, &color1, &raster);
    rasterize_triangle(&triangle2, &color2, &raster);
    write_raster(&raster);
}
