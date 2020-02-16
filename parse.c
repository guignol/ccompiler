#include "9cc.h"

// 現在着目しているトークン
Token *token;
// ローカル変数
Variable *locals;

/////////////////////////

struct Declaration_C {
    Declaration *head;
    Declaration *tail;
};
// 関数定義
struct Declaration_C *declarations;

void add_function_declaration(Declaration *next) {
    if (!declarations) {
        declarations = calloc(1, sizeof(struct Declaration_C));
    }
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
//				| ident "(" args ")"
// 				| "(" expr ")"
// index      = "[" primary "]"
// args       = (expr ("," expr)* )?
// decl_a     = ("int" | "char") "*"* (pointed_id | ident)
// decl_b     = decl_a ("[" num "]")*
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

Variable *find_local_variable(char *name, int len) {
    // 同名の変数は宣言できないので、型チェックはしない
    for (Variable *var = locals; var; var = var->next)
        if (var->len == len && !memcmp(name, var->name, var->len))
            return var;
    return NULL;
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
    variable->offset = (locals ? locals->offset : 0) + get_size(type);
    variable->index = -1;
    variable->next = locals;
    locals = variable;
    return variable;
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
    Variable *variable = find_local_variable(str, len);
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

Node *with_index(Node *left);

//////////////////////////////////////////////////////////////////

struct Program *program(Token *t) {
    token = t;
    globals = calloc(1, sizeof(struct Global_C));

    // 関数
    Function head_f;
    head_f.next = NULL;
    Function *tail_f = &head_f;
    {
        // デバッグ用の関数定義
        static char *foo[] = {"hoge", "bar", "foo", "alloc_array_4", "printf"};
        for (int i = 0; i < sizeof(foo) / sizeof(*foo); ++i) {
            Declaration *d = calloc(1, sizeof(Declaration));
            d->return_type = shared_int_type();
            d->name = foo[i];
            d->len = (int) strlen(d->name);
            add_function_declaration(d);
        }
    }

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
        // TODO グローバル変数で、まずポインタ1つまで
        char *pointer = consume("*");
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
            if (pointer) {
                error_at(pointer, "関数の入出力にポインタはまだ使えません");
                exit(1);
            }
            // 関数
            tail_f = tail_f->next = function(identifier, base);
        } else {
            // グローバル変数
            Type *const type = pointer
                               ? create_pointer_type(base)
                               : base;
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

Function *function(Token *function_name, Type *returnType) {
    {
        /*
         * int function_name(
         *                   ↑ここから
         */
        int i = 0;
        Type *param_type;
        while ((param_type = consume_base_type())) {
            if (consume("*")) {
                error_at(token->str, "関数の入出力にポインタはまだ使えません");
                exit(1);
            }
            Token *t = consume_ident();
            if (t) {
                if (find_local_variable(t->str, t->len)) {
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
    }
    expect(")");

    // 再帰呼び出しでの定義チェックがあるため、先に関数定義として追加
    Declaration *d = calloc(1, sizeof(Declaration));
    d->return_type = returnType;
    d->name = function_name->str;
    d->len = function_name->len;
    add_function_declaration(d);

    expect("{");
    // Node *code[100];
    Node **body = (Node **) malloc(sizeof(Node *) * 100);
    {
        int i = 0;
        while (!consume("}")) {
            body[i++] = stmt();
        }

        body[i] = NULL;
    }

    Function *function = calloc(1, sizeof(Function));
    function->name = copy(function_name->str, function_name->len);
    function->locals = locals;
    function->body = body;

    locals = NULL;

    return function;
}

Node *stmt() {
    Node *node;
    Type *base = consume_base_type();
    if (base) {
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
        // 同名の変数は宣言できない
        // グローバル変数との重複は可能
        if (find_local_variable(t->str, t->len)) {
            error_at(token->str, "変数名が重複しています");
            exit(1);
        }
        Type *backwards_pointer = NULL;
        if (backwards) {
            expect(")");
            backwards_pointer = type;
            type = base;
        }

        while (consume("[")) {
            int array_size = expect_number();
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
        register_variable(t->str, t->len, type);
        node = new_node(ND_NOTHING, NULL, NULL);
    } else if (consume("{")) {
        node = calloc(1, sizeof(Node));
        node->kind = ND_BLOCK;
        Node *last = node;
        while (!consume("}")) {
            Node *next = stmt();
            last->statement = next;
            last = next;
        }
        return node;
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
    } else {
        node = new_node(ND_EXPR_STMT, expr(), NULL); // 式文
    }
    expect(";");
    return node;
}

Node *expr() {
    return assign();
}

Node *assign() {
    Node *node = equality();
    if (consume("=")) {
        char *loc = token->str;
        Node *rhs = assign();
        Type *const left_type = find_type(node);
        Type *const right_type = find_type(rhs);
        switch (are_assignable_type(left_type, right_type)) {
            case AS_INCOMPATIBLE:
//                error("\n");
//                error_at(loc, "warning: 代入式の左右の型が異なります。");
//                warn_incompatible_type(left_type, right_type);
            case AS_SAME:
                node = new_node(ND_ASSIGN, node, rhs);
                break;
            case CANNOT_ASSIGN:
                error_at(loc, "error: 代入式の左右の型が異なります。");
                exit(1);
        }
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

    // 次のトークンが"("なら、"(" expr ")" または (a[0])[1], (*b)[1]
    if (consume("(")) {
        Node *node = expr();
        expect(")");
        return with_index(node);
    }

    // そうでなければ数値のはず
    Node *number = new_node_num(expect_number());
    return with_index(number);
}

void assert_indexable(Node *left, Node *right) {
    // 片方がintで、もう片方が配列orポインタ
    Type *left_type = find_type(left);
    Type *right_type = find_type(right);
    switch (left_type->ty) {
        case TYPE_CHAR:
        case TYPE_INT: {
            // 左がint
            switch (right_type->ty) {
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

Node *with_index(Node *left) {
    if (left->kind == ND_INDEX) {
        left->kind = ND_INDEX_CONTINUE;
    }
    while (consume("[")) {
        Node *right = expr();
        expect("]");
        // int a[2];
        // a[1] = 3;
        // は
        // *(a + 1) = 3;
        // の省略表現
        assert_indexable(left, right);
        Node *pointer = pointer_calc(ND_ADD, left, right);
        left = new_node(ND_INDEX_CONTINUE, pointer, NULL);
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