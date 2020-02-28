#include "common.h"

// 現在着目しているトークン
Token *token;

/////////////////////////

int stack_size = 0;

typedef struct Scope Scope;

struct Scope {
    Variable *variables;
    Scope *parent;
};

// 現在のスコープ
Scope *current_scope;

/////////////////////////

typedef struct Declaration Declaration;

struct Declaration {
    // 返り値の型
    Type *return_type;
    // 関数名
    char *name;
    int len;
    // 引数
    // Variable *parameters;

    Declaration *next;
};

struct Declaration_C {
    Declaration *head;
    Declaration *tail;
};
// 関数宣言
struct Declaration_C *declarations;

void add_function_declaration(Declaration *next) {
    if (!declarations->head) {
        declarations->head = next;
        declarations->tail = next;
        return;
    }
    declarations->tail = declarations->tail->next = next;
}

/////////////////////////

struct Global_C {
    Global *head;
    Global *tail;
};
// グローバル変数その他
struct Global_C *globals;

void add_globals(Global *next) {
    if (!globals->head) {
        globals->head = next;
        globals->tail = next;
        return;
    }
    globals->tail = globals->tail->next = next;
}

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
// global_var = decl_b ";"
// function   = decl_a "(" decl_a? ("," decl_a)* ")" { stmt* }
// stmt       = expr ";"
//              | decl_b ";"
//              | decl_c ";"
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
// primary    = num
// 				| literal_str
//				| ident
//				| primary ( "(" index ")" )? index*
//				| ident "(" args? ")"
// 				| "(" expr ")"
// 				| "({" stmt "})"
// index      = "[" primary "]"
// args       = expr ("," expr)*
// decl_a     = ("int" | "char") "*"* (pointed_id | ident)
// decl_b     = decl_a ("[" num "]")*
// decl_c     = decl_a "[]"? ("[" num? "]")* "=" (array_init | expr)
// decl_g     = decl_a "[]"? ("[" num? "]")* "=" (array_init | equality) // その他、制限あり
// pointed_id = "(" "*"* ident ")"
// ident      =
// literal_str=
// num        =

Global *global_variable_declaration(Token *variable_name, Type *type);

Function *function(Token *function_name, Type *returnType);

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

char *new_label() {
    static int cnt = 0;
    char buf[20];
    sprintf(buf, ".LC.%d", cnt++);
    return strndup(buf, 20);
}

Token *consume(char *op) {
    Token *const t = token;
    if (token->kind != TK_RESERVED ||
        strlen(op) != token->len ||
        memcmp(token->str, op, token->len) != 0)
        return NULL;
    token = token->next;
    return t;
}

Type *consume_base_type() {
    if (consume("int")) {
        return shared_int_type();
    } else if (consume("char")) {
        return shared_char_type();
    } else if (consume("void")) {
        return shared_void_type();
    } else {
        return NULL;
    }
}

Token *consume_type(Type *base, Type **r_type);

Token *consume_ident() {
    if (token->kind == TK_IDENT) {
        Token *t = token;
        token = token->next;
        return t;
    }
    return NULL;
}

Token *consume_number() {
    if (token->kind == TK_NUM) {
        Token *t = token;
        token = token->next;
        return t;
    }
    return NULL;
}

// 次のトークンが期待している記号のときには、トークンを1つ読み進める。
// それ以外の場合にはエラーを報告する。
void expect(char *op) {
    if (token->kind != TK_RESERVED ||
        strlen(op) != token->len ||
        memcmp(token->str, op, token->len) != 0) {
        error_at(token->str, "'%s'ではありません", op);
        exit(1);
    }
    token = token->next;
}

// 次のトークンが数値の場合、トークンを1つ読み進めてその数値を返す。
// それ以外の場合にはエラーを報告する。
int expect_number() {
    if (token->kind != TK_NUM) {
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
        if (var->len == len && !memcmp(name, var->name, var->len)) {
            return var;
        }
    }
    return find_local_variable(scope->parent, name, len);
}

