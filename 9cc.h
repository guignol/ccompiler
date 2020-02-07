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
    size_t len;        // トークンの長さ
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
    ND_VARIABLE,    // ローカル変数
    ND_VARIABLE_ARRAY, // ローカル変数（配列）
    ND_ADDRESS,     // &a
    ND_DEREF,       // *a
    /*
     * int (*b)[2];
     * (*b)[1] = 1;
     *   ↑
     *   ここの話
     */
            ND_DEREF_ARRAY_POINTER, // *a
    ND_INDEX, // a[0][1]
    /*
     * 配列の配列の場合
     *
     * int a[2][3];
     * a[1][2] = 1;
     *   ↑
     *   ここの話
     *
     * *(( *(a          + 1     ) ) + 2    ) = 1;
     * *((  ([PBP - 24] + 1 * 12) ) + 2 * 4) = 1;
     * 途中はデリファレンスせずにそのままポインタの足し算になる
     * ただし、型チェックのためASTにはデリファレンスであることを示す必要がある
     *
     */
            ND_INDEX_CONTINUE, // a[0][1]
    ND_ASSIGN,      // =
    ND_NOTHING      // 変数宣言
} NodeKind;

typedef struct Node Node;
typedef struct Type Type;

// 抽象構文木のノードの型
struct Node {
    NodeKind kind; // ノードの種別
    Node *lhs;     // 左辺
    Node *rhs;     // 右辺
    int val;       // kindがND_NUMの場合のみ使う

    // ND_VARIABLEの場合のみ
    Type *type;

    int offset;    // 変数のRBPからのオフセット
    char *name; // 変数名、関数名
    int len;

    Node *condition; // if (condition), while (condition)
    Node *execution; // for (;;) statement
    Node *statement; // { ...statement }
    Node *args;         // function( ...args )
};


/////////////////////////////////////////////////

struct Type {
    enum {
        TYPE_INT, // 4byte
        TYPE_POINTER, // 8byte
        TYPE_ARRAY, // array_size * sizeof(point_to) byte
    } ty;
    Type *point_to;

    /**
     * sizeofか単項&のオペランドとして使われるとき以外、配列は、
     * その配列の先頭要素を指すポインタに暗黙のうちに変換されるということになっているのです。
     * [中略]
     * Cにおいては配列アクセスのための[]演算子というものはありません。
     * Cの[]は、ポインタ経由で配列の要素にアクセスするための簡便な記法にすぎないのです。
     * https://www.sigbus.info/compilerbook#%E3%82%B9%E3%83%86%E3%83%83%E3%83%9721-%E9%85%8D%E5%88%97%E3%82%92%E5%AE%9F%E8%A3%85%E3%81%99%E3%82%8B
     */
    size_t array_size;
};

Type *shared_int_type();

Type *create_pointer_type(Type *point_to);

Type *create_array_type(Type *element_type, int array_size);

bool are_same_type(Type *left, Type *right);

bool are_assignable_type(Type *left, Type *right);

Type *find_type(const Node *node);

int get_weight(Node *node);

int get_size(Type *type);

bool type_32bit(Type *type);

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