#include "9cc.h"

// 現在着目しているトークン
Token *token;
// ローカル変数
Variable *locals;

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
Variable *find_local_variable(Token *tok)
{
	for (Variable *var = locals; var; var = var->next)
		if (var->len == tok->len && !memcmp(tok->str, var->name, var->len))
			return var;
	return NULL;
}

char *copy(char *str, int len)
{
	char *copied = malloc(sizeof(char *));
	strncpy(copied, str, len);
	return copied;
}

// program    = function*
// function   = ident "(" ident* ")" { stmt* }
// stmt       = expr ";"
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
// unary      = ("+" | "-")? primary
// primary    = num |
//				| ident
//				| ident "(" args ")"
// 				| "(" expr ")"
// args       = (expr ("," expr)* )?

Node *stmt();
Node *expr();
Node *assign();
Node *equality();
Node *relational();
Node *add();
Node *mul();
Node *unary();
Node *primary();

Function *program(Token *tok)
{
	token = tok;

	Token *function_name = consume_ident();
	if (!function_name)
		error_at(token->str, "関数名がありません");

	expect("(");
	// TODO 引数
	expect(")");

	expect("{");
	// Node *code[100];
	Node **body = (Node **)malloc(sizeof(Node *) * 100);
	int i = 0;
	while (!consume("}"))
	{
		body[i++] = stmt();
	}

	if (!at_eof())
		error_at(token->str, "何かが残っています？");

	body[i] = NULL;

	Function *function = malloc(sizeof(Function*));
	function->name = copy(function_name->str, function_name->len);
	function->parameters = NULL;
	function->locals = locals;
	function->body = body;

	return function;
}

Node *stmt()
{
	Node *node;
	if (consume("{"))
	{
		node = calloc(1, sizeof(Node));
		node->kind = ND_BLOCK;
		Node *last = node;
		while (!consume("}"))
		{
			Node *next = stmt();
			last->statement = next;
			last = next;
		}
		return node;
	}

	if (consume("if"))
	{
		node = calloc(1, sizeof(Node));
		node->kind = ND_IF;
		expect("(");
		node->condition = expr();
		expect(")");
		node->lhs = stmt();
		node->rhs = consume("else") ? stmt() : NULL;
		return node;
	}
	else if (consume("while"))
	{
		node = calloc(1, sizeof(Node));
		node->kind = ND_WHILE;
		expect("(");
		node->condition = expr();
		expect(")");
		node->lhs = stmt();
		return node;
	}
	else if (consume("for"))
	{
		node = calloc(1, sizeof(Node));
		node->kind = ND_FOR;
		expect("(");
		// init
		if (!consume(";"))
		{
			node->lhs = expr();
			expect(";");
		}
		// condition
		if (!consume(";"))
		{
			node->condition = expr();
			expect(";");
		}
		// increment
		if (!consume(")"))
		{
			node->rhs = expr();
			expect(")");
		}
		node->execution = stmt();
		return node;
	}
	else if (consume("return"))
	{
		node = calloc(1, sizeof(Node));
		node->kind = ND_RETURN;
		node->lhs = expr();
	}
	else
	{
		node = expr();
	}
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
		if (consume("("))
		{
			node->kind = ND_FUNC;
			if (!consume(")"))
			{
				Node *last = node;
				last->args = expr();
				while (consume(",") && (last = last->args))
				{
					last->args = expr();
				}
				expect(")");
			}
		}
		else
		{
			node->kind = ND_LVAR;
			Variable *lvar = find_local_variable(tok);
			if (lvar)
			{
				node->offset = lvar->offset;
			}
			else
			{
				lvar = calloc(1, sizeof(Variable));
				lvar->next = locals;
				lvar->name = tok->str;
				lvar->len = tok->len;

				lvar->offset = (locals ? locals->offset : 0) + 8;
				node->offset = lvar->offset;
				locals = lvar;
			}
		}
		node->name = copy(tok->str, tok->len);
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