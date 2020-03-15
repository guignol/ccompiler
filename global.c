#include "common.h"

struct Global_C {
    Global *head;
    Global *tail;
};

// グローバル変数その他
struct Global_C *globals;

void init_globals() {
    globals = calloc(1, sizeof(struct Global_C));
}

Global *get_globals() {
    return globals->head;
}

void add_globals(Global *next) {
    if (!globals->head) {
        globals->head = next;
        globals->tail = next;
        return;
    }
    globals->tail = globals->tail->next = next;
}

char *new_label() {
    static int cnt = 0;
    char buf[20];
    sprintf(buf, ".LC.%d", cnt++);
    return strndup(buf, 20);
}

Global *find_string_literal(char *name, int len) {
    for (Global *g = globals->head; g; g = g->next) {
        if (!g->target) {
            continue;
        }
        if (g->target->directive != _string) {
            continue;
        }
        Directives *target = g->target;
        if (target->literal_length == len &&
            !strncmp(name, target->literal, target->literal_length)) {
            return g;
        }
    }
    return NULL;
}

Global *get_string_literal(char *name, int len) {
    Global *g = find_string_literal(name, len);
    if (!g) {
        char *const label = new_label();
        const int label_length = (int) strlen(label);
        g = calloc(1, sizeof(Global));
        g->label = label;
        g->label_length = label_length;
        g->target = calloc(1, sizeof(Directives));
        g->target->directive = _string;
        g->target->literal = name;
        g->target->literal_length = len;
        // Globalsに追加
        add_globals(g);
    }
    return g;
}

Global *find_global_variable(char *name, int len) {
    for (Global *g = globals->head; g; g = g->next)
        if (g->label_length == len && !strncmp(name, g->label, g->label_length))
            return g;
    return NULL;
}

//////////////////////////////////////////////////////////////////

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

bool same_pointer(Node *left_p, Node *right_p) {
    if (left_p == NULL && right_p == NULL) {
        return true;
    } else if (left_p != NULL && right_p != NULL) {
        if (left_p->kind != right_p->kind) {
            return false;
        }
        switch (left_p->kind) {
            case ND_VARIABLE:
            case ND_VARIABLE_ARRAY: {
                if (left_p->len == right_p->len && !memcmp(left_p->name, right_p->name, left_p->len)) {
                    return true;
                }
                return false;
            }
            case ND_ADDRESS:
                return same_pointer(left_p->lhs, right_p->lhs);
            default:
                return false;
        }
    } else {
        return false;
    }
}

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
        // 整数のうち、0のみポインタと比較できる
        if (right == 0) {
            return 0;
        }
    } else if (left_p == NULL && right_p != NULL) {
        // 右側にポインタ
        // 0 < p
        // 整数のうち、0のみポインタと比較できる
        if (left == 0) {
            return 1;
        }
    } else {
        // ポインタ無しの場合はそのまま計算
        // ポインタ2つの場合はオフセット値のみで計算
        if (same_pointer(left_p, right_p)) {
            return operation(left, right);
        }
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
            // 最終的な呼び出し元ではポインタは1つしか扱わないが、途中で同じポインタは登場できる
            // 同じポインタに対する計算はコンパイル時に決定できるため
            Node *left_p = NULL;
            Node *right_p = NULL;
            int left = reduce_int(node->lhs, &left_p);
            int right = reduce_int(node->rhs, &right_p);
            if (left_p != NULL && right_p == NULL) {
                // 左側にポインタ
                *pointed = left_p;
                return left - right;
            } else if (left_p == NULL && right_p != NULL) {
                // 右側にポインタ
                *pointed = right_p;
                return left - right;
            } else {
                if (same_pointer(left_p, right_p)) {
                    if (pointed) {
                        *pointed = NULL;
                    }
                    return left - right;
                }
            }
            goto error_label;
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
                case ND_VARIABLE_ARRAY:
                    return get_pointer(referred, pointed);
                default:
                    goto error_label;
            }
        }
        case ND_VARIABLE_ARRAY:
            return get_pointer(node, pointed);
        default:
            goto error_label;
    }

    error_label:
    {
        error_at(loc__, "ぐええええええええええええ\n");
        exit(1);
    }
}

enum DIRECTIVE type_to_directive(Type *type) {
    switch (type->ty) {
        case TYPE_CHAR:
            return _byte;
        case TYPE_INT:
            return _long;
        case TYPE_POINTER:
            return _quad;
        case TYPE_ARRAY: {
            Type *pointed = type->point_to;
            while (pointed->ty == TYPE_ARRAY) {
                pointed = pointed->point_to;
            }
            return type_to_directive(pointed);
        }
        case TYPE_ENUM:
            return _enum;
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