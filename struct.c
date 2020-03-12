#include "common.h"

struct STRUCT_REGISTRY {
    STRUCT_INFO **memory;
    int count;
    int capacity;
};

// TODO 実際にはローカルスコープもあるので、ここに持つのは微妙かも
struct STRUCT_REGISTRY *registry;

void init_struct_registry() {
    const int capacity = 5;
    registry = malloc(sizeof(struct STRUCT_REGISTRY));
    registry->memory = malloc(sizeof(STRUCT_INFO *) * capacity);
    registry->count = 0;
    registry->capacity = capacity;
}

void push_struct(STRUCT_INFO *info) {
    if (registry->count == registry->capacity) {
        registry->memory = realloc(registry->memory, sizeof(STRUCT_INFO *) * registry->capacity * 2);
        registry->capacity *= 2;
    }
    // TODO 同じ名前のものが来たら
    registry->memory[registry->count] = info;
    registry->count++;
}

void load_struct(STRUCT_INFO *target) {
    const char *const name = target->type_name;
    const int len = target->name_length;
    for (int i = 0; i < registry->count; ++i) {
        STRUCT_INFO *s = registry->memory[i];
        if (s->name_length == len && !memcmp(name, s->type_name, len)) {
            target->members = s->members;
            return;
        }
    }
}
