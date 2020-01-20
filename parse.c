#include "9cc.h"

// 現在着目しているトークン
Token *token;

// 次のトークンが期待している記号のときには、トークンを読み進めて
// 真を返す。それ以外の場合には偽を返す。
bool consume(char *op)
{
	if (token->kind != TK_RESERVED ||
		strlen(op) != token->len ||
		memcmp(token->str, op, token->len))
		return false;
	token = token->next;
	return true;
}

Token *consume_ident()
{
	if (token->kind == TK_IDENT)
	{
		Token *t = token;
		token = token->next;
		return t;
	}
	return NULL;
}

// 次のトークンが期待している記号のときには、トークンを1つ読み進める。
// それ以外の場合にはエラーを報告する。
void expect(char *op)
{
	if (token->kind != TK_RESERVED ||
		strlen(op) != token->len ||
		memcmp(token->str, op, token->len))
		error_at(token->str, "'%s'ではありません", op);
	token = token->next;
}

// 次のトークンが数値の場合、トークンを1つ読み進めてその数値を返す。
// それ以外の場合にはエラーを報告する。
int expect_number()
{
	if (token->kind != TK_NUM)
		error_at(token->str, "数ではありません");
	int val = token->val;
	token = token->next;
	return val;
}

bool at_eof()
{
	return token->kind == TK_EOF;
}

//////////////////////////////////////////////////////////////////

Node *new_node(NodeKind kind, Node *lhs, Node *rhs)
{
	Node *node = calloc(1, sizeof(Node));
	node->kind = kind;
	node->lhs = lhs;
	node->rhs = rhs;
	return node;
}

Node *new_node_num(int val)
{
	Node *node = calloc(1, sizeof(Node));
	node->kind = ND_NUM;
	node->val = val;
	return node;
}

// 変数を名前で検索する。見つからなかった場合はNULLを返す。
LVar *find_lvar(Token *tok) {
  for (LVar *var = locals; var; var = var->next)
    if (var->len == tok->len && !memcmp(tok->str, var->name, var->len))
      return var;
  return NULL;
}

// program    = stmt*
// stmt       = expr ";"
// expr       = assign
// assign     = equality ("=" assign)?
// equality   = relational ("==" relational | "!=" relational)*
// relational = add ("<" add | "<=" add | ">" add | ">=" add)*
// add        = mul ("+" mul | "-" mul)*
// mul        = unary ("*" unary | "/" unary)*
// unary      = ("+" | "-")? primary
// primary    = num | ident | "(" expr ")"

Node *stmt();
Node *expr();
Node *assign();
Node *equality();
Node *relational();
Node *add();
Node *mul();
Node *unary();
Node *primary();

void program(Token *tok, Node *code[])
{
	token = tok;
	// return stmt();
	int i = 0;
	while (!at_eof())
		code[i++] = stmt();
	code[i] = NULL;
}

Node *stmt()
{
	Node *node = expr();
	expect(";");
	return node;
}

Node *expr()
{
	return assign();
}

Node *assign()
{
	Node *node = equality();
	if (consume("="))
		node = new_node(ND_ASSIGN, node, assign());
	return node;
}

Node *equality()
{
	Node *node = relational();

	for (;;)
	{
		if (consume("=="))
			node = new_node(ND_EQL, node, relational());
		else if (consume("!="))
			node = new_node(ND_NOT, node, relational());
		else
			return node;
	}
}

Node *relational()
{
	Node *node = add();

	for (;;)
	{
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

Node *add()
{
	Node *node = mul();

	for (;;)
	{
		if (consume("+"))
			node = new_node(ND_ADD, node, mul());
		else if (consume("-"))
			node = new_node(ND_SUB, node, mul());
		else
			return node;
	}
}

Node *mul()
{
	Node *node = unary();

	for (;;)
	{
		if (consume("*"))
			node = new_node(ND_MUL, node, unary());
		else if (consume("/"))
			node = new_node(ND_DIV, node, unary());
		else
			return node;
	}
}

Node *unary()
{
	if (consume("+"))
		return primary();
	if (consume("-"))
		return new_node(ND_SUB, new_node_num(0), primary());
	return primary();
}

Node *primary()
{
	Token *tok = consume_ident();
	if (tok)
	{
		Node *node = calloc(1, sizeof(Node));
		node->kind = ND_LVAR;

		LVar *lvar = find_lvar(tok);
		if (lvar)
		{
			node->offset = lvar->offset;
		}
		else
		{
			lvar = calloc(1, sizeof(LVar));
			lvar->next = locals;
			lvar->name = tok->str;
			lvar->len = tok->len;

			lvar->offset = (locals ? locals->offset : 0) + 8;
			node->offset = lvar->offset;
			locals = lvar;
		}
		return node;
	}

	// 次のトークンが"("なら、"(" expr ")"のはず
	if (consume("("))
	{
		Node *node = expr();
		expect(")");
		return node;
	}

	// そうでなければ数値のはず
	return new_node_num(expect_number());
}