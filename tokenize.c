#include "9cc.h"

// 入力プログラム
char *user_input;

// エラー箇所を報告する
void error_at(char *loc, char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);

	int pos = loc - user_input;
	fprintf(stderr, "%s\n", user_input);
	fprintf(stderr, "%*s", pos, ""); // pos個の空白を出力
	fprintf(stderr, "^ ");
	vfprintf(stderr, fmt, ap);
	fprintf(stderr, "\n");
	exit(1);
}

bool startswith(char *p, char *q)
{
	return memcmp(p, q, strlen(q)) == 0;
}

bool is_alpha(char c)
{
	return ('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z') || c == '_';
}

bool is_alnum(char c)
{
	return is_alpha(c) || ('0' <= c && c <= '9');
}

// 新しいトークンを作成してcurに繋げる
Token *new_token(TokenKind kind, Token *cur, char *str, int len)
{
	Token *tok = calloc(1, sizeof(Token));
	tok->kind = kind;
	tok->str = str;
	tok->len = len;
	cur->next = tok;
	return tok;
}

int reserved(char *p)
{
	// Keyword
	static char *kws[] = {"return", "if", "else"};
	for (int i = 0; i < sizeof(kws) / sizeof(*kws); i++)
	{
		char *keyword = kws[i];
		int len = strlen(keyword);
		char next = p[len];
		if (strncmp(p, keyword, len) == 0 && !is_alnum(next))
			return len;
	}

	// Multi-letter punctuator
	static char *ops[] = {"==", "!=", "<=", ">="};
	for (int i = 0; i < sizeof(ops) / sizeof(*ops); i++)
	{
		char *operator= ops[i];
		int len = strlen(operator);
		char next = p[len];
		if (strncmp(p, operator, len) == 0)
			return len;
	}

	// Single-letter punctuator
	if (strchr("+-*/()<>;=", *p))
		return 1;

	return 0;
}

// 入力文字列pをトークナイズしてそれを返す
Token *tokenize(char *p)
{
	user_input = p;
	Token head;
	head.next = NULL;
	Token *cur = &head;

	while (*p)
	{
		// 空白文字をスキップ
		if (isspace(*p))
		{
			p++;
			continue;
		}

		int len = reserved(p);
		if (0 < len)
		{
			cur = new_token(TK_RESERVED, cur, p, len);
			p += len;
			continue;
		}

		if (is_alpha(*p))
		{
			char *q = p++;
			while (is_alnum(*p))
				p++;
			cur = new_token(TK_IDENT, cur, q, p - q);
			continue;
		}

		// Integer literal
		if (isdigit(*p))
		{
			cur = new_token(TK_NUM, cur, p, 0);
			char *q = p;
			cur->val = strtol(p, &p, 10);
			cur->len = p - q;
			continue;
		}

		error_at(p, "トークナイズできません");
	}

	new_token(TK_EOF, cur, p, 0);
	return head.next;
}