Global *find_string_literal(char *name, int len) {
    for (Global *g = globals->head; g; g = g->next) {
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

Global *find_global_variable(char *name, int len) {
    for (Global *g = globals->head; g; g = g->next)
        if (g->label_length == len && !strncmp(name, g->label, g->label_length))
            return g;
    return NULL;
}

Declaration *find_function(char *name, int len) {
    for (Declaration *d = declarations->head; d; d = d->next)
        if (d->len == len && !strncmp(name, d->name, d->len))
            return d;
    return NULL;
}

Variable *register_variable(char *str, int len, Type *type) {
    Variable *variable = calloc(1, sizeof(Variable));
    variable->type = type;
    variable->name = str;
    variable->len = len;
    variable->offset = stack_size = stack_size + get_size(type);
    variable->index = -1;
    variable->next = current_scope->variables;
    current_scope->variables = variable;
    return variable;
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
            }
            break;
        }
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
    node->is_local = false;
    node->name = variable->label;
    node->len = variable->label_length;
    return node;
}

Node *new_node_string_literal() {
    // Globalsから検索
    Global *g = find_string_literal(token->str, token->len);
    if (!g) {
        char *const label = new_label();
        const int label_length = (int) strlen(label);
        g = calloc(1, sizeof(Global));
        g->label = label;
        g->label_length = label_length;
        g->target = calloc(1, sizeof(Directives));
        g->target->directive = _string;
        g->target->literal = token->str;
        g->target->literal_length = token->len;
        // Globalsに追加
        add_globals(g);
    }

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
    }
}

