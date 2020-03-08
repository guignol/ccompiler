#include "common.h"

static char *loc__ = NULL;

int get_pointer(Node *node, Node **pointed) {
    if (!pointed) {
        error_at(loc__, "グローバル変数の初期化に失敗： 変数を発見\n");
        exit(1);
    } else if (*pointed) {
        // ポインタが複数あった
        error_at(loc__, "グローバル変数の初期化に失敗： 変数が複数\n");
        exit(1);
    } else {
        *pointed = node;
        // 足し算または引き算の項として0を返す
        // ポインタに対しては乗除できないので、入れ子になった計算を考慮する必要は無いはず
        return 0;
    }
}

int reduce_int(Node *node, Node **pointed);

int FUNC_LESS(int left, int right) {
    return left < right;
}

int FUNC_LESS_EQL(int left, int right) {
    return left <= right;
}

int reduce_compare(Node *node, int (*operation)(int, int)) {
    Node *left_p = NULL;
    Node *right_p = NULL;
    int left = reduce_int(node->lhs, &left_p);
    int right = reduce_int(node->rhs, &right_p);
    if (left_p != NULL && right_p == NULL) {
        // 左側にポインタ
        // p < 0
        if (right <= 0) {
            // 0は特別扱いでwarningは出さない by gcc
            if (right != 0) {
                warn_at(loc__, "整数とポインターの比較をしています\n");
            }
            return 0;
        }
    } else if (left_p == NULL && right_p != NULL) {
        // 右側にポインタ
        // 0 < p
        if (left <= 0) {
            // 0は特別扱いでwarningは出さない by gcc
            if (left != 0) {
                warn_at(loc__, "整数とポインターの比較をしています\n");
            }
            return 1;
        }
    } else {
        // ポインタ無しの場合はそのまま計算
        // ポインタ2つの場合はオフセット値のみで計算
        return operation(left, right);
    }
    error_at(loc__, "ぽっぽえええええええええええええ\n");
    exit(1);
}

int reduce_int(Node *node, Node **pointed) {
    switch (node->kind) {
        case ND_NUM:
            return node->val;
        case ND_ADD: {
            int left = reduce_int(node->lhs, pointed);
            int right = reduce_int(node->rhs, pointed);
            return left + right;
        }
        case ND_SUB: {
            // TODO 最終的な呼び出し元ではポインタは1つしか扱わないが、途中で同じポインタは登場できる
            //  同じポインタに対する計算はコンパイル時に決定できるため
            //  同じポインタかどうかの判定はどうやる？
            int left = reduce_int(node->lhs, pointed);
            int right = reduce_int(node->rhs, pointed);
            return left - right;
        }
        case ND_MUL: {
            // 子nodeには変数によるポインタアクセスなし
            int left = reduce_int(node->lhs, NULL);
            int right = reduce_int(node->rhs, NULL);
            return left * right;
        }
        case ND_DIV: {
            // 子nodeには変数によるポインタアクセスなし
            int left = reduce_int(node->lhs, NULL);
            int right = reduce_int(node->rhs, NULL);
            return left / right;
        }
        case ND_LESS: {
            // 同じポインタへのオフセットどうしの比較は可能
            // 片方がポインタの場合は値が0または負の数のみ可能
            // 負の数の場合はwarningあり（intとポインタの比較になるため）
            //  【OK】 0 < (global_string + 3);は可能だが
            //  【NG】 1 < (global_string + 3);は不可能
            //  【OK】 (1 - 2) < (global_string + 3);
            //  等号が逆およびイコールつきでも同様
            return reduce_compare(node, FUNC_LESS);
        }
        case ND_LESS_EQL:
            return reduce_compare(node, FUNC_LESS_EQL);
        case ND_ADDRESS: {
            Node *const referred = node->lhs;
            switch (referred->kind) {
                case ND_VARIABLE:
                    return get_pointer(referred, pointed);
                default:
                    return reduce_int(referred, pointed);
            }
        }
        case ND_VARIABLE_ARRAY:
            return get_pointer(node, pointed);
        default:
            error_at(loc__, "ぐええええええええええええ\n");
            exit(1);
    }
}

DIRECTIVE type_to_directive(Type *type) {
    switch (type->ty) {
        case TYPE_CHAR:
            return _byte;
        case TYPE_INT:
            return _long;
        case TYPE_POINTER:
            return _quad;
        default:
            error_at(loc__, "どおおおおおおおおおおおおおお\n");
            exit(1);
    }
}

Directives *global_initializer(char *const loc, Type *type, Node *const node) {
    loc__ = loc;
    Directives *const target = calloc(1, sizeof(Directives));
    target->directive = type_to_directive(type);
    switch (node->kind) {
        case ND_EQL:
        case ND_NOT:
            error_at(loc, "TODO: グローバル変数の初期化は未実装");
            exit(1);
        case ND_STR_LITERAL:
            target->reference = node->label;
            target->reference_length = node->label_length;
            return target;
        case ND_NUM:
            // リテラル、sizeof
            target->value = node->val;
            return target;
        case ND_ADD:
        case ND_SUB: {
            Node *pointed = NULL;
            int sum = reduce_int(node, &pointed);
            if (pointed) {
                // 現実的な例としては、配列（の先頭を指すポインタ）がある
                // char head[4];
                // char *second = head + 1;
                target->reference = pointed->name;
                target->reference_length = pointed->len;
                target->value = sum;
            } else {
                target->value = sum;
            }
            return target;
        }
        case ND_MUL:
        case ND_DIV:
        case ND_LESS:
        case ND_LESS_EQL: {
            int sum = reduce_int(node, NULL);
            target->value = sum;
            return target;
        }
        case ND_DEREF_ARRAY_POINTER: {
            // 配列の途中まで、またはポインタへの[]アクセスの場合
            // ポインタ
            Node *pointed = NULL;
            int sum = reduce_int(node->lhs, &pointed);
            if (pointed) {
                target->reference = pointed->name;
                target->reference_length = pointed->len;
                target->value = sum;
                return target;
            }
            goto error_label;
        }
        case ND_ADDRESS: {
            // ポインタ
            Node *referred = node->lhs;
            switch (referred->kind) {
                case ND_VARIABLE:
                case ND_VARIABLE_ARRAY:
                    target->reference = referred->name;
                    target->reference_length = referred->len;
                    return target;
                case ND_DEREF:
                case ND_DEREF_ARRAY_POINTER: {
                    /*
                     * 例.1
                     * int global_array_int[4];
                     * int *global_array_element_pointer = &global_array_int[2];
                     * 例.2
                     * int global_array_array[2][3];
                     * int (*test)[3] = &(global_array_array[1]);
                     */
                    Node *pointed = NULL;
                    int sum = reduce_int(referred->lhs, &pointed);
                    if (pointed) {
                        target->reference = pointed->name;
                        target->reference_length = pointed->len;
                        target->value = sum;
                        return target;
                    }
                }
                default:
                    goto error_label;
            }
        }
        default:
            goto error_label;
    }
    error_label:
    {
        error_at(loc, "グローバル変数の初期化では非対応です？");
        exit(1);
    }
}