#include "common.h"

// 現在着目しているトークン
Token *token;

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

/////////////////////////

NodeArray *create_node_array(int capacity) {
    if (capacity < 1) {
        capacity = 10;
    }
    NodeArray *array = malloc(sizeof(NodeArray));
    array->memory = malloc(sizeof(Node *) * capacity);
    array->count = 0;
    array->capacity = capacity;
    return array;
}

NodeArray *push_node(NodeArray *array, Node *node) {
    if (array->count == array->capacity) {
        array->memory = realloc(array->memory, sizeof(Node *) * array->capacity * 2);
        array->capacity *= 2;
    }
    array->memory[array->count] = node;
    array->count++;
    return array;
}

/////////////////////////

// program    = (function | global_var)*
// global_var = (decl_b | decl_c) ";"
// function   = decl_a "(" decl_a? ("," decl_a)* ")" { stmt* }
// stmt       = expr ";"
//              | (decl_b | decl_c) ";"
//				| "{" stmt* "}"
//				| "return" expr ";"
//				| "if" "(" expr ")" stmt ("else" stmt)?
//		        | "while" "(" expr ")" stmt
//				| "for" "(" (expr | decl_b | decl_c)? ";" expr? ";" expr? ")" stmt
// expr       = assign
// assign     = equality ("=" assign)?
// equality   = relational ("==" relational | "!=" relational)*
// relational = add ("<" add | "<=" add | ">" add | ">=" add)*
// add        = mul ("+" mul | "-" mul)*
// mul        = unary ("*" unary | "/" unary)*
// unary      = "sizeof" unary
//				| ("+" | "-" | "*"* | "&" )? primary
// primary    = literal_str
//				| ident "(" args? ")"
// 				| "(" expr ")"
// 				| "({" stmt "})"
//				| (primary | num_char) ( "(" index ")" )? index*
//				| primary ("." ident )*
// index      = "[" primary "]"
// args       = expr ("," expr)*
// decl_a     = ("int" | "char" | "struct") "*"* (pointed_id | ident)
// decl_b     = decl_a ("[" num_char "]")*
// decl_c     = decl_a "[]"? ("[" num_char? "]")* "=" (array_init | expr)
// pointed_id = "(" "*"* ident ")"
// ident      =
// literal_str
// num_char

void global_variable_declaration(Token *variable_name, Type *type);

Function *function(Token *function_name, Type *return_type);

Node *stmt(void);

Node *expr(void);

Node *assign(void);

Node *equality(void);

Node *relational(void);

Node *add(void);

Node *mul(void);

Node *unary(void);

Node *primary(void);

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
    if (consume("int")) {
        return shared_int_type();
    } else if (consume("char")) {
        return shared_char_type();
    } else if (consume("void")) {
        return shared_void_type();
    } else if (consume("struct")) {
        Type *const base = create_struct_type();
        Token *const t = consume_ident();
        if (!t) {
            error_at(token->str, "構造体の名前がありません");
            exit(1);
        }
        STRUCT_INFO *structInfo = malloc(sizeof(STRUCT_INFO));
        structInfo->type_name = t->str;
        structInfo->name_length = t->len;
        structInfo->members = NULL;
        base->struct_info = structInfo;
        // 定義済みの型情報があれば読み込む
        load_struct(base);
        return base;
    } else {
        return NULL;
    }
}

Token *consume_type(Type *base, Type **r_type);

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

