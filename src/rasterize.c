#include "rasterize.h"
#include "types.h"
#include "util.h"
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define max(a, b) (a) > (b) ? (a) : (b)
#define min(a, b) (a) < (b) ? (a) : (b)
#define clamp(value, low, high) max(min((value), (high)), (low))

#define PI 3.14159265358979323846

typedef struct {
    u32 width;
    u32 height;
    u8* data;
} Raster;

typedef struct {
    float x, y;
} Vector2;

typedef struct {
    u8 r, g, b;
} Color;

typedef struct {
    Vector2 points[3];
    Color color;
} Triangle;

typedef struct {
    float A, B, C;
} Edge;

void raster_init(Raster* raster, u32 width, u32 height) {
    raster->width = width;
    raster->height = height;
    raster->data = calloc(width * height, 3);
}

static inline Vector2 vector2_add(Vector2 v1, Vector2 v2) {
    return (Vector2) {v1.x + v2.x, v1.y + v2.y};
}

static inline Vector2 vector2_subtract(Vector2 v1, Vector2 v2) {
    return (Vector2) {v1.x - v2.x, v1.y - v2.y};
}

static inline Vector2 vector2_rotate(Vector2 v, float radians) {
    return (Vector2) {
        v.x * cosf(radians) - v.y * sinf(radians),
        v.x * sinf(radians) + v.y * cosf(radians)
    };
}

void triangle_rotate_centroid(Triangle* triangle, float radians) {
    Vector2 centroid = {
        (triangle->points[0].x + triangle->points[1].x + triangle->points[2].x) / 3,
        (triangle->points[0].y + triangle->points[1].y + triangle->points[2].y) / 3,
    };

    for (u32 i = 0; i < 3; i++) {
        Vector2 relative = vector2_subtract(triangle->points[i], centroid);
        Vector2 relative_rotated = vector2_rotate(relative, radians);
        triangle->points[i] = vector2_add(relative_rotated, centroid);
    }
}

Triangle random_triangle(const Raster* raster, float triangle_size) {
    Vector2 position = {rand() % raster->width, rand() % raster->height};

    Triangle triangle = {
        .points = {
            {position.x - triangle_size, position.y - triangle_size},
            {position.x + triangle_size, position.y - triangle_size},
            {position.x, position.y + triangle_size},
        },
        .color = {
            rand() % 128 + 128,
            rand() % 128 + 128,
            rand() % 128 + 128,
        }
    };

    triangle_rotate_centroid(&triangle, rand());

    return triangle;
}

bool write_raster(const char* path, const Raster* raster) {
    if (!path) {
        fprintf(stderr, "Invalid path specified\n");
        return false;
    }

    if (!raster || !raster->data || raster->width == 0 || raster->height == 0) {
        fprintf(stderr, "Malformed raster when writing image %s\n", path);
        return false;
    }

    FILE* file = fopen(path, "wb");

    if (!file) {
        perror(path);
        return false;
    }

    if (fprintf(file, "P6\n%d %d\n255\n", raster->width, raster->height) < 0) {
        fprintf(stderr, "Failed to write header to image %s\n", path);
        fclose(file);
        return false;
    }

    size_t expected = (size_t)raster->width * (size_t)raster->height * 3;

    size_t written = fwrite(raster->data, 1, expected, file);

    if (written != expected) {
        fprintf(stderr, "Could not write full raster data to disk when writing image %s\n", path);
        fclose(file);
        return false;
    }

    if (fclose(file) != 0) {
        fprintf(stderr, "Error closing file %s\n", path);
        return false;
    }

    return true;
}

float double_signed_area_edge(Edge* edge, Vector2 point) {
    return edge->A * point.x + edge->B * point.y + edge->C;
}

typedef struct {
    Edge e1, e2, e3;
} Edges;

static inline Edge create_edge(Vector2 a, Vector2 b) {
    return (Edge) {
        a.y - b.y,
        b.x - a.x,
        a.x * b.y - a.y * b.x
    };
}

float double_signed_area_triangle(Triangle *triangle) {
    return (triangle->points[1].x - triangle->points[0].x) * (triangle->points[2].y - triangle->points[0].y) -
        (triangle->points[1].y - triangle->points[0].y) * (triangle->points[2].x - triangle->points[0].x);
}

