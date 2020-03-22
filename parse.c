#include "parse.h"

// 現在着目しているトークン
Token *token;

/////////////////////////

int context = 0;
int_stack *break_stack = NULL;
int_stack *continue_stack = NULL;

/////////////////////////

int stack_size = 0;

typedef struct Scope Scope;

struct Scope {
    Variable *variables;
    Variable *tail;
    Scope *parent;
};

// 現在のスコープ
Scope *current_scope;
Token *function_name;

//////////////////////////////////////////////////////////////////

Variable *find_local_variable(Scope *const scope, char *name, int len) {
    if (!scope) {
        return NULL;
    }
    for (Variable *var = scope->variables; var; var = var->next) {
        if (var->len == len && !memcmp(name, var->name, len)) {
            return var;
        }
    }
    return find_local_variable(scope->parent, name, len);
}

Variable *register_variable(char *str, int len, Type *type) {
    Variable *variable = calloc(1, sizeof(Variable));
    variable->type = type;
    variable->name = str;
    variable->len = len;
    variable->type_size = get_size(type);
    variable->offset = stack_size = stack_size + variable->type_size;
    variable->index = -1;
    if (current_scope->variables) {
        current_scope->tail = current_scope->tail->next = variable;
    } else {
        current_scope->variables = variable;
        current_scope->tail = variable;
    }
    return variable;
}

void void_arg(char *loc) {
    Variable *void_arg = calloc(1, sizeof(Variable));
    void_arg->type = shared_void_type();
    void_arg->name = NULL;
    void_arg->len = 0;
    void_arg->type_size = 0;
    void_arg->offset = 0;
    void_arg->index = -1;
    if (current_scope->variables) {
        error_at(loc, "引数にvoid以外のものがあります？");
        exit(1);
    } else {
        current_scope->variables = void_arg;
        current_scope->tail = void_arg;
    }
}

//////////////////////////////////////////////////////////////////

Token *consume(char *op) {
    Token *const t = token;
    if (token->kind != TK_RESERVED ||
        strlen(op) != token->len ||
        memcmp(token->str, op, token->len) != 0)
        return NULL;
    token = token->next;
    return t;
}

Token *consume_ident() {
    if (token->kind == TK_IDENT) {
        Token *t = token;
        token = token->next;
        return t;
    }
    return NULL;
}

Token *consume_num_char() {
    if (token->kind == TK_NUM || token->kind == TK_CHAR_LITERAL) {
        Token *t = token;
        token = token->next;
        return t;
    }
    return NULL;
}

Type *consume_base_type() {
    Token *const alias = consume_ident();
    if (alias) {
        Type *type = find_alias(alias);
        if (type == NULL) {
            token = alias;
        }
        return type;
    } else if (consume("int")) {
        return shared_int_type();
    } else if (consume("char")) {
        return shared_char_type();
    } else if (consume("_Bool")) {
        return shared_bool_type();
    } else if (consume("void")) {
        return shared_void_type();
    } else if (consume("struct")) {
        Type *const base = create_struct_type();
        Token *const t = consume_ident();
        base->struct_info = malloc(sizeof(STRUCT_INFO));
        base->struct_info->type_name = t ? t->str : NULL;
        base->struct_info->name_length = t ? t->len : 0;
        base->struct_info->members = NULL;
        // 定義済みの型情報があれば読み込む
        load_struct(base);
        return base;
    } else if (consume("enum")) {
        Type *const base = create_enum_type();
        Token *const t = consume_ident();
        base->enum_info = malloc(sizeof(ENUM_INFO));
        base->enum_info->type_name = t ? t->str : NULL;
        base->enum_info->name_length = t ? t->len : 0;
        base->enum_info->members = create_enum_member(5);
        return base;
    } else {
        return NULL;
    }
}

