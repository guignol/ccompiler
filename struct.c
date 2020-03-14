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

STRUCT_INFO *find_struct(const void *const name, const int len) {
    for (int i = 0; i < registry->count; ++i) {
        STRUCT_INFO *s = registry->memory[i];
        if (s->name_length == len && !memcmp(name, s->type_name, len)) {
            return s;
        }
    }
    return NULL;
}

void push_struct(STRUCT_INFO *target) {
    // 同じ名前のものが来たら
    const char *const name = target->type_name;
    const int len = target->name_length;
    STRUCT_INFO *const same_struct = find_struct(name, len);
    if (same_struct) {
        //　TODO 定義が重複する場合
        if (same_struct->members == NULL) {
            same_struct->members = target->members;
        }
        return;
    }

    if (registry->count == registry->capacity) {
        registry->memory = realloc(registry->memory, sizeof(STRUCT_INFO *) * registry->capacity * 2);
        registry->capacity *= 2;
    }
    registry->memory[registry->count] = target;
    registry->count++;
}

void load_struct(Type *const type) {
    switch (type->ty) {
        case TYPE_POINTER:
        case TYPE_ARRAY:
            load_struct(type->point_to);
            break;
        case TYPE_STRUCT: {
            const char *const name = type->struct_info->type_name;
            const int len = type->struct_info->name_length;
            STRUCT_INFO *const found = find_struct(name, len);
            if (found && found->members) {
                type->struct_info->members = found->members;
            }
            break;
        }
        case TYPE_VOID:
        case TYPE_CHAR:
        case TYPE_INT:
            break;
    }
}

Variable *find_member(STRUCT_INFO *target, const char *const name, const int len) {
    for (Variable *member = target->members; member; member = member->next) {
        if (member->len == len && !memcmp(name, member->name, len)) {
            return member;
        }
    }
    return NULL;
}

Node *new_node_struct_member(Node *variable, const char *const name, const int len) {
    STRUCT_INFO *const struct_info = variable->type->struct_info;
    // 構造体の宣言の後、定義の前に宣言されたグローバル変数の場合、
    // 変数宣言時には型情報を持っていないので読み込んでおく
    load_struct(variable->type);
    Variable *const member = find_member(struct_info, name, len);
    if (member == NULL) {
        error_at(name, "構造体 %.*s にはメンバー %.*s がありません。",
                 struct_info->name_length,
                 struct_info->type_name,
                 len,
                 name);
        exit(1);
    }

    bool is_array = member->type->ty == TYPE_ARRAY; // TODO
    Node *node = calloc(1, sizeof(Node));
    node->kind = is_array ? ND_VARIABLE_ARRAY : ND_VARIABLE;
    node->type = member->type;
    node->is_local = variable->is_local;
    //　TODO メンバーの名前が含まれない。ネストした構造体が使えるようになったら改めて考える
    node->name = variable->name;
    node->len = variable->len;
    if (node->is_local) {
        node->offset = variable->offset + member->offset;
    } else {
        node->offset = member->offset - member->type_size;
    }
    return node;
}