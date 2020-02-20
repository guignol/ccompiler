#include "9cc.h"

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
//				| "for" "(" expr? ";" expr? ";" expr? ")" stmt
// expr       = assign
// assign     = equality ("=" assign)?
// equality   = relational ("==" relational | "!=" relational)*
// relational = add ("<" add | "<=" add | ">" add | ">=" add)*
// add        = mul ("+" mul | "-" mul)*
// mul        = unary ("*" unary | "/" unary)*
// unary      = "sizeof" unary
//				| ("+" | "-" | "*"* | "&"*)? primary
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
// decl_c     = decl_a ("[" num? "]")* "=" (array_init | expr)
// array_init = "{" args "}"
// pointed_id = "(" "*"* ident ")"
// ident      =
// literal_str=
// num        =

Global *global_var(Token *variable_name, Type *type);

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

char *consume(char *op) {
    char *const location = token->str;
    if (token->kind != TK_RESERVED ||
        strlen(op) != token->len ||
        memcmp(location, op, token->len) != 0)
        return NULL;
    token = token->next;
    return location;
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
    char *copied = malloc(sizeof(char *));
    strncpy(copied, str, len);
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

Global *find_global_variable(char *name, int len) {
    for (Global *g = globals->head; g; g = g->next)
        if (g->label_length == len && !memcmp(name, g->label, g->label_length))
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

Node *new_node_variable(char *str, int len) {
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

Node *new_node_variable_global(char *str, int len) {
    Global *variable = find_global_variable(str, len);
    if (!variable) {
        error_at(str, "変数の宣言がありません。");
        exit(1);
    }
    Node *node = calloc(1, sizeof(Node));
    node->kind = ND_VARIABLE;
    node->type = variable->type;
    node->is_local = false;
    node->name = str;
    node->len = len;
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

Node *pointer_calc(NodeKind kind, Node *left, Node *right) {
    const int weight_l = get_weight(left);
    const int weight_r = get_weight(right);
    if (weight_l == weight_r) {
        if (kind == ND_SUB) {
            // ポインタ同士の引き算
            Node *node = new_node(kind, left, right);
            return new_node(ND_DIV, node, new_node_num(weight_l));
        } else {
            // TODO 足し算はどうなるべきか
            return new_node(kind, left, right);
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

Node *new_node_assign(char *loc, Node *const lhs, Node *const rhs) {
    Type *const left_type = find_type(lhs); // TODO 初期化
    Type *const right_type = find_type(rhs);
    switch (are_assignable_type(left_type, right_type)) {
        case AS_INCOMPATIBLE:
//                error("\n");
//                error_at(loc, "warning: 代入式の左右の型が異なります。");
//                warn_incompatible_type(left_type, right_type);
        case AS_SAME:
            return new_node(ND_ASSIGN, lhs, rhs);
        case CANNOT_ASSIGN:
            error_at(loc, "error: 代入式の左右の型が異なります。");
            exit(1);
    }
}

Node *new_node_array_index(Node *const left, Node *const right, const bool continued) {
    // int a[2];
    // a[1] = 3;
    // は
    // *(a + 1) = 3;
    // の省略表現
    assert_indexable(left, right);
    Node *pointer = pointer_calc(ND_ADD, left, right);
    return new_node(continued ? ND_INDEX_CONTINUE : ND_INDEX, pointer, NULL);
}

Node *new_node_array_initializer(Node *const array, Type *const type) {
    /*
      TODO 配列の初期化時のみ可能な式がいくつか
     * char msg[4] = {'f', 'o', 'o', '\0'};
     * char msg[] = "foo";
     * char msg[10] = "foo";
     * char msg[3] = "message"; => initializer-string for array of chars is too long
      int array[4] = {0, 1, 2, 3};
      int array[4] = {0, 1, 2};
      int array[] = {0, 1, 2, 3};
     */
    if (consume("{")) {
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
        Node *const node = calloc(1, sizeof(Node));
        node->kind = ND_BLOCK;
        Node *last = node;
        const bool undefined_size = type->array_size == 0;
        int element_count = 0;
        do {
            // 配列に各要素を代入
            Node *const index = new_node_num(element_count++);
            if (!undefined_size && type->array_size < index->val) {
                error_at(token->str, "変数のサイズより大きい配列を入れようとしています。");
                exit(1);
            }
            Node *const indexed_array = new_node_array_index(array, index, false);
            Node *const next = new_node_assign(token->str, indexed_array, expr());
            last->statement = next;
            last = next;
            consume(","); // 末尾に残ってもOK
        } while (!consume("}"));
        if (undefined_size) {
            // サイズを明示していない配列のサイズを決定する
            array->type->array_size = element_count;
        } else {
            // TODO ポインタの場合は？
            // 初期化式の要素数が配列のサイズより小さい場合、0で埋める。
            Node *const zero = new_node_num(0);
            while (element_count < type->array_size) {
                Node *const index = new_node_num(element_count++);
                Node *const left = new_node_array_index(array, index, false);
                Node *const next = new_node_assign(token->str, left, zero);
                last->statement = next;
                last = next;
            }
        }
        return node;
    } else {
        // TODO 文字列リテラルのみ？
        return new_node_assign(token->str, array, assign());
    }
}

Node *with_index(Node *left);

//////////////////////////////////////////////////////////////////

struct Program *parse(Token *tok) {
    token = tok;
    globals = calloc(1, sizeof(struct Global_C));
    declarations = calloc(1, sizeof(struct Declaration_C));

    // 関数
    Function head_f;
    head_f.next = NULL;
    Function *tail_f = &head_f;
    {
//        char *const label = new_label();
//        // デバッグ用のグローバル変数
//        Global *g = calloc(1, sizeof(Global));
//        g->label = "debug_moji"; // 変数名
//        g->label_length = (int) strlen(g->label);
//        g->directive = _quad;
//        g->target = calloc(1, sizeof(directive_target));
//        g->target->label = label;
//        g->target->label_length = (int) strlen(g->target->label);
//        add_globals(g);
//        g = calloc(1, sizeof(Global));
//        g->label = label;
//        g->label_length = (int) strlen(g->label);
//        g->directive = _string;
//        g->target = calloc(1, sizeof(directive_target));
//        g->target->literal = "moji: %i\\n"; // リテラル
//        g->target->literal_length = (int) strlen(g->target->literal);
//        add_globals(g);
    }
    while (!at_eof()) {
        Type *base = consume_base_type();
        if (!base) {
            error_at(token->str, "関数の型が正しく定義されていません");
            exit(1);
        }
        // TODO まずポインタ1つまで
        Type *const type = consume("*")
                           ? create_pointer_type(base)
                           : base;
        Token *identifier = consume_ident();
        if (!identifier) {
            error_at(token->str, "関数名または変数名がありません");
            exit(1);
        }
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
            Global *const g = global_var(identifier, type);
            add_globals(g);
        }
    }

    token = NULL;

    struct Program *prog = calloc(1, sizeof(struct Program));
    prog->functions = head_f.next;
    prog->globals = globals->head;
    return prog;
}

Global *global_var(Token *variable_name, Type *type) {
    // TODO まずは宣言のみ
    expect(";");
    Global *g = calloc(1, sizeof(Global));
    g->type = type;
    g->label = variable_name->str;
    g->label_length = variable_name->len;
    g->directive = _zero;
    g->target = calloc(1, sizeof(directive_target));
    // サイズ分を0で初期化
    g->target->value = get_size(type);
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

Node **function_body() {
    expect("{");
    // Node *code[100];
    Node **body = (Node **) malloc(sizeof(Node *) * 100);
    {
        int i = 0;
        while (!consume("}")) {
            // この数が100まで、なので、Node自体は100を超えても大丈夫
            body[i++] = stmt();
        }
        // ただし、これが75になる関数がテストコード内にある
//        printf("# FUNCTION TOP LEVEL Node: %d\n", i + 1);

        body[i] = NULL;
    }
    return body;
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

Node *stmt(void) {
    Node *node;
    Type *base = consume_base_type();
    if (base) {
        if (base->ty == TYPE_VOID) {
            error_at(token->str, "変数にvoidは使えません");
            exit(1);
        }
        // ローカル変数の宣言
        bool backwards = consume("(");
        Type *type = backwards ? NULL : base;
        while (consume("*")) {
            Type *pointer = create_pointer_type(type);
            type = pointer;
        }
        Token *t = consume_ident();
        if (!t) {
            error_at(token->str, "変数名がありません");
            exit(1);
        }
        {
            // 同じスコープ内で同名の変数は宣言できない
            // グローバル変数との重複は可能
            Scope *parent = current_scope->parent;
            current_scope->parent = NULL; // 現在のスコープのみ
            if (find_local_variable(current_scope, t->str, t->len)) {
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

        bool undefined_size_array = false;
        while (consume("[")) {
            Token *size_token = consume_number();
            if (!size_token) {
                // 初期化式では右辺からサイズを決定できる
                // 配列の配列ではできないはずなのでここでbreakする
                undefined_size_array = true;
                type = create_array_type(type, 0);
                expect("]");
                break;
            }
            int array_size = size_token->val;
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
        // 変数の登録（RBPへのオフセットが決定しない場合がある）
        Variable *const variable = register_variable(t->str, t->len, type);
        if (consume("=")) {
            Node *const variable_node = new_node_variable(t->str, t->len);
            if (type->ty == TYPE_ARRAY) {
                // 配列の初期化
                node = new_node_array_initializer(variable_node, type);
                if (undefined_size_array) {
                    // 配列のサイズが決まってからオフセットを再設定する
                    variable->offset = stack_size = stack_size + get_size(type);
                    variable_node->offset = variable->offset;
                }
            } else {
                if (backwards_pointer && undefined_size_array) {
                    // TODO
                    // 配列へのポインタはサイズ不定でもOK
                    // ただし、サイズを指定して不一致の場合はwarningを出す
                    // int (*pointer)[] = &array;
                }
                node = new_node_assign(token->str, variable_node, assign());
            }
            expect(";");
            return node;
        } else {
            node = new_node(ND_NOTHING, NULL, NULL);
            expect(";");
            return node;
        }
    } else if (consume("{")) {
        return block_statement();
    } else if (consume("if")) {
        node = calloc(1, sizeof(Node));
        node->kind = ND_IF;
        expect("(");
        node->condition = expr();
        expect(")");
        node->lhs = stmt();
        node->rhs = consume("else") ? stmt() : NULL;
        return node;
    } else if (consume("while")) {
        node = calloc(1, sizeof(Node));
        node->kind = ND_WHILE;
        expect("(");
        node->condition = expr();
        expect(")");
        node->lhs = stmt();
        return node;
    } else if (consume("for")) {
        node = calloc(1, sizeof(Node));
        node->kind = ND_FOR;
        expect("(");
        // init
        if (!consume(";")) {
            // TODO 変数宣言とそのスコープ、初期化
            node->lhs = new_node(ND_EXPR_STMT, expr(), NULL);
            expect(";");
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
        return node;
    } else if (consume("return")) {
        node = calloc(1, sizeof(Node));
        node->kind = ND_RETURN;
        node->lhs = expr();
        expect(";");
        return node;
    } else {
        node = new_node(ND_EXPR_STMT, expr(), NULL); // 式文
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
        node = new_node_assign(token->str, node, assign());
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
        Node *operand = unary();
        Type *type = find_type(operand);
        int size = get_size(type);
        return new_node_num(size);
    }

    if (consume("+"))
        return primary();
    if (consume("-"))
        return new_node(ND_SUB, new_node_num(0), primary());
    if (consume("*")) {
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
    }
    if (consume("&")) {
        Node *operand = primary();
        return new_node(ND_ADDRESS, operand, NULL);
    }
    return primary();
}

Node *primary() {
    Token *tok = consume_ident();
    if (tok) {
        if (consume("(")) {
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
        } else {
            Node *variable = new_node_variable(tok->str, tok->len);
            if (!variable) {
                variable = new_node_variable_global(tok->str, tok->len);
            }
            return with_index(variable);
        }
    }

    // 文字列リテラル
    if (token->kind == TK_STR_LITERAL) {
        // Globalsに追加
        char *const label = new_label();
        const int label_length = (int) strlen(label);
        Global *g = calloc(1, sizeof(Global));
        g->label = label;
        g->label_length = label_length;
        g->directive = _string;
        g->target = calloc(1, sizeof(directive_target));
        g->target->literal = token->str;
        g->target->literal_length = token->len;
        add_globals(g);
        // ラベルを指すnodeを作る
        Node *const node = new_node(ND_STR_LITERAL, NULL, NULL);
        node->label = label;
        node->label_length = label_length;
        token = token->next;
        return node;
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

Node *with_index(Node *left) {
    if (left->kind == ND_INDEX) {
        left->kind = ND_INDEX_CONTINUE;
    }
    while (consume("[")) {
        left = new_node_array_index(left, expr(), true);
        expect("]");
    }
    if (left->kind == ND_INDEX_CONTINUE) {
        /*
         * 配列は代入できないので、[]でのアクセスは連続していて、
         * なので、ここで終端判定ができるはず
         * （ポインタ変数には代入できるけど忘れていいはず）
         *
         * ただし、括弧は書けるので、括弧の中をパースした後にもう一度ここを通る場合がある
         * (a[0])[1];
         * その判定のために ND_DEREF ではなく ND_INDEX という別の種別を導入した
         */
        left->kind = ND_INDEX;
    }
    return left;
}