Token *consume_type(Type *base, Type **r_type) {
    /**
     * どこから呼ばれてるか
     * 1. グローバル（ファイル）スコープ
     * 2. ローカルでの変数宣言および初期化 local_variable_declaration
     *   2.1. 関数スコープまたはブロックスコープ
     *   2.2. forスコープ
     * 3. sizeof
     *
     * 何ができるか
     * 型利用（変数宣言、関数宣言、関数定義、サイズ）
     * 1. 型定義　型宣言 型利用
     * 2.1. 型定義　型宣言 型利用
     * 2.2. 型利用
     * 3. 型利用（identifierなし）
     */
    bool backwards = consume("(");
    Type *type = backwards ? NULL : base;
    while (consume("*")) {
        Type *pointer = create_pointer_type(type);
        type = pointer;
    }
    Token *identifier_token = consume_ident();
    // 変数の宣言
    if (identifier_token && current_scope) {
        // 同じスコープ内で同名の変数は宣言できない
        // グローバル変数との重複は可能
        Scope *parent = current_scope->parent;
        current_scope->parent = NULL; // 現在のスコープのみ
        if (find_local_variable(current_scope, identifier_token->str, identifier_token->len)) {
            error_at(token->str, "変数名またはメンバー名が重複しています");
            exit(1);
        }
        current_scope->parent = parent;
    }
    Type *backwards_pointer = NULL;
    if (backwards) {
        expect(")");
        backwards_pointer = type;
        type = base;
    }

    while (consume("[")) {
        /**
         * intの配列
         * int p[2]
         *
         * ポインタの配列
         * int *p[3];
         *
         * ポインタへのポインタの配列
         * int **p[4];
         *
         * intの配列の配列
         * int p[5][6];
         *
         * 配列へのポインタ
         * int (*p)[];
         */
        Token *size_token = consume_num_char();
        if (!size_token) {
            // 最初の[]のみ、初期化式では右辺からサイズを決定できる
            // 配列へのポインタの場合も同様
            if (type->ty == TYPE_ARRAY) {
                error_at(token->str, "配列のサイズを省略できません");
                exit(1);
            }
            type = create_array_type(type, 0);
            expect("]");
            continue;
        }
        int array_size = size_token->val;
        type = create_array_type(type, array_size);
        expect("]");
    }
    if (backwards_pointer) {
        Type *edge = backwards_pointer;
        while (edge->point_to) {
            edge = edge->point_to;
        }
        edge->point_to = type;
        type = edge;
    }
    *r_type = type;
    return identifier_token;
}

Variable *consume_member() {
    Type *base = consume_base_type();
    if (!base) {
        error_at(token->str, "構造体のメンバーが正しく定義されていません");
        exit(1);
    }
    if (base->ty == TYPE_VOID) {
        // TODO voidポインタ
        error_at(token->str, "構造体のメンバーにvoidは使えません");
        exit(1);
    }
    // TODO 構造体やenumがここで定義される場合がある
    Type *type;
    Token *const identifier = consume_type(base, &type);
    if (!identifier) {
        error_at(token->str, "構造体のメンバーに名前がありません");
        exit(1);
    }
    Variable *variable = register_variable(identifier->str, identifier->len, type);
    expect(";");
    return variable;
}

bool consume_struct_def_or_dec(Type *base) {
    if (base->ty == TYPE_STRUCT) {
        // 構造体の定義
        if (consume("{")) {
            // 前処理
            const int saved_stack = stack_size; // TODO ローカルで定義されると効いてくる
            stack_size = 0;
            Scope *const saved_scope = current_scope;
            {
                // 構造体メンバー名の重複チェックに使う
                Scope disposable;
                disposable.variables = NULL;
                disposable.parent = NULL;
                current_scope = &disposable;
            }

            STRUCT_INFO *const struct_info = base->struct_info;
            Variable head;
            head.next = NULL;
            Variable *tail = &head;
            do {
                Variable *const member = consume_member();
                tail = tail->next = member;
            } while (!consume("}"));
            struct_info->members = head.next;

            // 後処理
            current_scope = saved_scope;
            stack_size = saved_stack;

            push_struct(base->struct_info);
            return true;
        }
        // 構造体の宣言
        if (check(";")) {
            push_struct(base->struct_info);
            return true;
        }
    }
    return false;
}

void consume_type_def(Type *base) {
    Type *type = base;
    while (consume("*")) {
        type = create_pointer_type(type);
    }
    Token *alias = consume_ident();
    register_type_def(type, alias);
}

// 次のトークンが期待している記号のときには、トークンを1つ読み進める。
// それ以外の場合にはエラーを報告する。
void expect(char *op) {
    if (token->kind != TK_RESERVED ||
        strlen(op) != token->len ||
        memcmp(token->str, op, token->len) != 0) {
        error_at_1(token->str, "'%s'ではありません", op);
        exit(1);
    }
    token = token->next;
}

// 次のトークンが数値の場合、トークンを1つ読み進めてその数値を返す。
// それ以外の場合にはエラーを報告する。
int expect_num_char() {
    if (token->kind != TK_NUM && token->kind != TK_CHAR_LITERAL) {
        error_at(token->str, "数ではありません");
        exit(1);
    }
    int val = token->val;
    token = token->next;
    return val;
}

