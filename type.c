#include "common.h"
#include "parse.h"

Type *void_type;

Type *shared_void_type() {
    if (!void_type) {
        void_type = calloc(1, sizeof(Type));
        void_type->ty = TYPE_VOID;
        void_type->point_to = NULL;
    }
    return void_type;
}

Type *char_type;

Type *shared_char_type() {
    if (!char_type) {
        char_type = calloc(1, sizeof(Type));
        char_type->ty = TYPE_CHAR;
    }
    return char_type;
}

Type *bool_type;

Type *shared_bool_type() {
    if (!bool_type) {
        bool_type = calloc(1, sizeof(Type));
        bool_type->ty = TYPE_BOOL;
    }
    return bool_type;
}

Type *int_type;

Type *shared_int_type() {
    if (!int_type) {
        int_type = calloc(1, sizeof(Type));
        int_type->ty = TYPE_INT;
    }
    return int_type;
}

Type *create_struct_type() {
    Type *struct_type = calloc(1, sizeof(Type));
    struct_type->ty = TYPE_STRUCT;
    return struct_type;
}

Type *create_enum_type() {
    Type *enum_type = calloc(1, sizeof(Type));
    enum_type->ty = TYPE_ENUM;
    return enum_type;
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
        if (left->ty == TYPE_ENUM) {
            // サイズ固定
            return right->ty == TYPE_INT;
        }
        if (right->ty == TYPE_ENUM) {
            // サイズ固定
            return left->ty == TYPE_INT;
        }
        return false;
    }
    switch (left->ty) {
        case TYPE_STRUCT: {
            const int length = left->struct_info->name_length;
            return length == right->struct_info->name_length &&
                   !memcmp(left->struct_info->type_name, left->struct_info->type_name, length);
        }
        case TYPE_ENUM:
        case TYPE_VOID:
        case TYPE_CHAR:
        case TYPE_BOOL:
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

enum Assignable are_assignable_type(Type *left, Type *right, bool r_zero) {
    if (left->ty == TYPE_ARRAY) {
        // 配列型の変数には代入できない。初期化のみ。 https://stackoverflow.com/a/41889579
        return CANNOT_ASSIGN;
    }
    if (are_same_type(left, right)) {
        return AS_SAME;
    }
    bool left_is_number = left->ty == TYPE_CHAR || left->ty == TYPE_BOOL || left->ty == TYPE_INT;
    bool right_is_number = right->ty == TYPE_CHAR || right->ty == TYPE_BOOL || right->ty == TYPE_INT;
    if (left_is_number && right_is_number) {
        // implicit conversion loses integer precisionの場合もある
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
    // voidポインタ
    if (left->ty == TYPE_POINTER && right->ty == TYPE_POINTER) {
        // TODO 配列がどういう扱いか未確認
        if (left->point_to->ty == TYPE_VOID || right->point_to->ty == TYPE_VOID) {
            return AS_SAME;
        }
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

    // ポインタ型にはあらゆる整数型が入るっぽいので通す
    if (left->ty == TYPE_POINTER &&
        (right->ty == TYPE_INT ||
         right->ty == TYPE_CHAR ||
         right->ty == TYPE_BOOL)) {
        if (r_zero) {
            // 0の場合は型は一致していると見なせる
            return AS_SAME;
        }
        return AS_INCOMPATIBLE;
    }
    if (left->ty == TYPE_BOOL) {
        // TODO たぶんこれでいいはず
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
        case ND_FUNC:
            return node->type;
        case ND_MUL:
        case ND_DIV:
        case ND_MOD:
        case ND_EQL:
        case ND_NOT:
        case ND_LESS:
        case ND_LESS_EQL:
        case ND_NUM:
            return shared_int_type();
        case ND_STR_LITERAL:
            return create_pointer_type(shared_char_type());
        case ND_MEMBER:
            return node->type;
        case ND_ADD:
        case ND_SUB: {
            Type *left = find_type(node->lhs);
            Type *right = find_type(node->rhs);
            if (left->ty == right->ty) {
                switch (left->ty) {
                    case TYPE_CHAR:
                    case TYPE_BOOL:
                    case TYPE_INT:
                    case TYPE_ENUM:
                        return left;
                    case TYPE_ARRAY:
                    case TYPE_POINTER: {
                        if (node->kind == ND_ADD) {
                            error("ポインター型どうしの足し算はできません？\n");
                            exit(1);
                        }
                        if (are_same_type(left, right)) {
                            // 本来の型はptrdiff_tとして定義されていて環境依存で
                            // x64 Linuxではlongらしい
                            // x64 LinuxはLP64なので64bit
                            // これをintとして扱うと、gccは-Wconversionオプションでwarningを出す
                            return shared_int_type();
                        }
                        error("異なるポインター型に対する演算はできません？\n");
                        exit(1);
                    }
                    case TYPE_VOID:
                        error("voidに対する演算はできません？\n");
                        exit(1);
                    case TYPE_STRUCT:
                        // TODO
                        error("[type]構造体実装中\n");
                        exit(1);
                }
            } else {
                switch (left->ty) {
                    case TYPE_CHAR:
                    case TYPE_BOOL:
                    case TYPE_INT:
                    case TYPE_ENUM:
                        return right;
                    case TYPE_POINTER:
                    case TYPE_ARRAY:
                        return left;
                    case TYPE_VOID:
                        error("voidに対する演算はできません？\n");
                        exit(1);
                    case TYPE_STRUCT:
                        // TODO
                        error("[type]構造体実装中\n");
                        exit(1);
                }
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
        case ND_INVERT:
            return shared_int_type();
        case ND_DEREF:
        case ND_DEREF_ARRAY_POINTER: {
            // オペランドがポインタ型または配列型であることは検証済みの前提
            Type *type = find_type(node->lhs);
            switch (type->ty) {
                case TYPE_VOID:
                case TYPE_CHAR:
                case TYPE_BOOL:
                case TYPE_INT:
                    error("ポインタ型ではありません\n");
                    exit(1);
                case TYPE_POINTER:
                case TYPE_ARRAY:
                    return type->point_to;
                case TYPE_STRUCT:
                    // TODO
                    error("[type]構造体実装中\n");
                    exit(1);
                case TYPE_ENUM:
                    // TODO
                    error("[type]enum実装中\n");
                    exit(1);
            }
        }
        case ND_BLOCK: {
            // 前置および後置インクリメント
            switch (node->incr) {
                case PRE:
                    // 計算後の値が必要
                    return find_type(node->statement->memory[1]);
                case POST:
                    // 計算前の値が必要
                    return find_type(node->statement->memory[0]);
                case NONE:
                    break;
            }
            Node *last = peek_last_node(node->statement);
            Type *type = find_type(last);
            return type;
        }
        case ND_LOGICAL_OR:
        case ND_LOGICAL_AND:
            return shared_bool_type();
        case ND_IF:
            if (node->type) {
                // 三項演算子
                return node->type;
            }
        default: {
            error("値を返さないはずです？\n");
            exit(1);
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
        case TYPE_VOID:
            error("voidで計算？\n");
            exit(1);
        case TYPE_CHAR:
        case TYPE_BOOL:
        case TYPE_INT:
        case TYPE_ENUM:
            return 1;
        case TYPE_POINTER:
        case TYPE_ARRAY:
            // 配列は代入できないがポインタと同様に演算ができる
            return get_size(type->point_to);
        case TYPE_STRUCT:
            return get_size(type);
    }
}

int get_size(Type *type) {
    if (!type) {
        error("型が分かりません？\n");
        exit(1);
    }
    switch (type->ty) {
        case TYPE_VOID:
            // Cの仕様には無いらしいがgccは1を返す（clangも同様）
            // https://stackoverflow.com/questions/1666224/what-is-the-size-of-void/1666255#1666255
            error("voidのサイズ？\n");
            exit(1);
        case TYPE_CHAR:
        case TYPE_BOOL:
            return sizeof(char); // 1
        case TYPE_INT:
        case TYPE_ENUM:
            return sizeof(int); // 4
        case TYPE_POINTER:
            return sizeof(int *); // 8
        case TYPE_ARRAY: {
            // 初期化式つきの配列の場合、まず0でここを通る
            int array_size = /** (int) */ type->array_size;
            return array_size * get_size(type->point_to);
        }
        case TYPE_STRUCT: {
            if (type->struct_info->members == NULL) {
                // サイズが不要なグローバル変数の宣言を除くと、
                // 定義が前方にある必要がある
                error("構造体の定義がありません\n");
                exit(1);
            }
            int size = 0;
            for (Variable *m = type->struct_info->members; m; m = m->next) {
                if (size < m->offset) {
                    size = m->offset;
                }
            }
            return size;
        }
    }
}

int get_element_count(Type *type) {
    if (!type) {
        error("型が分かりません？\n");
        exit(1);
    }
    switch (type->ty) {
        case TYPE_VOID:
        case TYPE_CHAR:
        case TYPE_BOOL:
        case TYPE_INT:
        case TYPE_POINTER:
            return 1;
        case TYPE_ARRAY:
            return /** (int) */ type->array_size * get_element_count(type->point_to);
        case TYPE_STRUCT:
            // TODO
            error("[type]構造体実装中\n");
            exit(1);
        case TYPE_ENUM:
            // TODO
            error("[type]enum実装中\n");
            exit(1);
    }
}

/////////////////////////////////////////////////

Type *printing = NULL;

char *base_type_name() {
    Type *type = printing;
    while (type->point_to) {
        type = type->point_to;
    }
    switch (type->ty) {
        case TYPE_VOID:
            return "void";
        case TYPE_CHAR:
            return "char";
        case TYPE_BOOL:
            return "bool";
        case TYPE_INT:
            return "int";
        default:
            // TODO 構造体やenumはそもそもここ通る？
            error("型が不明です\n");
            exit(1);
    }
}

int count_pointer(void) {
    int count = 0;
    while (printing->ty == TYPE_POINTER) {
        count++;
        printing = printing->point_to;
    }
    return count;
}

void print_type(FILE *__stream, Type *type) {
    printing = type;

    fprintf(__stream, "%s", base_type_name());
    // int ***(**)[2][3]
    const int pointers_inner = count_pointer();
    int size_array[10];
    int arrays;
    for (arrays = 0; printing->ty == TYPE_ARRAY; ++arrays) {
        size_array[arrays] = printing->array_size;
        printing = printing->point_to;
    }
    size_array[arrays + 1] = 0;
    const int pointers_outer = count_pointer();

    // 型に配列を含むかどうか
    const bool contains_array = 0 < arrays;
    // 配列へのポインタ
    const bool point_to_array = contains_array && 0 < pointers_inner;
    // 配列型を代入する場合は、配列の先頭要素へのポインタを表示する
    const bool point_to_first_element = contains_array && !point_to_array;

    for (int i = 0; i < pointers_outer; i = i - 1) {
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
    if (!warning) {
        return;
    }

    // TODO GCCの結果と比べる
    // warning: assignment to '左辺の型' from incompatible pointer type '右辺の型'
    fprintf(stderr, "@@@ ");
    print_type(stderr, left);
    fprintf(stderr, " <= ");
    print_type(stderr, right);
    fprintf(stderr, "\n");
}