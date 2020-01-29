#include "9cc.h"

// 現在着目しているトークン
Token *token;
// ローカル変数
Variable *locals;

// 次のトークンが期待している記号のときには、トークンを読み進めて
// 真を返す。それ以外の場合には偽を返す。
bool consume(char *op) {
    if (token->kind != TK_RESERVED ||
        strlen(op) != token->len ||
        memcmp(token->str, op, token->len))
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
        memcmp(token->str, "*", token->len) == 0)
        error_at(token->str, "関数の入出力にポインタはまだ使えません");
}

// 次のトークンが期待している記号のときには、トークンを1つ読み進める。
// それ以外の場合にはエラーを報告する。
void expect(char *op) {
    if (token->kind != TK_RESERVED ||
        strlen(op) != token->len ||
        memcmp(token->str, op, token->len) != 0)
        error_at(token->str, "'%s'ではありません", op);
    token = token->next;
}

// 次のトークンが数値の場合、トークンを1つ読み進めてその数値を返す。
// それ以外の場合にはエラーを報告する。
int expect_number() {
    if (token->kind != TK_NUM)
        error_at(token->str, "数ではありません");
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
    variable->offset = (locals ? locals->offset : 0) + 8;
    variable->index = -1;
    variable->next = locals;
    locals = variable;
    return variable;
}

Type *shared_int_type() {
    static Type *int_type;
    if (!int_type) {
        int_type = calloc(1, sizeof(Type));
        int_type->ty = INT;
        int_type->point_to = NULL;
    }
    return int_type;
}

Type *create_pointer_type(Type *point_to) {
    Type *type = calloc(1, sizeof(Type));
    type->ty = POINTER;
    type->point_to = point_to;
    return type;
}

bool are_same_type(Type *left, Type *right) {
    if (left->ty != right->ty) {
        return false;
    }
    switch (left->ty) {
        case INT:
            return true;
        case POINTER:
            return are_same_type(left->point_to, right->point_to);
    }
}

Type *find_type(const Node *node) {
    switch (node->kind) {
        case ND_FUNC: // TODO
        case ND_MUL:
        case ND_DIV:
        case ND_EQL:
        case ND_NOT:
        case ND_LESS:
        case ND_LESS_EQL:
        case ND_NUM:
            return shared_int_type();
        case ND_ADD:
        case ND_SUB: {
            Type *left = find_type(node->lhs);
            Type *right = find_type(node->rhs);
            if (left->ty == right->ty) {
                switch (left->ty) {
                    case INT:
                        return left;
                    case POINTER: {
                        if (are_same_type(left, right)) {
                            return left;
                        }
                        error("異なるポインター型の演算はできません？\n");
                    }
                }
            } else {
                switch (left->ty) {
                    case INT:
                        return right;
                    case POINTER:
                        return left;
                }
            }
            case ND_VARIABLE:
                return find_local_variable(node->name, node->len)->type;
            case ND_ASSIGN: {
                // 左右の型が同じことは検証済みの前提
                return find_type(node->lhs);
            }
            case ND_ADDRESS: {
                Type *operand_type = find_type(node->lhs);
                return create_pointer_type(operand_type);
            }
            case ND_DEREF:
                // オペランドがポインタ型であることは検証済みの前提
                return find_type(node->lhs)->point_to;
            default:
                error("値を返さないはずです？\n");
            return NULL;
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
    Variable *variable = find_local_variable(str, len);
    if (!variable)
        error_at(str, "変数の宣言がありません。");
    Node *node = calloc(1, sizeof(Node));
    node->kind = ND_VARIABLE;
    node->offset = variable->offset;
    node->name = str;
    node->len = len;
    return node;
}

// program    = function*
// function   = type ident "(" (type ident)* ")" { stmt* }
// stmt       = expr ";"
//				| type ident ";"
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
// unary      = ("+" | "-" | "*" | "&")? primary
// primary    = num |
//				| ident
//				| ident "(" args ")"
// 				| "(" expr ")"
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
    if (!function_name)
        error_at(token->str, "関数名がありません");

    expect("(");
    {
        int i = 0;
        while (consume("int")) {
            assert_not_asterisk();
            Token *t = consume_ident();
            if (t) {
                if (find_local_variable(t->str, t->len))
                    error_at(t->str, "引数名が重複しています");
            } else {
                error_at(token->str, "引数名がありません");
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
        Type *type = shared_int_type();
        while (consume("*")) {
            Type *pointer = create_pointer_type(type);
            type = pointer;
        }
        Token *t = consume_ident();
        if (!t)
            error_at(token->str, "変数名がありません");
        //  TODO 同名の変数は宣言できないことにしておく
        if (find_local_variable(t->str, t->len))
            error_at(token->str, "変数名が重複しています");
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
        if (are_same_type(find_type(node), find_type(rhs))) {
            node = new_node(ND_ASSIGN, node, rhs);
        } else {
            error_at(loc, "代入式の左右の型が異なります。");
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

int get_bytes(Node *node) {
    Type *type = find_type(node);
    if (type->ty == INT) {
        return 1;
    } else if (type->point_to->ty == INT) {
        // intへのポインタ
        return 4;
    } else {
        // intへのポインタのポインタ
        return 8;
    }
}

Node *pointer_calc(NodeKind kind, Node *left, Node *right) {
    const int bytes_l = get_bytes(left);
    const int bytes_r = get_bytes(right);
    if (bytes_l == bytes_r) {
        return new_node(kind, left, right);
    } else if (bytes_l == 1) {
        left = new_node(ND_MUL, new_node_num(bytes_r), left);
        return new_node(kind, left, right);
    } else if (bytes_r == 1) {
        right = new_node(ND_MUL, new_node_num(bytes_l), right);
        return new_node(kind, left, right);
    } else {
        // TODO
        error("異なるポインター型の演算はできません？\n");
    }
}

Node *add() {
    Node *node = mul();

    for (;;) {
        if (consume("+")) {
            node = pointer_calc(ND_ADD, node, mul());
        } else if (consume("-"))
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
    if (consume("+"))
        return primary();
    if (consume("-"))
        return new_node(ND_SUB, new_node_num(0), primary());
    if (consume("*")) {
        char *loc = token->str;
        Node *operand = primary();
        if (find_type(operand)->ty != POINTER) {
            error_at(loc, "ポインタ型ではありません");
        }
        return new_node(ND_DEREF, operand, NULL);
    }
    if (consume("&"))
        return new_node(ND_ADDRESS, primary(), NULL);
    return primary();
}

Node *primary() {
    Token *tok = consume_ident();
    if (tok) {
        if (consume("(")) {
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
            return new_node_variable(tok->str, tok->len);
        }
    }

    // 次のトークンが"("なら、"(" expr ")"のはず
    if (consume("(")) {
        Node *node = expr();
        expect(")");
        return node;
    }

    // そうでなければ数値のはず
    return new_node_num(expect_number());
}