bool check(char *op) {
    Token *const consumed = consume(op);
    if (consumed) {
        token = consumed;
        return true;
    }
    return false;
}

bool at_eof() {
    return token->kind == TK_EOF;
}

//////////////////////////////////////////////////////////////////

char *copy(char *str, int len) {
    char *copied = malloc(sizeof(char) * len + 1);
    strncpy(copied, str, len);
    copied[len] = '\0';
    return copied;
}

void assert_assignable(char *loc,
                       Type *const left_type,
                       Node *const rhs) {
    // コンパイラで生成しているnew_node_num(0)もあるはずだが、
    // 代入式の右辺の最上部Nodeに現れるものはリテラル相当のはず
    // （0リテラル、配列の0パディング）
    bool rZero = rhs->kind == ND_NUM && rhs->val == 0;
    Type *const right_type = find_type(rhs);
    switch (are_assignable_type(left_type, right_type, rZero)) {
        case AS_INCOMPATIBLE:
            warn_at(loc, "warning: 代入式の左右または引数の型が異なります。");
            warn_incompatible_type(left_type, right_type);
        case AS_SAME:
            break;
        case CANNOT_ASSIGN:
            error_at(loc, "error: 代入式の左右または引数の型が異なります。");
            exit(1);
    }
}

//////////////////////////////////////////////////////////////////

Node *new_node(enum NodeKind kind, Node *lhs, Node *rhs) {
    Node *node = calloc(1, sizeof(Node));
    node->kind = kind;
    node->lhs = lhs;
    node->rhs = rhs;
    return node;
}

Node *new_node_num(int val) {
    Node *node = calloc(1, sizeof(Node));
    node->kind = ND_NUM;
    node->val = val;
    return node;
}

Node *new_node_local_variable(char *str, int len) {
    Variable *variable = find_local_variable(current_scope, str, len);
    if (!variable) {
        // ローカル変数が無い場合はグローバル変数を探す
        return NULL;
    }
    bool is_array = variable->type->ty == TYPE_ARRAY;
    Node *node = calloc(1, sizeof(Node));
    node->kind = is_array ? ND_VARIABLE_ARRAY : ND_VARIABLE;
    node->type = variable->type;
    load_struct(node->type);
    node->is_local = true;
    node->name = str;
    node->len = len;
    node->offset = variable->offset;
    return node;
}

Node *new_node_dereference(Node *operand) {
    Type *type = find_type(operand);
    switch (type->ty) {
        case TYPE_VOID:
        case TYPE_CHAR:
        case TYPE_BOOL:
        case TYPE_INT:
        case TYPE_STRUCT:
        case TYPE_ENUM:
            return NULL;
        case TYPE_POINTER:
            if (type->point_to->ty == TYPE_ARRAY) {
                return new_node(ND_DEREF_ARRAY_POINTER, operand, NULL);
            }
        case TYPE_ARRAY:
            return new_node(ND_DEREF, operand, NULL);
    }
}

Node *new_node_function_call(const Token *tok) {
    // 存在チェック
    Declaration *declaration = find_function(tok->str, tok->len);
    if (!declaration) {
        error_at(tok->str, "関数が定義または宣言されていません");
        exit(1);
    }
    // 関数呼び出し
    Node *node = calloc(1, sizeof(Node));
    node->contextual_suffix = context++;
    node->kind = ND_FUNC;
    node->type = declaration->return_type;
    if (!consume(")")) {
        Node *last = node;
        Variable *declared_type = declaration->type_only;
        bool type_check = declared_type != NULL;
        do {
            char *const loc = token->str;
            last = last->args = expr();
            // 引数の型チェック
            if (type_check) {
                if (declared_type) {
                    assert_assignable(loc, declared_type->type, last);
                    declared_type = declared_type->next;
                } else {
                    // 宣言側の型が足りなかったら
                    error_at(loc, "引数の数が多いです");
                    exit(1);
                }
            }
        } while (consume(","));
        // 引数の型チェック
        if (type_check) {
            if (declared_type) {
                // 宣言側の型が余っていたら
                error_at(token->str, "引数の数が少ないです");
                exit(1);
            }
        }
        expect(")");
    }
    node->name = tok->str;
    node->len = tok->len;
    return node;
}

