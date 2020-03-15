#include "common.h"

EnumMembers *create_enum_member(int capacity) {
    if (capacity < 1) {
        capacity = 10;
    }
    EnumMembers *members = malloc(sizeof(EnumMembers));
    members->memory = malloc(sizeof(Global *) * capacity);
    members->count = 0;
    members->capacity = capacity;
    return members;
}

EnumMembers *push_enum_member(EnumMembers *members, Global *new) {
    if (members->count == members->capacity) {
        members->memory = realloc(members->memory, sizeof(Global *) * members->capacity * 2);
        members->capacity *= 2;
    }
    members->memory[members->count] = new;
    members->count++;
    return members;
}