void rasterize_triangle_brute(Triangle* triangle, Raster* raster, u32* render_count) {
    if (double_signed_area_triangle(triangle) < 0) {
        return;
    }

    (*render_count)++;

    u32 top, bottom, left, right;

    top    = clamp(min(min(triangle->points[0].y, triangle->points[1].y), triangle->points[2].y), 0, raster->height - 1);
    bottom = clamp(max(max(triangle->points[0].y, triangle->points[1].y), triangle->points[2].y), 0, raster->height - 1);
    left   = clamp(min(min(triangle->points[0].x, triangle->points[1].x), triangle->points[2].x), 0, raster->width  - 1);
    right  = clamp(max(max(triangle->points[0].x, triangle->points[1].x), triangle->points[2].x), 0, raster->width  - 1);

    Edge AB = create_edge(triangle->points[0], triangle->points[1]);
    Edge BC = create_edge(triangle->points[1], triangle->points[2]);
    Edge CA = create_edge(triangle->points[2], triangle->points[0]);

    u32 row_size = raster->width * 3;
    for (u32 y = top; y <= bottom; y++) {
        u32 row_index = row_size * y;

        Vector2 point = {left, y};

        float ABP = double_signed_area_edge(&AB, point);
        float BCP = double_signed_area_edge(&BC, point);
        float CAP = double_signed_area_edge(&CA, point);

        for (u32 x = left; x <= right; x++) {
            if (ABP >= 0 && BCP >= 0 && CAP >= 0) {
                raster->data[row_index + x * 3 + 0] = triangle->color.r;
                raster->data[row_index + x * 3 + 1] = triangle->color.g;
                raster->data[row_index + x * 3 + 2] = triangle->color.b;
            }

            ABP += AB.A;
            BCP += BC.A;
            CAP += CA.A;
        }
    }
}

void rasterize_triangle_walk(Triangle* triangle, Raster* raster, u32* render_count) {
    if (double_signed_area_triangle(triangle) < 0) {
        return;
    }

    (*render_count)++;

    Vector2 top_v = triangle->points[0];
    Vector2 right_v = triangle->points[1];
    Vector2 left_v = triangle->points[2];

    u32 top, bottom, left, right;

    top    = clamp(top_v.y, 0, raster->height - 1);
    bottom = clamp(max(left_v.y, right_v.y), 0, raster->height - 1);
    left   = clamp(left_v.x, 0, raster->width  - 1);
    right  = clamp(right_v.x, 0, raster->width  - 1);

    float ABS = (top_v.x - right_v.x) / (top_v.y - right_v.y);
    float BCS = (top_v.x - left_v.x) / (top_v.y - left_v.y);

    u32 row_size = raster->width * 3;
    for (u32 y = top; y <= bottom; y++) {
        u32 row_index = row_size * y;

        u32 rel_left = clamp(BCS * (y - top) + left, right, left);
        u32 rel_right = clamp(ABS * (y - top) + left, right, left);

        for (u32 x = rel_left; x <= rel_right; x++) {
            raster->data[row_index + x * 3 + 0] = triangle->color.r;
            raster->data[row_index + x * 3 + 1] = triangle->color.g;
            raster->data[row_index + x * 3 + 2] = triangle->color.b;
        }
    }
}

void rasterize(void) {
    u32 width = 1920;
    u32 height = 1080;

    Raster raster;
    raster_init(&raster, width, height);

    u32 num_triangles = 10000;

    Triangle* triangles = calloc(num_triangles, sizeof(Triangle));

    srand(current_time_secs());

    double start = current_time_secs();
    
    for (u32 i = 0; i < num_triangles; i++) {
        triangles[i] = random_triangle(&raster, 15);
    }

    double end = current_time_secs();

    printf("%f seconds to generate %d triangles\n", end - start, num_triangles);

    u32 render_count = 0;

    start = current_time_secs();

    for (u32 i = 0; i < num_triangles; i++) {
        rasterize_triangle_brute(triangles + i, &raster, &render_count);
    }

    end = current_time_secs();

    printf("%f seconds to render %d out of %d triangles\n", end - start, render_count, num_triangles);

    Triangle triangle = {
        .points = {
            {400, 400}, // top
            {500, 500}, // right
            {300, 500}, // left
        },
        .color = { 255, 255, 255 }
    };

    rasterize_triangle_walk(&triangle, &raster, &render_count);

    write_raster("test.ppm", &raster);

    free(raster.data);
}
