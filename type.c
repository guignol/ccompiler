#include "9cc.h"

Type *shared_int_type() {
    static Type *int_type;
    if (!int_type) {
        int_type = calloc(1, sizeof(Type));
        int_type->ty = TYPE_INT;
        int_type->point_to = NULL;
    }
    return int_type;
}

Type *create_pointer_type(Type *point_to) {
    Type *type = calloc(1, sizeof(Type));
    type->ty = TYPE_POINTER;
    type->point_to = point_to;
    return type;
}

bool are_same_type(Type *left, Type *right) {
    if (left->ty != right->ty) {
        return false;
    }
    switch (left->ty) {
        case TYPE_INT:
            return true;
        case TYPE_POINTER:
            return are_same_type(left->point_to, right->point_to);
    }
}

Type *find_type(const Node *node) {
    switch (node->kind) {
        case ND_FUNC: // TODO
        case ND_MUL:
        case ND_DIV:
        case ND_EQL:
        case ND_NOT:
        case ND_LESS:
        case ND_LESS_EQL:
        case ND_NUM:
            return shared_int_type();
        case ND_ADD:
        case ND_SUB: {
            Type *left = find_type(node->lhs);
            Type *right = find_type(node->rhs);
            if (left->ty == right->ty) {
                switch (left->ty) {
                    case TYPE_INT:
                        return left;
                    case TYPE_POINTER: {
                        if (are_same_type(left, right)) {
                            return left;
                        }
                        error("異なるポインター型の演算はできません？\n");
                        exit(1);
                    }
                }
            } else {
                switch (left->ty) {
                    case TYPE_INT:
                        return right;
                    case TYPE_POINTER:
                        return left;
                }
            }
            case ND_VARIABLE:
                return node->type;
            case ND_ASSIGN: {
                // 左右の型が同じことは検証済みの前提
                return find_type(node->lhs);
            }
            case ND_ADDRESS: {
                Type *operand_type = find_type(node->lhs);
                return create_pointer_type(operand_type);
            }
            case ND_DEREF:
                // オペランドがポインタ型であることは検証済みの前提
                return find_type(node->lhs)->point_to;
            default: {
                error("値を返さないはずです？\n");
                exit(1);
            }
        }
    }
}

int get_weight(Node *node) {
    Type *type = find_type(node);
    if (!type) {
        error("型が分かりません？");
        exit(1);
    }
    switch (type->ty) {
        case TYPE_INT:
            return 1;
        case TYPE_POINTER:
            return get_size(type->point_to);
    }
}

int get_size(Type *type) {
    if (!type) {
        error("型が分かりません？");
        exit(1);
    }
    switch (type->ty) {
        case TYPE_INT:
            return sizeof(int); // 4
        case TYPE_POINTER:
            return sizeof(int *); // 8
    }
}

bool type_32bit(Type *type) {
    return get_size(type) == 4; // bytes
}