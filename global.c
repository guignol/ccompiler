#include "parse.h"

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

Node *new_node_string_literal() {
    Global *g = get_string_literal(token->str, token->len);
    // ラベルを指すnodeを作る
    Node *const node = new_node(ND_STR_LITERAL, NULL, NULL);
    node->label = g->label;
    node->label_length = g->label_length;
    token = token->next;
    return node;
}

Global *find_global_variable_by_name(char *name, int len) {
    for (Global *g = globals->head; g; g = g->next) {
        if (g->label_length == len && !strncmp(name, g->label, len)) {
            return g;
        }
    }
    return NULL;
}

Global *find_enum_member(char *name, int len) {
    Global *const enum_m = find_global_variable_by_name(name, len);
    if (enum_m && enum_m->target && enum_m->target->directive == _enum) {
        return enum_m;
    }
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
        case ND_EQL: {
            // TODO ポインタのことは未確認
            int left = reduce_int(node->lhs, NULL);
            int right = reduce_int(node->rhs, NULL);
            return left == right ? 1 : 0;
        }
        case ND_NOT: {
            // TODO ポインタのことは未確認
            int left = reduce_int(node->lhs, NULL);
            int right = reduce_int(node->rhs, NULL);
            return left != right ? 1 : 0;
        }
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
        case TYPE_ENUM:
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
        case ND_STR_LITERAL:
            target->reference = node->label;
            target->reference_length = node->label_length;
            return target;
        case ND_NUM:
            // リテラル、sizeof
            target->value = node->val;
            return target;
        case ND_VARIABLE:
            if (type->ty == TYPE_ENUM) {
                target->value = node->val;
                return target;
            }
            goto error_label;
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
        case ND_LESS_EQL:
        case ND_EQL:
        case ND_NOT: {
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

//////////////////////////////////////////////////////////////////

Node *new_node_global_variable(char *str, int len) {
    Global *variable = find_global_variable_by_name(str, len);
    if (!variable) {
        error_at(str, "変数の宣言がありません。");
        exit(1);
    }
    if (variable->target && variable->target->directive == _enum) {
        return new_node_num(variable->target->value);
    }
    Node *node = calloc(1, sizeof(Node));
    bool is_array = variable->type->ty == TYPE_ARRAY;
    node->kind = is_array ? ND_VARIABLE_ARRAY : ND_VARIABLE;
    node->type = variable->type;
    load_struct(node->type);
    node->is_local = false;
    node->name = variable->label;
    node->len = variable->label_length;
    node->offset = 0;
    return node;
}

void global_variable_declaration(Token *variable_name, Type *type) {
    // 他のグローバル変数や関数との名前重複チェック
    Global *const previously = find_global_variable_by_name(
            variable_name->str,
            variable_name->len
    );
    // 同じファイル内で、同名で同型のグローバル変数は宣言できる
    if (previously && !are_same_type(previously->type, type)) {
        error_at(variable_name->str, "グローバル変数の名前が他のグローバル変数と重複しています");
        exit(1);
    }
    if (find_function(variable_name->str, variable_name->len)) {
        error_at(variable_name->str, "グローバル変数の名前が関数と重複しています");
        exit(1);
    }
    if (type->ty == TYPE_VOID) {
        error_at(token->str, "変数にvoidは使えません");
        exit(1);
    }
    /*
     * グローバル変数の初期化に使えるもの
     *
     * 【定数式】
     *  （OK）
     *  int a[] = {2, 0, 3};
     *  int moshi = 1 < 2 ? 3 : sizeof(a);
     *  (NG)
     * 　int n = ({3;});
     *  int x = *y;
     *  int x = c = 9;
     *
     *  三項演算子は使えるが、コンパイル時に解決される
     *
     * 【グローバル変数や関数のアドレス】
     *  (OK)
     *  int(*n)[2] = &m;
     *  int *y     = &m;
     *  size_t m[] = {(size_t)&a, 3};
     *  size_t m[] = {&a, 3};
     *  (NG)
     *  size_t m[] = {(int)&a, 3};
     *  int m[]    = {(int)&a, 3};
     *  int m[]    = {&a, 3};
     *
     *  ポインタのintへのキャストなし代入
     *   => error （ローカルではwarning）
     *  ポインタのintへのキャスト
     *   => error （ローカルではwarning）
     *
     *  現在のところキャストもsize_tも実装してないので、単に型チェックするだけで良いはず
     *
     * 【グローバル変数や関数のアドレスに定数を足したもの】
     *  引いたものもOK
     */
    Global *g = calloc(1, sizeof(Global));
    g->type = type;
    g->label = variable_name->str;
    g->label_length = variable_name->len;
    add_globals(g);
    char *loc = token->str;
    if (consume("=")) {
        if (previously && previously->target && previously->target->directive != _zero) {
            // 0初期化以外の初期化が行われている場合
            error_at(loc, "このグローバル変数は既に初期化済みです");
            exit(1);
        }
        // TODO 構造体の初期化 https://ja.cppreference.com/w/c/language/struct_initialization
        if (type->ty == TYPE_ARRAY) {
            if (token->kind == TK_STR_LITERAL) {
                if (type->point_to->ty != TYPE_CHAR) {
                    error_at(loc, "error: 代入式の左右の型が異なります。");
                    exit(1);
                }
                g->target = calloc(1, sizeof(Directives));
                g->target->directive = _string;
                g->target->literal = token->str;
                g->target->literal_length = type->array_size = token->len;
                token = token->next;
            } else {
                // 配列の初期化
                Node *const variable_node = new_node_global_variable(variable_name->str, variable_name->len);
                // 型チェックはここでできてるはず
                // 配列のサイズが明示されていない場合は、type変数が更新されるが
                // それ以外はそのままで問題無いはず
                Node *block = array_initializer(variable_node, type);

                Directives head;
                head.next = NULL;
                Directives *tail = &head;
                for (Node *next = block->statement;
                     next;
                     next = next->statement) {
                    // 代入式の右辺
                    Node *right = next->rhs;
                    tail = tail->next = global_initializer(loc, type, right);
                }
                g->target = head.next;
            }
        } else {
            Node *node = expr();
            // 型チェック
            // 暗黙のキャストはできないので
            // int *yy_yy = &x - 4; は通るが
            // int yy_yy = &x - 4; はエラーになる
            // 0は特別扱い
            // 暗黙のキャストだけどエラーにならないパターンもある
            // int *a = 55;
            assert_assignable(loc, type, node);
            g->target = global_initializer(loc, type, node);
        }
    } else {
        if (previously && previously->target && previously->target->directive == _zero) {
            // 0初期化済み
            expect(";");
            return;
        }
        if (type->ty == TYPE_ARRAY && type->array_size == 0) {
            error_at(loc, "配列のサイズが指定されていません。");
            exit(1);
        }
        g->target = calloc(1, sizeof(Directives));
        g->target->directive = _zero;
        if (type->ty == TYPE_STRUCT && !type->struct_info->members) {
            // 構造体の定義が前方に無い場合
            g->target->backwards_struct = type;
        } else {
            // サイズ分を0で初期化
            g->target->value = get_size(type);
        }
    }
    expect(";");
}