#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

void error(const char *fmt, ...);

void error_at(const char *loc, const char *fmt, ...);

void warn_at(const char *loc, const char *fmt, ...);

extern bool warning;

// トークンの種類
typedef enum {
    TK_RESERVED,    // 記号
    TK_IDENT,       // 識別子
    TK_CHAR_LITERAL, // 文字
    TK_STR_LITERAL, // 文字列
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
    ND_FUNC,        // 関数コール
    ND_NUM,         // 整数 : int
    ND_STR_LITERAL, // 文字列リテラル
    ND_VARIABLE,    // ローカル変数
    ND_VARIABLE_ARRAY, // ローカル変数（配列）
    ND_ADDRESS,     // &a
    /*
     * int a;
     * *a;
     *
     * int a[1][2];
     * a[0][1]
     *      ↑
     */
    ND_DEREF,
    /*
     * int (*b)[2];
     * (*b)[1] = 1;
     *   ↑
     *
     * int a[2][3];
     * a[1][2] = 1;
     *   ↑
     *
     * *(( *(a          + 1     ) ) + 2    ) = 1;
     * *((  ([RBP - 24] + 1 * 12) ) + 2 * 4) = 1;
     * 途中はデリファレンスせずにそのままポインタの足し算になる
     */
    ND_DEREF_ARRAY_POINTER, // *a
    ND_ASSIGN,      // =
    ND_NOTHING,      // 変数宣言（初期化なし）
} NodeKind;

typedef struct Node Node;
typedef struct Type Type;

// 抽象構文木のノードの型
struct Node {
    NodeKind kind; // ノードの種別
    Node *lhs;     // 左辺
    Node *rhs;     // 右辺
    int val;       // kindがND_NUMの場合のみ使う

    // 変数または関数の返り値の型
    Type *type;
    bool is_local;
    char *name; // 変数名、関数名
    int len;
    int offset;    // ローカル変数のRBPからのオフセット

    // 文字列リテラルへの参照
    char *label;
    int label_length;

    Node *condition; // if (condition), while (condition)
    Node *execution; // for (;;) statement
    Node *statement; // { ...statement }
    Node *args;         // function( ...args )
};

typedef struct NodeArray NodeArray;
struct NodeArray {
    Node **memory;
    int count;
    int capacity;
};

/////////////////////////////////////////////////

struct Type {
    enum {
        TYPE_VOID,
        TYPE_CHAR, // 1byte
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

typedef enum {
    CANNOT_ASSIGN = 0,
    AS_SAME,
    AS_INCOMPATIBLE,
} Assignable;

Type *shared_void_type();

Type *shared_char_type();

Type *shared_int_type();

Type *create_pointer_type(Type *point_to);

Type *create_array_type(Type *element_type, int array_size);

Assignable are_assignable_type(Type *left, Type *right, bool r_zero);

Type *find_type(const Node *node);

int get_weight(Node *node);

int get_size(Type *type);

int get_element_count(Type *type);

void print_type(FILE *__stream, Type *type);

void warn_incompatible_type(Type *left, Type *right);

/////////////////////////////////////////////////

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
typedef struct Global Global;

struct Function {
    // 関数名
    char *name;
    // 引数
    Variable *parameters;
    // 関数本体
    NodeArray *body;
    // スタックのサイズ
    int stack_size;

    Function *next;
};

struct Program {
    Function *functions;
    Global *globals;
};

struct Program *parse(Token *tok);

void generate(struct Program *program);

/////////////////////////////////////////////////


//char c;
//char c_c[2];
//int x;
//int x_x = 2;
//int *y[20];
//int *yy;
//int *yy_yy = &x;
//char *moji = "moji";

// c:
//  .zero 1
// c_c:
//  .zero 2
// x:
//  .zero 4
// x_x:
//  .long 2
// y:
//  .zero 160
// yy:
//  .zero 8
// yy_yy:
//  .quad x
// .LC0:
//  .string "moji"
// moji:
//  .quad .LC0
// .LC1:
//  .string "%s"

typedef enum {
    /*
     * -fno-commonが無いと、gccやclangは .zero ではなく .comm を使うらしい
     *   -fno-common
     *    このオプションを指定した場合、未初期化の大域変数は .data セクションに配置されます。（-fcommon の解説を参照。）
     *    必ず 1 つの大域変数だけが非 extern 宣言で、他は全て extern 宣言である必要があります。
     *    （GCC や Clang コンパイラ、GNU ld リンカのデフォルト動作に依存しない、
     *    できる限り移植性のあるコードを書く必要がある場合に指定します。）
     *   -fcommon
     *    変数を ELF ファイル中のどのセクションに配置するかは、C の規格には含まれていません。
     *    多くのコンパイラでは、初期値のある大域変数は .data セクション、
     *    0 初期化された大域変数は bss セクションに配置されます。
     *    GCC と Clang は、未初期化の大域変数をとりあえず common セクションに配置し、
     *    GNU ld リンカ（およびその互換リンカ）が最後にまとめる（多重定義エラーにならない）という処理を行います。（デフォルト）
     *   http://solid.kmckk.com/doc/skit/current/solid_toolchain/overview.html
     */
    _zero,
    /*
     * .comm name, size, alignment
     *   The .comm directive allocates storage in the data section.
     *   The storage is referenced by the identifier name.
     *   Size is measured in bytes and must be a positive integer.
     *   Name cannot be predefined. Alignment is optional.
     *   If alignment is specified, the address of name is aligned to a multiple of alignment.
     *   https://docs.oracle.com/cd/E26502_01/html/E28388/eoiyg.html
     */
//    _comm,
    _byte, //　(1 byte) char
    _long, // (4 byte) int
    _quad, // (8 byte) pointer or size_t
    _string,
} DIRECTIVE;

typedef struct Directives Directives;

struct Directives {
    DIRECTIVE directive; // http://web.mit.edu/gnu/doc/html/as_7.html

    // _zero, _byte, _long
    int value;
    // _quad
    char *reference;
    int reference_length;
    // _string
    char *literal;
    int literal_length;

    Directives *next;
};

struct Global {
    Type *type;
    char *label;
    int label_length;
    Directives *target;

    Global *next;
};