Node *pointer_calc(enum NodeKind kind, Node *left, Node *right) {
    const int weight_l = get_weight(left);
    const int weight_r = get_weight(right);
    if (weight_l == weight_r) {
        if (weight_l == 1) {
            // 整数の引き算または足し算
            return new_node(kind, left, right);
        } else {
            if (kind == ND_SUB) {
                // ポインタどうしの引き算
                Node *node = new_node(kind, left, right);
                return new_node(ND_DIV, node, new_node_num(weight_l));
            } else {
                error_at(token->str, "ポインタどうしの足し算はできません");
                exit(1);
            }
        }
    } else if (weight_l == 1) {
        left = new_node(ND_MUL, new_node_num(weight_r), left);
        return new_node(kind, left, right);
    } else if (weight_r == 1) {
        right = new_node(ND_MUL, new_node_num(weight_l), right);
        return new_node(kind, left, right);
    } else {
        error("異なるポインター型の演算はできません？\n");
        exit(1);
    }
}

Node *new_node_assign(char *loc, Node *const lhs, Node *const rhs) {
    Type *const left_type = find_type(lhs);
    assert_assignable(loc, left_type, rhs);
    return new_node(ND_ASSIGN, lhs, rhs);
}

Node *with_dot(Node *left, Token *const dot) {
    Type *type = find_type(left);
    if (type->ty != TYPE_STRUCT) {
        error_at(dot->str - 1, "構造体ではありません");
        exit(1);
    }
    Token *const m = consume_ident();
    if (m == NULL) {
        error_at(dot->str + 1, "構造体のメンバー名がありません。");
        exit(1);
    }
    Variable *const member = find_struct_member(type, m->str, m->len);
    Node *const offset = new_node_num(member->offset - member->type_size);
    left = new_node(ND_MEMBER, left, offset);
    left->type = member->type;
    left->name = member->name;
    left->len = member->len;
    return left;
}

Node *with_accessor(Node *left) {
    // [] access
    left = with_index(left);
    {
        Token *const dot = consume(".");
        if (dot) {
            Node *const dot_access = with_dot(left, dot);
            return with_accessor(dot_access);
        }
    }
    {
        Token *const arrow = consume("->");
        if (arrow) {
            left = new_node_dereference(left);
            Node *const dot_access = with_dot(left, arrow);
            return with_accessor(dot_access);
        }
    }
    return left;
}

//////////////////////////////////////////////////////////////////

struct Program *parse(Token *tok) {
    token = tok;
    init_globals();
    init_functions();
    init_struct_registry();

    // 関数
    Function head_f;
    head_f.next = NULL;
    Function *tail_f = &head_f;
    while (!at_eof()) {
        bool typedef_ = consume("typedef");
        bool extern_ = consume("extern");
        Type *base = consume_base_type();
        if (!base) {
            error_at(token->str, "関数または変数の型が正しく定義されていません");
            exit(1);
        }
        // 構造体の定義または宣言
        if (consume_struct_def_or_dec(base)) {
            if (typedef_) {
                consume_type_def(base);
            }
            expect(";");
            continue;
        }
        // enumの定義
        if (consume_enum_def(base)) {
            expect(";");
            continue;
        }
        if (typedef_) {
            consume_type_def(base);
            expect(";");
            continue;
        }
        Type *type;
        Token *identifier = consume_type(base, &type);
        if (!identifier) {
            error_at(token->str, "関数名または変数名がありません");
            exit(1);
        }
        // TODO グローバル変数では定義が後ろにあっても使える
        // 構造体の型データを取得
        load_struct(type);

        if (extern_) {
            expect(";");
            Global *g = calloc(1, sizeof(Global));
            g->type = type;
            g->label = identifier->str;
            g->label_length = identifier->len;
            add_globals(g);
            continue;
        }

        if (consume("(")) {
            // 関数(宣言の場合はNULLが返ってくる)
            function_name = identifier;
            Function *func = function(type);
            if (func) {
                tail_f = tail_f->next = func;
            }
            function_name = NULL;
        } else {
            // グローバル変数
            global_variable_declaration(identifier, type);
        }
    }

    token = NULL;

    struct Program *prog = calloc(1, sizeof(struct Program));
    prog->functions = head_f.next;
    prog->globals = get_globals();
    return prog;
}