Node *new_node_function_call(const Token *tok) {
    // 存在チェック
    Declaration *declaration = find_function(tok->str, tok->len);
    if (!declaration) {
        error_at(tok->str, "関数が定義されていません");
        exit(1);
    }
    // 関数呼び出し
    Node *node = calloc(1, sizeof(Node));
    node->kind = ND_FUNC;
    node->type = declaration->return_type;
    if (!consume(")")) {
        Node *last = node;
        last->args = expr();
        while (consume(",") && (last = last->args)) {
            last->args = expr();
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
        // TODO
        error("異なるポインター型の演算はできません？\n");
        exit(1);
    }
}

Assignable assert_assignable(char *loc,
                             Type *const left_type,
                             Type *const right_type,
                             const bool rZero) {
    switch (are_assignable_type(left_type, right_type, rZero)) {
        case AS_INCOMPATIBLE:
            warn_at(loc, "warning: 代入式の左右の型が異なります。");
            warn_incompatible_type(left_type, right_type);
        case AS_SAME:
            break;
        case CANNOT_ASSIGN:
            error_at(loc, "error: 代入式の左右の型が異なります。");
            exit(1);
    }
    return true;
}

Node *new_node_assign(char *loc, Node *const lhs, Node *const rhs) {
    Type *const left_type = find_type(lhs);
    Type *const right_type = find_type(rhs);
    // コンパイラで生成しているnew_node_num(0)もあるはずだが、
    // 代入式の右辺の最上部Nodeに現れるものはリテラル相当のはず
    // （0リテラル、配列の0パディング）
    bool rZero = rhs->kind == ND_NUM && rhs->val == 0;
    if (assert_assignable(loc, left_type, right_type, rZero)) {
        return new_node(ND_ASSIGN, lhs, rhs);
    }
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

//////////////////////////////////////////////////////////////////

struct Program *parse(Token *tok) {
    token = tok;
    globals = calloc(1, sizeof(struct Global_C));
    declarations = calloc(1, sizeof(struct Declaration_C));

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
        // TODO 関数の返り値に配列は書けない
        Type *type;
        Token *identifier = consume_type(base, &type);
        if (!identifier) {
            error_at(token->str, "関数名または変数名がありません");
            exit(1);
        }
        // 同名で同型のグローバル変数は宣言できる
        // 他のグローバル変数や関数との名前重複チェック
        if (find_global_variable(identifier->str, identifier->len)
            || find_function(identifier->str, identifier->len)) {
            error_at(identifier->str, "関数名またはグローバル変数名が重複しています");
            exit(1);
        }
        if (consume("(")) {
            // TODO 関数宣言（テストを動かすための仮実装）
            Token *saved = token;
            if (consume(")") && consume(";")) {
                Declaration *d = calloc(1, sizeof(Declaration));
                d->return_type = type;
                d->name = identifier->str;
                d->len = identifier->len;
                add_function_declaration(d);
                continue;
            }
            token = saved;
            // 関数
            tail_f = tail_f->next = function(identifier, type);
        } else {
            // グローバル変数
            Global *const g = global_variable_declaration(identifier, type);
            add_globals(g);
        }
    }

    token = NULL;

    struct Program *prog = calloc(1, sizeof(struct Program));
    prog->functions = head_f.next;
    prog->globals = globals->head;
    return prog;
}

int retrieve_int(Node *node, Node **pointed) {
    switch (node->kind) {
        case ND_ADD: {
            int left = retrieve_int(node->lhs, pointed);
            int right = retrieve_int(node->rhs, pointed);
            return left + right;
        }
        case ND_SUB: {
            int left = retrieve_int(node->lhs, pointed);
            int right = retrieve_int(node->rhs, pointed);
            return left - right;
        }
        case ND_MUL: {
            int left = retrieve_int(node->lhs, pointed);
            int right = retrieve_int(node->rhs, pointed);
            return left * right;
        }
        case ND_DIV: {
            int left = retrieve_int(node->lhs, pointed);
            int right = retrieve_int(node->rhs, pointed);
            return left / right;
        }
        case ND_NUM:
            return node->val;
        case ND_ADDRESS:
            return retrieve_int(node->lhs, pointed);
        case ND_VARIABLE:
        case ND_VARIABLE_ARRAY: {
            if (!pointed) {
                // ポインタは予期していない
                error_at(token->str, "ぐいいいいいいいいいいいいい\n");
                exit(1);
            } else if (*pointed) {
                // ポインタが複数あった
                error_at(token->str, "ぐああああああああああああああああ\n");
                exit(1);
            } else {
                *pointed = node;
                return 0;
            }
        }
        default:
            error_at(token->str, "ぐええええええええええええ\n");
            exit(1);
    }
}

Global *global_variable_declaration(Token *variable_name, Type *type) {
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
     *  参考演算子は使えるが、コンパイル時に解決される
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
    g->target = calloc(1, sizeof(Directives));
    Directives *target = g->target;
    char *loc = token->str;
    if (consume("=")) {
        if (type->ty == TYPE_ARRAY) {
            error_at(loc, "TODO: グローバル変数（配列）の初期化");
            exit(1);
        } else {
            Node *const node = equality();
            // 型チェック
            bool rZero = node->kind == ND_NUM && node->val == 0;
            assert_assignable(loc, type, find_type(node), rZero);
            switch (node->kind) {
                case ND_STR_LITERAL:
                    target->directive = _quad;
                    target->reference = node->label;
                    target->reference_length = node->label_length;
                    break;
                case ND_NUM:
                    // リテラル、sizeof
                    target->directive = _long;
                    target->value = node->val;
                    break;
                case ND_ADD:
                case ND_SUB:
                case ND_MUL:
                case ND_DIV: {
                    int sum = retrieve_int(node, NULL);
                    target->directive = _long;
                    target->value = sum;
                    break;
                }
                case ND_ADDRESS: {
                    // ポインタ
                    Node *v = node->lhs;
                    switch (v->kind) {
                        case ND_VARIABLE:
                        case ND_VARIABLE_ARRAY:
                            target->directive = _quad;
                            target->reference = v->name;
                            target->reference_length = v->len;
                            break;
                        case ND_DEREF:
                        case ND_DEREF_ARRAY_POINTER: {
                            Node *pointed = NULL;
                            int sum = retrieve_int(v->lhs, &pointed);
                            if (pointed) {
                                target->directive = _quad;
                                target->reference = pointed->name;
                                target->reference_length = pointed->len;
                                target->value = sum;
                            } else {
                                error_at(loc, "ぐaaaaaaaaaaaaaa");
                                exit(1);
                            }
                            break;
                        }
                        default:
                            error_at(loc, "グローバル変数の初期化に使えるアドレスはグローバル変数のみです");
                            exit(1);
                    }
                    break;
                }
                case ND_EQL:
                case ND_NOT:
                case ND_LESS:
                case ND_LESS_EQL:
                    error_at(loc, "TODO: グローバル変数の初期化は未実装");
                    exit(1);
                case ND_VARIABLE: // Nodeのトップには来ない
                case ND_VARIABLE_ARRAY: // Nodeのトップには来ない
                case ND_DEREF: // Nodeのトップには来ない
                case ND_DEREF_ARRAY_POINTER: // Nodeのトップには来ない
                case ND_EXPR_STMT:
                case ND_IF:
                case ND_WHILE:
                case ND_FOR:
                case ND_BLOCK:
                case ND_FUNC:
                case ND_RETURN:
                case ND_ASSIGN:
                case ND_NOTHING:
                    error_at(loc, "グローバル変数の初期化では非対応です？");
                    exit(1);
            }
        }
    } else {
        if (type->ty == TYPE_ARRAY && type->array_size == 0) {
            error_at(loc, "配列のサイズが指定されていません。");
            exit(1);
        }
        target->directive = _zero;
        // サイズ分を0で初期化
        target->value = get_size(type);
    }
    expect(";");
    return g;
}

void function_parameter() {
    /*
     * int function_name(
     *                   ↑ここから
     */
    int i = 0;
    Type *param_type;
    while ((param_type = consume_base_type())) {
        if (param_type->ty == TYPE_VOID) {
            error_at(token->str, "引数にvoidは使えません");
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
            error_at(token->str, "引数名がありません");
            exit(1);
        }

        Variable *param = register_variable(t->str, t->len, param_type);
        param->index = i++;
        if (!consume(","))
            break;
    }

    expect(")");
}

NodeArray *function_body() {
    expect("{");
    NodeArray *nodeArray = create_node_array(10);
    while (!consume("}")) {
        push_node(nodeArray, stmt());
    }
    return nodeArray;
}

Function *function(Token *function_name, Type *returnType) {
    Scope *parameter = current_scope = calloc(1, sizeof(Scope));
    function_parameter();

    // 仮引数以外は不要、かつblockスコープのものは捨てるので、他のローカル変数も捨てることにする
    Scope func_body;
    func_body.variables = NULL;
    func_body.parent = parameter;
    // 関数スコープ
    current_scope = &func_body;

    // 再帰呼び出しでの定義チェックがあるため、先に関数宣言として追加
    Declaration *d = calloc(1, sizeof(Declaration));
    d->return_type = returnType;
    d->name = function_name->str;
    d->len = function_name->len;
    add_function_declaration(d);

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
  配列の初期化時のみ可能な式がいくつか
  int array[4] = {0, 1, 2, 3};
  int array[4] = {0, 1, 2};
  int array[] = {0, 1, 2, 3};
  char msg[4] = {'f', 'o', 'o', '_'}; => 警告なし
  char msg[4] = {'f', 'o', 'o', '\0'};
  char msg[] = {'f', 'o', 'o', '\0'};
  char *s1[2] = {"abc", "def"};
  char *s1[] = {"abc", "def"};
  char msg[] = "foo";
  char msg[10] = "foo";
  char msg[3] = "message"; => initializer-string for array of chars is too long
  char s1[2][3] = {"abc", "def"};
  char s1[][3] = {"abc", "def"};
  int array[3][2] = {{3, 3}, {3, 3}, {3, 3}};
  int array[][2] = {{3, 3}, {3, 3}, {3, 3}};
  TODO C99にはさらに色々ある
   https://kaworu.jpn.org/c/%E9%85%8D%E5%88%97%E3%81%AE%E5%88%9D%E6%9C%9F%E5%8C%96_%E6%8C%87%E7%A4%BA%E4%BB%98%E3%81%8D%E3%81%AE%E5%88%9D%E6%9C%9F%E5%8C%96%E6%8C%87%E5%AE%9A%E5%AD%90
 */
Node *array_initializer(Node *const array_variable, Type *const type) {
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
        char *loc = token->str; // TODO
        last = last->statement = new_node_assign(loc, indexed, n);
    }
    return block;
}

Token *consume_type(Type *base, Type **r_type) {
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
            error_at(token->str, "変数名が重複しています");
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
        Token *size_token = consume_number();
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
    // 変数の登録
    Type *type;
    Token *const identifier = consume_type(base, &type);
    if (base->ty == TYPE_VOID) {
        error_at(token->str, "変数にvoidは使えません");
        exit(1);
    }
    if (!identifier) {
        error_at(token->str, "変数名がありません");
        exit(1);
    }
    Variable *variable = register_variable(identifier->str, identifier->len, type);
    // 初期化
    if (consume("=")) {
        // RBPへのオフセットが決定しない場合がある
        const bool implicit_size = type->array_size == 0;
        Node *const variable_node = new_node_local_variable(variable->name, variable->len);
        if (type->ty == TYPE_ARRAY) {
            // 配列の初期化
            Node *node = array_initializer(variable_node, type);
            if (implicit_size) {
                // 配列のサイズが決まってからオフセットを再設定する
                variable->offset = stack_size = stack_size + get_size(type);
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
            return with_index(variable);
        }
    }

    // 文字列リテラル
    if (token->kind == TK_STR_LITERAL) {
        return new_node_string_literal();
    }
    // 文字リテラル
    if (token->kind == TK_CHAR_LITERAL) {
        const int c = token->val;
        token = token->next;
        return new_node_num(c);
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
        return with_index(node);
    }

    // そうでなければ数値のはず
    Node *number = new_node_num(expect_number());
    return with_index(number);
}
