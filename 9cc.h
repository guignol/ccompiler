#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void error_at(char *loc, char *fmt, ...);

// トークンの種類
typedef enum
{
	TK_RESERVED, // 記号
	TK_IDENT,	// 識別子
	TK_NUM,		 // 整数トークン
	TK_EOF,		 // 入力の終わりを表すトークン
} TokenKind;

typedef struct Token Token;

// トークン型
struct Token
{
	TokenKind kind; // トークンの型
	Token *next;	// 次の入力トークン
	int val;		// kindがTK_NUMの場合、その数値
	char *str;		// トークン文字列
	int len;		// トークンの長さ
};

Token *tokenize(char *p);

/////////////////////////////////////////////////

// 抽象構文木のノードの種類
typedef enum
{
	ND_ADD,		 // +
	ND_SUB,		 // -
	ND_MUL,		 // *
	ND_DIV,		 // /
	ND_EQL,		 // ==
	ND_NOT,		 // !=
	ND_LESS,	 // <
	ND_LESS_EQL, // <=
	ND_ASSIGN,   // =
	ND_LVAR,	 // ローカル変数
	ND_FUNC,	 // 関数
	ND_NUM,		 // 整数
	ND_RETURN,   // return
	ND_IF,		 // if
	ND_WHILE,	// while
	ND_FOR,		 // for
	ND_BLOCK,	// { }
} NodeKind;

typedef struct Node Node;

// 抽象構文木のノードの型
struct Node
{
	NodeKind kind; // ノードの型
	Node *lhs;	 // 左辺
	Node *rhs;	 // 右辺
	int val;	   // kindがND_NUMの場合のみ使う
	int offset;	// kindがND_LVARの場合のみ使う
	char *name;	// 変数名、関数名

	Node *condition; // if (condition), while (condition)
	Node *execution; // for (;;) statement
	Node *statement; // { ...statement }
	Node *args;		 // function( ...args )
};

/////////////////////////////////////////////////

typedef struct Variable Variable;

// 変数の型
struct Variable
{
	Variable *next; // 次の変数かNULL
	char *name;		// 変数の名前
	int len;		// 名前の長さ
	int offset;		// RBPからのオフセット
};

typedef struct Function
{
	// 関数名
	char *name;
	// 引数
	Variable *parameters;
	// ローカル変数
	Variable *locals;
	// 関数本体
	Node **body;
} Function;

void program(Token *tok, Node *code[], Variable **local_variables);
void generate(Function *func);