void consume_function_parameter() {
    /*
     * int function_name(
     *                   ↑ここから
     */
    int i = 0;
    Type *param_type;
    bool expect_just_declare = false;
    while ((param_type = consume_base_type())) {
        while (consume("*")) {
            param_type = create_pointer_type(param_type);
        }
        // void
        // voidポインタは先へ進む
        if (param_type->ty == TYPE_VOID) {
            if (i == 0) {
                void_arg(token->str);
                break;
            }
            error_at(token->str, "voidとその他の引数は共存できません");
            exit(1);
        }
        Token *t = consume_ident();
        if (t) {
            if (find_local_variable(current_scope, t->str, t->len)) {
                error_at(t->str, "引数名が重複しています");
                exit(1);
            }
        } else {
            // 宣言なら名前はいらない
            expect_just_declare = true;
        }
        char *name = t ? t->str : NULL;
        int len = t ? /** (int) */ t->len : 0;
        Variable *param = register_variable(name, len, param_type);
        param->index = i++;
        if (!consume(",")) {
            break;
        }
    }
    expect(")");

    // 引数名が無かったので関数宣言とみなす
    if (expect_just_declare) {
        Token *const semi_colon = consume(";");
        if (!semi_colon) {
            // 実際には関数定義だった場合
            error_at(token->str, "関数定義の引数に名前がありません");
            exit(1);
        }
        token = semi_colon;
    }
}

NodeArray *function_body() {
    expect("{");
    NodeArray *nodeArray = create_node_array(10);
    while (!consume("}")) {
        push_node(nodeArray, stmt());
    }
    return nodeArray;
}

Function *function(Type *return_type) {
    // 他の関数との重複チェックは、宣言か定義かを確認する必要があるためもう少し先で
    // 他のグローバル変数や関数との名前重複チェック
    if (find_global_variable_by_name(function_name->str, function_name->len)) {
        error_at(function_name->str, "関数の名前がグローバル変数と重複しています");
        exit(1);
    }
    if (return_type->ty == TYPE_ARRAY) {
        error_at(token->str, "関数の返り値に配列は使えません");
        exit(1);
    }
    // ラベルは関数単位
    init_goto_label();

    Scope *const parameter = current_scope = calloc(1, sizeof(Scope));
    consume_function_parameter();

    const bool just_declare = consume(";");
    // 再帰呼び出しがあるため、先に関数宣言として追加
    add_function_declaration(return_type, function_name, parameter->variables, !just_declare);
    if (just_declare) {
        // 関数宣言
        current_scope = NULL;
        stack_size = 0;
        return NULL;
    }

    // 仮引数以外は不要、かつblockスコープのものは捨てるので、他のローカル変数も捨てることにする
    Scope func_body;
    func_body.variables = NULL;
    func_body.parent = parameter;
    // 関数スコープ
    current_scope = &func_body;

    Function *function = calloc(1, sizeof(Function));
    function->name = copy(function_name->str, function_name->len);
    function->parameters = parameter->variables;
    function->body = function_body();
    // 関数本体を読んでからスタックサイズを決定する
    function->stack_size = stack_size;

    current_scope = NULL;
    function_name = NULL;
    stack_size = 0;
    // goto先のラベルが存在するか確認する
    assert_goto_label();

    return function;
}

Node *block_statement(void) {
    // 使い捨てるのでallocしない
    Scope disposable;
    // blockスコープ
    disposable.variables = NULL;
    disposable.parent = current_scope;
    current_scope = &disposable;

    Node *node;
    {
        node = calloc(1, sizeof(Node));
        node->kind = ND_BLOCK;
        Node *last = node;
        while (!consume("}")) {
            Node *next = stmt();
            last->statement = next;
            last = next;
        }
    }

    current_scope = disposable.parent;
    return node;
}

Node *local_variable_declaration(Type *base) {
    // 変数の登録
    Type *type;
    Token *const identifier = consume_type(base, &type);
    if (type->ty == TYPE_VOID) {
        error_at(token->str, "変数にvoidは使えません");
        exit(1);
    }
    if (!identifier) {
        error_at(token->str, "変数名がありません");
        exit(1);
    }
    if (base->ty == TYPE_STRUCT && base->struct_info->members == NULL) {
        // ポインタや配列は、構造体の型が必要になったときに存在しないとエラーになるはず
        error_at(token->str, "構造体の定義がありません");
        exit(1);
    }
    Variable *variable = register_variable(identifier->str, identifier->len, type);
    // 初期化
    if (consume("=")) {
        // TODO 構造体の初期化 https://ja.cppreference.com/w/c/language/struct_initialization
        // RBPへのオフセットが決定しない場合がある
        const bool implicit_size = type->array_size == 0;
        Node *const variable_node = new_node_local_variable(variable->name, variable->len);
        if (type->ty == TYPE_ARRAY) {
            // 配列の初期化
            Node *node = array_initializer(variable_node, type);
            if (implicit_size) {
                // 配列のサイズが決まってからオフセットを再設定する
                variable->type_size = get_size(type);
                variable->offset = stack_size = stack_size + variable->type_size;
                variable_node->offset = variable->offset;
            }
            expect(";");
            return node;
        } else {
            if (type->ty == TYPE_POINTER) {
                // TODO サイズ不一致の場合はwarningを出す
                //  int (*pointer)[] = &array;
                //  配列の場合はarray_initializer関数で実施済み
            }
            char *loc = token->str;
            Node *node = new_node_assign(loc, variable_node, assign());
            expect(";");
            return node;
        }
    } else {
        Node *node = new_node(ND_NOTHING, NULL, NULL);
        expect(";");
        return node;
    }
}

