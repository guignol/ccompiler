#include "common.h"

STRUCT_INFO *create_struct_info(const char *type_name, int name_length) {
    const int capacity = 10;
    STRUCT_INFO *struct_info = malloc(sizeof(STRUCT_INFO));
    struct_info->type_name = type_name;
    struct_info->name_length = name_length;
    // TODO 後でmallocしてもいいかも？
    struct_info->members = malloc(sizeof(Type *) * capacity);
    struct_info->count = 0;
    struct_info->capacity = capacity;
    return struct_info;
}

STRUCT_INFO *push_type_to_struct(STRUCT_INFO *struct_info, Type *type) {
    // TODO 同じ名前のものが無いか確認する
    if (struct_info->count == struct_info->capacity) {
        struct_info->members = realloc(struct_info->members, sizeof(Type *) * struct_info->capacity * 2);
        struct_info->capacity *= 2;
    }
    struct_info->members[struct_info->count] = type;
    struct_info->count++;
    return struct_info;
}

/////////////////////////
