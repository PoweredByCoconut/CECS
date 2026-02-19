#ifndef MAP_H
#define MAP_H

#include "types.h"

typedef struct MapData MapData;

typedef struct {
    MapData* data;
    u32 data_size;
    u32 length;
    u32 capacity;
} Map;

void map_init(Map* map, u32 data_size, u32 initial_capacity);

void map_clean(Map* map);

void map_grow(Map* map);

MapData* map_add(Map* map, u32 key);

void map_set(Map* map, u32 key, void* data);

void* map_get(Map* map, u32 key);

#endif