Node *stmt(void) {
    {
        // label:
        Token *label = consume_ident();
        if (label != NULL) {
            if (consume(":")) {
                return new_node_label(function_name, label);
            }
            token = label;
        }
    }
    if (consume("goto")) {
        Token *const label = consume_ident();
        if (label == NULL) {
            error_at(token->str, "goto先のラベル名がありません。");
            exit(1);
        }
        Node *const node = new_node_goto(function_name, label);
        expect(";");
        return node;
    }
    Type *const base = consume_base_type();
    if (base) {
        // 変数宣言および初期化
        Node *const node = local_variable_declaration(base);
        if (node->kind == ND_ASSIGN) {
            return new_node(ND_EXPR_STMT, node, NULL);
        } else {
            return node;
        }
    } else if (consume("{")) {
        return block_statement();
    } else if (consume("if")) {
        Node *const node = calloc(1, sizeof(Node));
        node->contextual_suffix = context++;
        node->kind = ND_IF;
        expect("(");
        node->condition = expr();
        expect(")");
        node->lhs = stmt();
        node->rhs = consume("else") ? stmt() : NULL;
        return node;
    } else if (consume("do")) {
        Node *const node = calloc(1, sizeof(Node));
        node->contextual_suffix = context++;
        // break先をスタックに積む
        push_int(&break_stack, node->contextual_suffix);
        // continue先をスタックに積む
        push_int(&continue_stack, node->contextual_suffix);
        node->kind = ND_DO_WHILE;
        // 処理
        node->lhs = stmt();
        expect("while");
        // 条件
        expect("(");
        node->condition = expr();
        expect(")");
        expect(";");
        // break先をスタックから降ろす
        pop_int(&break_stack);
        // continue先をスタックから降ろす
        pop_int(&continue_stack);
        return node;
    } else if (consume("while")) {
        Node *const node = calloc(1, sizeof(Node));
        node->contextual_suffix = context++;
        // break先をスタックに積む
        push_int(&break_stack, node->contextual_suffix);
        // continue先をスタックに積む
        push_int(&continue_stack, node->contextual_suffix);
        node->kind = ND_WHILE;
        // 条件
        expect("(");
        node->condition = expr();
        expect(")");
        // 処理
        node->lhs = stmt();
        // break先をスタックから降ろす
        pop_int(&break_stack);
        // continue先をスタックから降ろす
        pop_int(&continue_stack);
        return node;
    } else if (consume("for")) {
        Node *const node = calloc(1, sizeof(Node));
        node->contextual_suffix = context++;
        // break先をスタックに積む
        push_int(&break_stack, node->contextual_suffix);
        // continue先をスタックに積む
        push_int(&continue_stack, node->contextual_suffix);
        node->kind = ND_FOR;
        expect("(");
        // 使い捨てるのでallocしない
        Scope disposable;
        // forスコープ
        disposable.variables = NULL;
        disposable.parent = current_scope;
        current_scope = &disposable;
        // init
        if (!consume(";")) {
            Node *init;
            Type *init_type_base = consume_base_type();
            if (init_type_base) {
                // 変数宣言および初期化
                init = local_variable_declaration(init_type_base);
                node->lhs = new_node(ND_EXPR_STMT, init, NULL);
            } else {
                init = expr();
                node->lhs = new_node(ND_EXPR_STMT, init, NULL);
                expect(";");
            }
        }
        // condition
        if (!consume(";")) {
            node->condition = expr();
            expect(";");
        }
        // increment
        if (!consume(")")) {
            node->rhs = new_node(ND_EXPR_STMT, expr(), NULL);
            expect(")");
        }
        node->execution = stmt();
        // break先をスタックから降ろす
        pop_int(&break_stack);
        // continue先をスタックから降ろす
        pop_int(&continue_stack);
        // スコープを戻す
        current_scope = disposable.parent;
        return node;
    } else if (consume("switch")) {
        Node *const node = calloc(1, sizeof(Node));
        node->contextual_suffix = context++;
        // break先をスタックに積む
        push_int(&break_stack, node->contextual_suffix);
        node->kind = ND_SWITCH;
        expect("(");
        node->lhs = expr();
        expect(")");
        expect("{");
        node->cases = consume_case();
        expect("}");
        // break先をスタックから降ろす
        pop_int(&break_stack);
        return node;
    } else if (consume("goto")) {
        Node *const node = calloc(1, sizeof(Node));
        node->kind = ND_GOTO;
        // ラベルは関数内にあれば良い　TODO どうやって確認するか
        Token *const label = consume_ident();
        if (label == NULL) {
            error_at(token->str, "goto先のラベル名がありません。");
            exit(1);
        }
        expect(";");
        node->label = label->str;
        node->label_length = label->len;
        // 所属する関数名
        node->name = function_name->str;
        node->len = function_name->len;
        return node;
    } else if (consume("return")) {
        Node *const node = calloc(1, sizeof(Node));
        node->kind = ND_RETURN;
        node->type = find_function(function_name->str, function_name->len)->return_type;
        if (node->type->ty != TYPE_VOID) {
            // void以外の場合
            node->lhs = expr();
        }
        // returnする関数名
        node->name = function_name->str;
        node->len = function_name->len;
        expect(";");
        return node;
    } else if (consume("break")) {
        if (break_stack == NULL) {
            error_at(token->str, "ここではbreakできません");
            exit(1);
        }
        Node *const node = calloc(1, sizeof(Node));
        node->contextual_suffix = peek_int(&break_stack);
        node->kind = ND_BREAK;
        expect(";");
        return node;
    } else if (consume("continue")) {
        if (continue_stack == NULL) {
            error_at(token->str, "ここではcontinueできません");
            exit(1);
        }
        Node *const node = calloc(1, sizeof(Node));
        node->contextual_suffix = peek_int(&break_stack);
        node->kind = ND_CONTINUE;
        expect(";");
        return node;
    } else {
        Node *const node = new_node(ND_EXPR_STMT, expr(), NULL); // 式文
        expect(";");
        return node;
    }
}

