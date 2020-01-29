#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void error(char *message);
void error_at(const char *loc, const char *fmt, ...);

// トークンの種類
typedef enum {
    TK_RESERVED, // 記号
    TK_IDENT,    // 識別子
    TK_NUM,         // 整数トークン
    TK_EOF,         // 入力の終わりを表すトークン
} TokenKind;

typedef struct Token Token;

// トークン型
struct Token {
    TokenKind kind; // トークンの型
    Token *next;    // 次の入力トークン
    int val;        // kindがTK_NUMの場合、その数値
    char *str;        // トークン文字列
    int len;        // トークンの長さ
};

Token *tokenize(char *p);

/////////////////////////////////////////////////

// 抽象構文木のノードの種類
typedef enum {
    ND_ADD,         // +
    ND_SUB,         // -
    ND_MUL,         // *
    ND_DIV,         // /
    ND_EQL,         // ==
    ND_NOT,         // !=
    ND_LESS,        // <
    ND_LESS_EQL,    // <=
    ND_RETURN,      // return
    ND_EXPR_STMT,   // 式文
    ND_IF,          // if
    ND_WHILE,       // while
    ND_FOR,         // for
    ND_BLOCK,       // { }
    ND_FUNC,        // 関数コール : 今のところintのみ
    ND_NUM,         // 整数 : int
    ND_VARIABLE,    // ローカル変数 : 変数宣言を見れば分かる
    ND_ADDRESS,     // &a : ノードの型を見れば分かる
    ND_DEREF,       // *a : ノードの型を見れば分かる
    ND_ASSIGN,      // = : ノードの型を見れば分かる
    ND_NOTHING      // 変数宣言
} NodeKind;

typedef struct Node Node;

// 抽象構文木のノードの型
struct Node {
    NodeKind kind; // ノードの種別
    Node *lhs;     // 左辺
    Node *rhs;     // 右辺
    int val;       // kindがND_NUMの場合のみ使う

    int offset;    // 変数のRBPからのオフセット
    char *name; // 変数名、関数名
    int len;

    Node *condition; // if (condition), while (condition)
    Node *execution; // for (;;) statement
    Node *statement; // { ...statement }
    Node *args;         // function( ...args )
};


/////////////////////////////////////////////////

typedef struct Type Type;

struct Type {
    enum {
        INT, // 4byte
        POINTER // 8byte
    } ty;
    Type *point_to;
};

typedef struct Variable Variable;

// 変数
struct Variable {
    Type *type;      // 型
    char *name;     // 変数の名前
    int len;        // 名前の長さ
    int offset;     // RBPからのオフセット

    int index; // 関数の仮引数の場合、何番目か

    Variable *next; // 次の変数かNULL
};

typedef struct Function Function;

struct Function {
    // 返り値の型
    // Type *type; // TODO 関数フレーム内で確保した値へのポインタをそのまま返せない
    // 関数名
    char *name;
    // ローカル変数および引数
    Variable *locals;
    // 関数本体
    Node **body;

    Function *next;
};

Function *program(Token *tok);

void generate(Function *func);