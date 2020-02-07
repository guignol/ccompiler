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

Type *create_array_type(Type *element_type, int array_size) {
    Type *array = calloc(1, sizeof(Type));
    array->ty = TYPE_ARRAY;
    array->array_size = array_size;
    if (element_type->ty == TYPE_ARRAY) {
        /*
         * 配列の配列の場合
         *
         * int a[2][3];
         * a[1][2] = 1;
         *
         * 0--,1--
         * 444,444
         * 012,012
         *
         * []の部分は宣言の表記順になる
         * array of array of int
         * 2*3*4    3*4      4
         *
         */
        Type *terminal_array = element_type;
        while (true) {
            // array of int あるいは array of pointerまで下る
            if (terminal_array->point_to->ty == TYPE_ARRAY) {
                terminal_array = terminal_array->point_to;
            } else {
                break;
            }
        }
        // 最後から手前に差し込む
        Type *int_or_pointer = terminal_array->point_to;
        terminal_array->point_to = array;
        array->point_to = int_or_pointer;
        return element_type;
    } else {
        // array of element_type
        array->point_to = element_type;
        return array;
    }
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
        case TYPE_ARRAY:
            if (left->array_size == right->array_size) {
                return are_same_type(left->point_to, right->point_to);
            } else {
                return false;
            }
    }
}

Assignable are_assignable_type(Type *left, Type *right) {
    if (left->ty == TYPE_ARRAY) {
        // 配列型の変数には代入できない。初期化のみ。 https://stackoverflow.com/a/41889579
        return CANNOT_ASSIGN;
    }
    if (are_same_type(left, right)) {
        return AS_SAME;
    }
    if (left->ty == TYPE_POINTER &&
        right->ty == TYPE_ARRAY &&
        are_same_type(left->point_to, right->point_to)) {
        /*
         * int a[2];
         * int *c;
         * c = a;  => 右辺の型はint*
         * 配列変数をそのまま使うと先頭要素へのポインタ（int*）なので、gccではwarningが出ない
         *
         * int a[2][3];
         * int (*b)[3];
         * b = a;  => 右辺の型はint(*)[3]
         * 配列変数をそのまま使うと先頭要素へのポインタ（int(*)[3]）なので、gccではwarningが出ない
         */
        return AS_SAME;
    }
    // ポインタ型にはあらゆるポインタ型が入るっぽいので通す
    if (left->ty == TYPE_POINTER &&
        (right->ty == TYPE_POINTER ||
         right->ty == TYPE_ARRAY)) {
        /*
         * int a[2];
         * int *c;
         * c = &a; => 右辺の型はint(*)[2]で、gccではwarning
         * 値としては、配列の先頭要素へのポインタ(int*)
         *
         * int a[2];
         * int (*b)[2];
         * b = &a; => 右辺の型はint (*)[2]
         * b = a;  => 右辺の型はint *で、gccではwarning
         * ただし、前者は型が完全一致して何も考慮すべきことはないのでこの例外処理は不要
         */
        return AS_INCOMPATIBLE;
    }
    return CANNOT_ASSIGN;
}

Type *find_type(const Node *node) {
    if (!node) {
        error("Nodeがありません\n");
        exit(1);
    }
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
                    case TYPE_POINTER:
                    case TYPE_ARRAY: {
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
                    case TYPE_ARRAY:
                        return left;
                }
            }
            case ND_VARIABLE:
            case ND_VARIABLE_ARRAY:
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
            case ND_DEREF_ARRAY_POINTER:
            case ND_INDEX:
            case ND_INDEX_CONTINUE: {
                // オペランドがポインタ型または配列型であることは検証済みの前提
                Type *type = find_type(node->lhs);
                switch (type->ty) {
                    case TYPE_INT:
                        error("ポインタ型ではありません\n");
                        exit(1);
                    case TYPE_POINTER:
                    case TYPE_ARRAY:
                        return type->point_to;
                }
            }
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
        error("型が分かりません？\n");
        exit(1);
    }
    switch (type->ty) {
        case TYPE_INT:
            return 1;
        case TYPE_POINTER:
        case TYPE_ARRAY:
            // 配列は代入できないがポインタと同様に演算ができる
            return get_size(type->point_to);

    }
}

int get_size(Type *type) {
    if (!type) {
        error("型が分かりません？\n");
        exit(1);
    }
    switch (type->ty) {
        case TYPE_INT:
            return sizeof(int); // 4
        case TYPE_POINTER:
            return sizeof(int *); // 8
        case TYPE_ARRAY:
            return (int) type->array_size * get_size(type->point_to);
    }
}

bool type_32bit(Type *type) {
    return get_size(type) == 4; // bytes
}

/////////////////////////////////////////////////

Type *printing = NULL;

int count_pointer(void) {
    int count = 0;
    while (printing->ty == TYPE_POINTER) {
        count++;
        printing = printing->point_to;
    }
    return count;
}

int count_array(int counts[]) {
    int i;
    for (i = 0; printing->ty == TYPE_ARRAY; ++i) {
        counts[i] = printing->array_size;
        printing = printing->point_to;
    }
    counts[i + 1] = 0;
    return i;
}

void print_type(FILE *__stream, Type *type) {
    printing = type;

    fprintf(__stream, "int");
    // int ***(**)[2][3]
    const int pointers_inner = count_pointer();
    int size_array[10];
    const int arrays = count_array(size_array);
    const int pointers_outer = count_pointer();

    // 型に配列を含むかどうか
    const bool contains_array = 0 < arrays;
    // 配列へのポインタ
    const bool point_to_array = contains_array && 0 < pointers_inner;
    // 配列型を代入する場合は、配列の先頭要素へのポインタを表示する
    const bool point_to_first_element = contains_array && !point_to_array;

    for (int i = 0; i < pointers_outer; --i) {
        fprintf(__stream, "*");
    }

    point_to_array && fprintf(__stream, "(");
    for (int i = 0; i < pointers_inner; ++i) {
        fprintf(__stream, "*");
    }
    point_to_array && fprintf(__stream, ")");

    if (point_to_first_element) {
        if (arrays == 1) {
            fprintf(__stream, "*");
        } else {
            fprintf(__stream, "(*)");
        }
    }
    for (int i = point_to_first_element ? 1 : 0; i < arrays; ++i) {
        fprintf(__stream, "[%d]", size_array[i]);
    }
}

void warn_incompatible_type(Type *left, Type *right) {
    // TODO GCCの結果と比べる
    // warning: assignment to '左辺の型' from incompatible pointer type '右辺の型'
    fprintf(stderr, "\n");
    print_type(stderr, left);
    fprintf(stderr, " <= ");
    print_type(stderr, right);
}