#include "rasterize.h"
#include "types.h"
#include "util.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define MAX(a, b) a > b ? a : b
#define MIN(a, b) a < b ? a : b
#define CLAMP(a, b, c) MAX(MIN(a, c), b)

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

void rasterize_triangle(Triangle* triangle, Raster* raster, u32* render_count) {
    if (double_signed_area_triangle(triangle) < 0) {
        return;
    }

    (*render_count)++;

    u32 top, bottom, left, right;

    top    = CLAMP(MIN(MIN(triangle->points[0].y, triangle->points[1].y), triangle->points[2].y), 0, raster->height - 1);
    bottom = CLAMP(MAX(MAX(triangle->points[0].y, triangle->points[1].y), triangle->points[2].y), 0, raster->height - 1);
    left   = CLAMP(MIN(MIN(triangle->points[0].x, triangle->points[1].x), triangle->points[2].x), 0, raster->width  - 1);
    right  = CLAMP(MAX(MAX(triangle->points[0].x, triangle->points[1].x), triangle->points[2].x), 0, raster->width  - 1);

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

Triangle random_triangle(void) {
    Vector2 position = {rand() % 1920, rand() % 1080};

    Triangle triangle = {
        .points = {
            {position.x - 15, position.y - 15},
            {position.x + 15, position.y - 15},
            {position.x, position.y + 15},
        },
        .color = {
            rand() % 128 + 128,
            rand() % 128 + 128,
            rand() % 128 + 128,
        }
    };

    return triangle;
}

void rasterize(void) {
    Raster raster = {
        .width = 1920,
        .height = 1080,
        .data = calloc(1920 * 1080, 3)
    };

    u32 num_triangles = 95000;

    Triangle* triangles = calloc(num_triangles, sizeof(Triangle));

    srand(current_time_secs());

    double start = current_time_secs();
    
    for (u32 i = 0; i < num_triangles; i++) {
        triangles[i] = random_triangle();
    }

    double end = current_time_secs();

    printf("%f seconds to generate %d triangles\n", end - start, num_triangles);

    u32 render_count = 0;

    start = current_time_secs();

    for (u32 i = 0; i < num_triangles; i++) {
        rasterize_triangle(triangles + i, &raster, &render_count);
    }

    end = current_time_secs();

    printf("%f seconds to render %d out of %d triangles\n", end - start, render_count, num_triangles);

    write_raster("test.ppm", &raster);

    free(raster.data);
}
