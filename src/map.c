#include "types.h"
#include "map.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct MapData {
    u32 key;
    MapData* next;
    bool set;
};

MapData* map_get_data_address(Map* map, u32 key) {
    return (MapData*)((u8*)map->data + (key % map->capacity) * (sizeof(MapData) + map->data_size));
}

void map_init(Map* map, u32 data_size, u32 initial_capacity) {
    void* data = calloc(initial_capacity, sizeof(MapData) + data_size);

    if (!data) {
        fprintf(stderr, "Failed to allocate map\n");
        return;
    }

    *map = (Map) {
        .data = data,
        .data_size = data_size,
        .length = 0,
        .capacity = initial_capacity,
    };
}

void map_data_clean(MapData* map_data) {
    if (!map_data) {
        return;
    }
    if (map_data->next) {
        map_data_clean(map_data->next);
    }
    free(map_data);
}

void map_clean(Map* map) {
    for (int i = 0; i < map->capacity; i++) {
        map_data_clean(map_get_data_address(map, i)->next);
    }
    free(map->data);
}

MapData* map_add(Map* map, u32 key) {
    MapData* map_data = map_get_data_address(map, key);

    while (map_data->set) {
        if (map_data->key == key) {
            return map_data;
        }
        if (!map_data->next) {
            map_data->next = calloc(1, sizeof(MapData) + map->data_size);
            if (!map_data->next) {
                fprintf(stderr, "failed to add key %d to map\n", key);
                return NULL;
            }
            map_data = map_data->next;
            break;
        }
        map_data = map_data->next;
    }

    (*map_data) = (MapData) {
        .key = key,
        .next = NULL,
        .set = true,
    };

    map->length++;

    if (map->length > map->capacity * 4 / 5) {
        // TODO: grow map
        printf("map should grow\n");
    }

    return map_data;
}

void map_set(Map* map, u32 key, void* data) {
    MapData* map_data = map_get_data_address(map, key);

    map_data = map_add(map, key);

    void* dest = map_data + 1;

    memcpy(dest, data, map->data_size);
}

void* map_get(Map* map, u32 key) {
    MapData* map_data = map_get_data_address(map, key);

    while (map_data->key != key) {
        if (!map_data->next) {
            fprintf(stderr, "Key %d is not in map\n", key);
            return NULL;
        }
        map_data = map_data->next;
    }

    return map_data + 1;
}
