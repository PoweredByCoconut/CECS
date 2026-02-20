#include "list.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void list_init(List* list, u32 data_size, u32 initial_capacity) {
    void* data = calloc(initial_capacity, data_size);
    if (!data) {
        fprintf(stderr, "Unable to allocate space for list\n");
        return;
    }

    list->data = data;
    list->data_size = data_size;
    list->length = 0;
    list->capacity = initial_capacity;
}

void list_clean(List* list) {
    free(list->data);
}

void* list_add(List* list, void* value) {
    if (list->length >= list->capacity - 1) {
        void* new_data = realloc(list->data, 2 * list->capacity * list->data_size);

        if (!new_data) {
            fprintf(stderr, "Unable to grow list, value not added\n");
            return NULL;
        }

        list->capacity *= 2;
    }

    void* dest = (u8*)list->data + list->length * list->data_size;

    if (value) {
        memcpy(dest, value, list->data_size);
    }

    list->length++;

    return dest;
}

void* list_get(List* list, u32 i) {
    return (u8*)list->data + i * list->data_size;
}