Node *expr() {
    return assign();
}

Node *assign(void) {
    Node *node = ternary();
    if (consume("=")) {
        char *loc = token->str;
        node = new_node_assign(loc, node, assign());
    }
    return node;
}

Node *ternary(void) {
    Node *node = logical();
    if (consume("?")) {
        Node *const ternary = calloc(1, sizeof(Node));
        ternary->contextual_suffix = context++;
        ternary->kind = ND_IF;
        ternary->condition = node;
        ternary->lhs = expr();
        char *const loc = token->str;
        expect(":");
        ternary->rhs = expr();
        if (ternary->lhs->kind == ND_NUM && ternary->lhs->val == 0) {
            // 左が0で右がポインタの場合に備える
            // 型チェック
            Type *const type = find_type(ternary->rhs);
            assert_assignable(loc, type, ternary->lhs);
            ternary->type = type;
        } else {
            // 型チェック
            Type *const type = find_type(ternary->lhs);
            assert_assignable(loc, type, ternary->rhs);
            ternary->type = type;
        }
        return ternary;
    }
    return node;
}

Node *logical(void) {
    Node *node = equality();
    for (;;) {
        if (consume("||")) {
            node = new_node(ND_LOGICAL_OR, node, equality());
            node->contextual_suffix = context++;
        } else if (consume("&&")) {
            node = new_node(ND_LOGICAL_AND, node, equality());
            node->contextual_suffix = context++;
        } else {
            return node;
        }
    }
}

Node *equality(void) {
    Node *node = relational();

    for (;;) {
        if (consume("=="))
            node = new_node(ND_EQL, node, relational());
        else if (consume("!="))
            node = new_node(ND_NOT, node, relational());
        else
            return node;
    }
}

Node *relational() {
    Node *node = add();
    for (;;) {
        if (consume("<"))
            node = new_node(ND_LESS, node, add());
        else if (consume(">"))
            node = new_node(ND_LESS, add(), node);
        else if (consume("<="))
            node = new_node(ND_LESS_EQL, node, add());
        else if (consume(">="))
            node = new_node(ND_LESS_EQL, add(), node);
        else
            return node;
    }
}

Node *add() {
    Node *node = mul();
    for (;;) {
        if (consume("+")) {
            node = pointer_calc(ND_ADD, node, mul());
        } else if (consume("-"))
            // ポインタ同士の引き算はサイズで割る
            node = pointer_calc(ND_SUB, node, mul());
        else
            return node;
    }
}

