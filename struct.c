#include "common.h"

STRUCT_INFO *create_struct_info(const char *type_name, int name_length) {
    STRUCT_INFO *struct_info = malloc(sizeof(STRUCT_INFO));
    struct_info->type_name = type_name;
    struct_info->name_length = name_length;
    struct_info->members = NULL;
    return struct_info;
}

void push_type_to_struct(STRUCT_INFO *struct_info, Variable *member) {
    if (struct_info->members) {
        struct_info->members = struct_info->members->next = member;
    } else {
        struct_info->members = member;
    }
}

/////////////////////////