void assert_indexable(Node *left, Node *right) {
    // 片方がintで、もう片方が配列orポインタ
    Type *left_type = find_type(left);
    Type *right_type = find_type(right);
    switch (left_type->ty) {
        case TYPE_VOID:
            error("voidで[]は使えません？\n");
            exit(1);
        case TYPE_CHAR:
        case TYPE_INT: {
            // 左がint
            switch (right_type->ty) {
                case TYPE_VOID:
                    error("voidで[]は使えません？\n");
                    exit(1);
                case TYPE_CHAR:
                case TYPE_INT:
                    error("整数間で[]は使えません？\n");
                    exit(1);
                case TYPE_POINTER:
                case TYPE_ARRAY:
                    break;
                case TYPE_STRUCT:
                    // TODO
                    error("[parse]構造体実装中\n");
                    exit(1);
            }
            break;
        }
        case TYPE_POINTER:
        case TYPE_ARRAY: {
            // 左が配列orポインタ
            switch (right_type->ty) {
                case TYPE_VOID:
                    error("voidで[]は使えません？\n");
                    exit(1);
                case TYPE_CHAR:
                case TYPE_INT:
                    break;
                case TYPE_POINTER:
                case TYPE_ARRAY:
                    error("not_int[not_int]はできません？");
                    exit(1);
                case TYPE_STRUCT:
                    // TODO
                    error("[parse]構造体実装中\n");
                    exit(1);
            }
            break;
        }
        case TYPE_STRUCT:
            // TODO
            error("[parse]構造体実装中\n");
            exit(1);
    }
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

Node *new_node(NodeKind kind, Node *lhs, Node *rhs) {
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

Node *new_node_global_variable(char *str, int len) {
    Global *variable = find_global_variable(str, len);
    if (!variable) {
        error_at(str, "変数の宣言がありません。");
        exit(1);
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

Node *new_node_string_literal() {
    Global *g = get_string_literal(token->str, token->len);
    // ラベルを指すnodeを作る
    Node *const node = new_node(ND_STR_LITERAL, NULL, NULL);
    node->label = g->label;
    node->label_length = g->label_length;
    token = token->next;
    return node;
}

Node *new_node_dereference(Node *operand) {
    Type *type = find_type(operand);
    switch (type->ty) {
        case TYPE_VOID:
        case TYPE_CHAR:
        case TYPE_INT:
            return NULL;
        case TYPE_POINTER:
            if (type->point_to->ty == TYPE_ARRAY) {
                return new_node(ND_DEREF_ARRAY_POINTER, operand, NULL);
            }
        case TYPE_ARRAY:
            return new_node(ND_DEREF, operand, NULL);
        case TYPE_STRUCT:
            // TODO
            error("[parse]構造体実装中\n");
            exit(1);
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

Node *pointer_calc(NodeKind kind, Node *left, Node *right) {
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

Node *new_node_array_index(Node *const left, Node *const right, const NodeKind kind) {
    // int a[2];
    // a[1] = 3;
    // は
    // *(a + 1) = 3;
    // の省略表現
    assert_indexable(left, right);
    Node *pointer = pointer_calc(ND_ADD, left, right);
    return new_node(kind, pointer, NULL);
}

Node *with_index(Node *left) {
    bool consumed = false;
    while (consume("[")) {
        consumed = true;
        left = new_node_array_index(left, expr(), ND_DEREF_ARRAY_POINTER);
        expect("]");
    }
    Type *type = find_type(left);
    if (consumed && type->ty != TYPE_ARRAY) {
        // 配列の指すところまでアクセスしたらポインタの値をデリファレンスする
        left->kind = ND_DEREF;
    }
    return left;
}

Node *with_accessor(Node *left) {
    // [] access
    left = with_index(left);
    // . access
    Token *const dot = consume(".");
    if (dot) {
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
        Variable *const member = struct_member(type, m->str, m->len);
        Node *const offset = new_node_num(member->offset - member->type_size);
        left = new_node(ND_MEMBER, left, offset);
        left->type = member->type;
        left->name = member->name;
        left->len = member->len;
        return with_accessor(left);
    } else {
        return left;
    }
}

Node *array_initializer(Node *array_variable, Type *type);

//////////////////////////////////////////////////////////////////

Variable *consume_member() {
    Type *base = consume_base_type();
    if (!base) {
        error_at(token->str, "構造体のメンバーが正しく定義されていません");
        exit(1);
    }
    if (base->ty == TYPE_VOID) {
        error_at(token->str, "構造体のメンバーにvoidは使えません");
        exit(1);
    }
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

            expect(";");
            push_struct(base->struct_info);
            return true;
        }
        // 構造体の宣言
        if (consume(";")) {
            push_struct(base->struct_info);
            return true;
        }
    }
    return false;
}

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
        Type *base = consume_base_type();
        if (!base) {
            error_at(token->str, "関数の型が正しく定義されていません");
            exit(1);
        }
        // 構造体の定義または宣言
        if (consume_struct_def_or_dec(base)) {
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
        if (consume("(")) {
            // 関数(宣言の場合はNULLが返ってくる)
            Function *func = function(identifier, type);
            if (func) {
                tail_f = tail_f->next = func;
            }
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

void global_variable_declaration(Token *variable_name, Type *type) {
    // 他のグローバル変数や関数との名前重複チェック
    if (find_global_variable(variable_name->str, variable_name->len)) {
        // TODO 同名で同型のグローバル変数は宣言できる
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

void consume_function_parameter() {
    /*
     * int function_name(
     *                   ↑ここから
     */
    int i = 0;
    Type *param_type;
    bool expect_just_declare = false;
    while ((param_type = consume_base_type())) {
        // void
        if (param_type->ty == TYPE_VOID) {
            if (i == 0) {
                void_arg(token->str);
                break;
            }
            error_at(token->str, "voidとその他の引数は共存できません");
            exit(1);
        }
        // TODO 複数
        if (consume("*")) {
            param_type = create_pointer_type(param_type);
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
        int len = t ? (int) t->len : 0;
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

Function *function(Token *function_name, Type *return_type) {
    // 他の関数との重複チェックは、宣言か定義かを確認する必要があるためもう少し先で
    // 他のグローバル変数や関数との名前重複チェック
    if (find_global_variable(function_name->str, function_name->len)) {
        error("関数の名前が");
        error_at(function_name->str, "グローバル変数と重複しています");
        exit(1);
    }
    if (return_type->ty == TYPE_ARRAY) {
        error_at(token->str, "関数の返り値に配列は使えません");
        exit(1);
    }

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
    stack_size = 0;

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

void visit_array_initializer(NodeArray *array, Type *type) {
    // 配列サイズが明示されていない場合
    const bool implicit_size = type->array_size == 0;
    /*
     * char charara[2][4] = {"abc", "def"};
     *
     * 0: pointer  (1 * 8 byte)
     * 1: pointer
     *
     * ↑ではなく↓
     *
     * 0: char char char (3 * 1 byte)
     * 1: char char char
     *
     * ↓ x86-64 gcc 9.1以降（これに近い方式を採用する）
     *
     *   sub rsp, 16
     *   mov WORD PTR [rbp-6], 25185
     *   mov BYTE PTR [rbp-4], 99
     *   mov WORD PTR [rbp-3], 25956
     *   mov BYTE PTR [rbp-1], 102
     *
     * ↓ x86-64 gcc 4.7.4以前
     *
     *   sub rsp, 16
     *   mov WORD PTR [rbp-16], 25185
     *   mov BYTE PTR [rbp-14], 99
     *   mov WORD PTR [rbp-13], 25956
     *   mov BYTE PTR [rbp-11], 102
     *
     * ↓ その他のgcc
     *
     * .LC0:
     *   .ascii "abc"
     *   .ascii "def"
     * ---------------
     *   sub rsp, 16
     *   mov eax, DWORD PTR .LC0[rip]
     *   mov DWORD PTR [rbp-6], eax
     *   movzx eax, WORD PTR .LC0[rip+4]
     *   mov WORD PTR [rbp-2], ax
     *
     */
    if (consume("{")) {
        if (type->ty != TYPE_ARRAY) {
            error_at(token->str, "配列の初期化式ではありません？");
            exit(1);
        }
        int index = 0;
        do {
            if (!implicit_size && type->array_size <= index) {
                warn_at(token->str, "excess elements in array initializer");
                token = token->next;
            } else {
                visit_array_initializer(array, type->point_to);
            }
            consume(","); // 末尾に残ってもOK
            index++;
        } while (!consume("}"));
        if (implicit_size) {
            // サイズを明示していない配列のサイズを決定する
            type->array_size = index;
        }
        // 初期化式の要素数が配列のサイズより小さい場合、0で埋める。
        while (index < type->array_size) {
            push_node(array, new_node_num(0));
            index++;
        }
    } else if (token->kind == TK_STR_LITERAL && type->ty == TYPE_ARRAY) {
        int index = 0;
        for (; index < token->len; index++) {
            if (!implicit_size && type->array_size <= index) {
                warn_at(token->str, "initializer-string for array of chars is too long");
                break;
            }
            char c = token->str[index];
            Node *const node = new_node_num(c);
            push_node(array, node);
        }
        if (implicit_size) {
            index++;
            // 末尾を\0
            Node *const zero = new_node_num(0);
            push_node(array, zero);
            // サイズを明示していない配列のサイズを決定する
            type->array_size = index;
        }
        // 初期化式の要素数が配列のサイズより小さい場合、0で埋める。
        while (index < type->array_size) {
            push_node(array, new_node_num(0));
            index++;
        }
        token = token->next;
    } else {
        Node *const node = expr();
        push_node(array, node);
    }
}

/*
 * 配列の初期化時のみ可能な式がいくつか
 * int array[4] = {0, 1, 2, 3};
 * int array[4] = {0, 1, 2};
 * int array[] = {0, 1, 2, 3};
 * char msg[4] = {'f', 'o', 'o', '_'}; => 警告なし
 * char msg[4] = {'f', 'o', 'o', '\0'};
 * char msg[] = {'f', 'o', 'o', '\0'};
 * char *s1[2] = {"abc", "def"};
 * char *s1[] = {"abc", "def"};
 * char msg[] = "foo";
 * char msg[10] = "foo";
 * char msg[3] = "message"; => initializer-string for array of chars is too long
 * char s1[2][3] = {"abc", "def"};
 * char s1[][3] = {"abc", "def"};
 * int array[3][2] = {{3, 3}, {3, 3}, {3, 3}};
 * int array[][2] = {{3, 3}, {3, 3}, {3, 3}};
 *
 * C99にはさらに色々ある
 * https://kaworu.jpn.org/c/%E9%85%8D%E5%88%97%E3%81%AE%E5%88%9D%E6%9C%9F%E5%8C%96_%E6%8C%87%E7%A4%BA%E4%BB%98%E3%81%8D%E3%81%AE%E5%88%9D%E6%9C%9F%E5%8C%96%E6%8C%87%E5%AE%9A%E5%AD%90
 */
Node *array_initializer(Node *const array_variable, Type *const type) {
    char *loc = token->str;
    int capacity = get_element_count(type);
    NodeArray *nodeArray = create_node_array(capacity);
    visit_array_initializer(nodeArray, type);

    // ポインター型への変換
    Type *pointed = type->point_to;
    while (pointed->ty == TYPE_ARRAY) {
        pointed = pointed->point_to;
    }
    array_variable->type = create_pointer_type(pointed);
    Node *pointer = array_variable;

    /*
      int x[] = {1, 2, foo()};
      ↓ ↓　↓　↓
      ブロックでまとめる
      ↓ ↓　↓　↓
      int x[3];
      {
        x[0] = 1;
        x[1] = 2;
        x[2] = foo();
      }
     */
    Node *const block = calloc(1, sizeof(Node));
    block->kind = ND_BLOCK;
    Node *last = block;
    for (int i = 0; i < nodeArray->count; ++i) {
        Node *n = nodeArray->memory[i];
        Node *const indexed = new_node_array_index(pointer, new_node_num(i), ND_DEREF);
        last = last->statement = new_node_assign(loc, indexed, n);
    }
    return block;
}

Token *consume_type(Type *base, Type **r_type) {
    // TODO 構造体の宣言、定義は、ローカルでも可能で
    //  定義なしに宣言しただけでもポインタなら変数宣言が可能（たぶんサイズが決まるから）
    //  グローバルならどこかで定義されていれば利用可能（対応は後回しの予定）
    // TODO
    //  本来この関数は、型か変数宣言を読むもの
    //  構造体は、さらに型定義と型宣言のパターンがある
    //  しかし、そうすると、型定義が存在するかどうかはどこでやる？
    //  他の型は、この関数が呼ぶ前後で、型の存在は前提している
    //  型の定義や宣言ができない場所からも呼ばれる
    /**
     * TODO どこから呼ばれてるか
     * 1. グローバル（ファイル）スコープ
     * 2. ローカルでの変数宣言および初期化 local_variable_declaration
     *   2.1. 関数スコープまたはブロックスコープ
     *   2.2. forスコープ
     * 3. sizeof
     * TODO 何ができるか
     * 型利用（変数宣言、関数宣言、関数定義、サイズ）
     * 1. 型定義　型宣言 型利用
     * 2.1. 型定義　型宣言 型利用
     * 2.2. 型利用
     * 3. 型利用（identifierなし）
     *
     * TODO とりあえずグローバルから？
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

Node *local_variable_declaration(Type *base) {
    if (base->ty == TYPE_VOID) {
        error_at(token->str, "変数にvoidは使えません");
        exit(1);
    }
    // 変数の登録
    Type *type;
    Token *const identifier = consume_type(base, &type);
    if (!identifier) {
        error_at(token->str, "変数名がありません");
        exit(1);
    }
    if (base->ty == TYPE_STRUCT && type->struct_info->members == NULL) {
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
    Type *base = consume_base_type();
    if (base) {
        // 変数宣言および初期化
        return local_variable_declaration(base);
    } else if (consume("{")) {
        return block_statement();
    } else if (consume("if")) {
        Node *const node = calloc(1, sizeof(Node));
        node->kind = ND_IF;
        expect("(");
        node->condition = expr();
        expect(")");
        node->lhs = stmt();
        node->rhs = consume("else") ? stmt() : NULL;
        return node;
    } else if (consume("while")) {
        Node *const node = calloc(1, sizeof(Node));
        node->kind = ND_WHILE;
        expect("(");
        node->condition = expr();
        expect(")");
        node->lhs = stmt();
        return node;
    } else if (consume("for")) {
        Node *const node = calloc(1, sizeof(Node));
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
        // スコープを戻す
        current_scope = disposable.parent;
        return node;
    } else if (consume("return")) {
        Node *const node = calloc(1, sizeof(Node));
        node->kind = ND_RETURN;
        node->lhs = expr();
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

Node *assign() {
    Node *node = equality();
    if (consume("=")) {
        char *loc = token->str;
        node = new_node_assign(loc, node, assign());
    }
    return node;
}

Node *equality() {
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
            // 変数
            Node *variable = new_node_local_variable(tok->str, tok->len);
            if (!variable) {
                variable = new_node_global_variable(tok->str, tok->len);
            }
            return with_accessor(variable);
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
