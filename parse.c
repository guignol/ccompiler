#include "9cc.h"

// 現在着目しているトークン
Token *token;
// ローカル変数
Variable *locals;

// program    = function*
// function   = type ident "(" (type ident)* ")" { stmt* }
// stmt       = expr ";"
//              | type ident ("[" num "]")* ";"
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
//				| ("+" | "-" | "*" | "&")? primary
// primary    = num |
//				| ident
//				| primary ( "(" index ")" )? index*
//				| ident "(" args ")"
// 				| "(" expr ")"
// index      = "[" primary "]"
// args       = (expr ("," expr)* )?
// type       = "int" "*"*

Function *function();

Node *stmt();

Node *expr();

Node *assign();

Node *equality();

Node *relational();

Node *add();

Node *mul();

Node *unary();

Node *primary();

//////////////////////////////////////////////////////////////////

// 次のトークンが期待している記号のときには、トークンを読み進めて
// 真を返す。それ以外の場合には偽を返す。
bool consume(char *op) {
    if (token->kind != TK_RESERVED ||
        strlen(op) != token->len ||
        memcmp(token->str, op, token->len) != 0)
        return false;
    token = token->next;
    return true;
}

Token *consume_ident() {
    if (token->kind == TK_IDENT) {
        Token *t = token;
        token = token->next;
        return t;
    }
    return NULL;
}

void assert_not_asterisk() {
    if (token->kind == TK_RESERVED &&
        strlen("*") == token->len &&
        memcmp(token->str, "*", token->len) == 0) {
        error_at(token->str, "関数の入出力にポインタはまだ使えません");
        exit(1);
    }
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
    // TODO 同名の変数は宣言できない前提なので、型チェックはしない
    for (Variable *var = locals; var; var = var->next)
        if (var->len == len && !memcmp(name, var->name, var->len))
            return var;
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
        error_at(str, "変数の宣言がありません。");
        exit(1);
    }
    bool is_array = variable->type->ty == TYPE_ARRAY;
    Node *node = calloc(1, sizeof(Node));
    node->kind = is_array ? ND_VARIABLE_ARRAY : ND_VARIABLE;
    node->type = variable->type;
    node->offset = variable->offset;
    node->name = str;
    node->len = len;
    return node;
}

Node *new_node_dereference(Node *operand) {
    Type *type = find_type(operand);
    switch (type->ty) {
        case TYPE_INT:
            return NULL;
        case TYPE_POINTER:
        case TYPE_ARRAY:
            return new_node(ND_DEREF, operand, NULL);
    }
}

Node *with_index(Node *left);

//////////////////////////////////////////////////////////////////

Function *program(Token *t) {
    token = t;

    Function head;
    head.next = NULL;
    Function *tail = &head;
    while (!at_eof()) {
        tail->next = function();
        tail = tail->next;
    }

    token = NULL;

    return head.next;
}

Function *function() {
    expect("int");
    assert_not_asterisk();

    Token *function_name = consume_ident();
    if (!function_name) {
        error_at(token->str, "関数名がありません");
        exit(1);
    }

    expect("(");
    {
        int i = 0;
        while (consume("int")) {
            assert_not_asterisk();
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

            Type *type = shared_int_type();
            Variable *param = register_variable(t->str, t->len, type);
            param->index = i++;
            if (!consume(","))
                break;
        }
    }
    expect(")");

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
    if (consume("int")) {
        // ローカル変数の宣言
        Type *type = shared_int_type();
        while (consume("*")) {
            Type *pointer = create_pointer_type(type);
            type = pointer;
        }
        Token *t = consume_ident();
        if (!t) {
            error_at(token->str, "変数名がありません");
            exit(1);
        }
        //  TODO 同名の変数は宣言できないことにしておく
        if (find_local_variable(t->str, t->len)) {
            error_at(token->str, "変数名が重複しています");
            exit(1);
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
             * TODO 配列へのポインタ
             * int (*p)[];
             */
            type = create_array_type(type, array_size);
            expect("]");
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
        if (are_compatible_type(find_type(node), find_type(rhs))) {
            node = new_node(ND_ASSIGN, node, rhs);
        } else {
            error_at(loc, "代入式の左右の型が異なります。");
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
            // 関数呼び出し
            Node *node = calloc(1, sizeof(Node));
            node->kind = ND_FUNC;
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
            return with_index(variable);
        }

    }

    // 次のトークンが"("なら、"(" expr ")" または (a[0])[1]
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
        case TYPE_INT: {
            // 左がint
            switch (right_type->ty) {
                case TYPE_INT:
                    error("int[int]はできません？\n");
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