Node *mul() {
    Node *node = unary();
    for (;;) {
        if (consume("*"))
            node = new_node(ND_MUL, node, unary());
        else if (consume("/"))
            node = new_node(ND_DIV, node, unary());
        else if (consume("%"))
            node = new_node(ND_MOD, node, unary());
        else
            return node;
    }
}

Node *unary() {
    if (consume("sizeof")) {
        Type *type;
        Token *kakko = consume("(");
        if (kakko) {
            Type *const base = consume_base_type();
            if (base) {
                // sizeof(int)など
                Token *const identifier_token = consume_type(base, &type);
                if (identifier_token) {
                    error_at(identifier_token->str, "ここでは変数を宣言できません");
                    exit(1);
                }
                expect(")");
                int size = get_size(type);
                return new_node_num(size);
            } else {
                token = kakko;
                type = find_type(unary());
            }
        } else {
            type = find_type(unary());
        }
        int size = get_size(type);
        return new_node_num(size);
    } else if (consume("+")) {
        return primary();
    } else if (consume("++")) {
        char *const loc = token->str;
        Node *const node = primary();
        Node *const block = calloc(1, sizeof(Node));
        block->kind = ND_BLOCK;
        block->incr = PRE;
        // インクリメントする計算
        Node *const incremented = pointer_calc(ND_ADD, node, new_node_num(1));
        // 変数に代入
        Node *const assign = new_node_assign(loc, node, incremented);
        // 代入式の値をスタックに残さない
        block->statement = new_node(ND_EXPR_STMT, assign, NULL);
        // スタックに現在の値を積む
        block->statement->statement = node;
        return block;
    } else if (consume("-")) {
        return new_node(ND_SUB, new_node_num(0), primary());
    } else if (consume("*")) {
        int dereference_count = 1;
        while (consume("*")) {
            dereference_count++;
        }
        char *loc = token->str;
        Node *operand = primary();
        for (; 0 < dereference_count; dereference_count--) {
            operand = new_node_dereference(operand);
            if (!operand) {
                error_at(loc, "ポインタ型ではありません\n");
                exit(1);
            }
        }
        return operand;
    } else if (consume("&")) {
        Node *operand = primary();
        return new_node(ND_ADDRESS, operand, NULL);
    } else if (consume("!")) {
        // TODO 非ポインターの構造体は使えない。他は不明
        int count = 1;
        while (consume("!")) {
            count++;
        }
        Node *node = primary();
        for (int i = 0; i < count; ++i) {
            node = new_node(ND_INVERT, node, NULL);
        }
        return node;
    } else {
        return primary();
    }
}

Node *primary() {
    // 文字列リテラル
    if (token->kind == TK_STR_LITERAL) {
        return new_node_string_literal();
    }

    Token *tok = consume_ident();
    if (tok) {
        if (consume("(")) {
            // 関数呼び出し
            return new_node_function_call(tok);
        } else {
            // 変数アクセス
            Node *variable = new_node_local_variable(tok->str, tok->len);
            if (!variable) {
                variable = new_node_global_variable(tok->str, tok->len);
            }
            variable = with_accessor(variable);
            if (consume("++")) {
                Node *const block = calloc(1, sizeof(Node));
                block->kind = ND_BLOCK;
                block->incr = POST;
                // スタックに現在の値を積む
                block->statement = variable;
                // インクリメントする計算
                Node *const incremented = pointer_calc(ND_ADD, variable, new_node_num(1));
                // 変数に代入
                Node *const assign = new_node_assign(tok->str, variable, incremented);
                // 代入式の値をスタックに残さない
                variable->statement = new_node(ND_EXPR_STMT, assign, NULL);
                return block;
            }
            return variable;
        }
    }

    // 次のトークンが"("なら、"(" expr ")" または "({" stmt "})" または (a[0])[1], (*b)[1]
    if (consume("(")) {
        if (consume("{")) {
            Node *node = block_statement();
            Node *last;
            Node *last_before = node;
            for (last = node->statement;;) {
                if (last->statement) {
                    last_before = last;
                    last = last->statement;
                } else {
                    break;
                }
            }
            if (last->kind == ND_EXPR_STMT) {
                // 式文扱いを取り消して、値がスタックに積まれるようにする
                last_before->statement = last->lhs;
            }
            expect(")");
            return node;
        }
        Node *node = expr();
        expect(")");
        return with_accessor(node);
    }

    // そうでなければ数値か文字リテラルのはず
    Node *number = new_node_num(expect_num_char());
    return with_accessor(number);
}
