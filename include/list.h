#ifndef LIST_H
#define LIST_H

#include "types.h"

typedef struct {
    void* data;
    u32 data_size;
    u32 length;
    u32 capacity;
} List;

void list_init(List* list, u32 data_size, u32 initial_capacity);

void list_clean(List* list);

void* list_add(List* list, void* value);

void* list_get(List* list, u32 i);

#endif
