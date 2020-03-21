#include "common.h"

extern Token *token;

///////////////////////// https://en.wikipedia.org/wiki/Order_of_operations

// program    = (function | global_var)*
// global_var = (decl_b | decl_c) ";"
// function   = decl_a "(" decl_a? ("," decl_a)* ")" { stmt* }
// stmt       = expr ";"
//              | (decl_b | decl_c) ";"
//				| "{" stmt* "}"
//				| "return" expr ";"
//				| "break" ";"
//				| "if" "(" expr ")" stmt ("else" stmt)?
//		        | "while" "(" expr ")" stmt
//				| "for" "(" (expr | decl_b | decl_c)? ";" expr? ";" expr? ")" stmt
//				| "switch" "(" expr ")" "{" ("case" ident ":" stmt)* "}" // TODO {}は必須ではないけど
// expr       = assign
// assign     = ternary ("=" assign)?
// ternary    = logical ("?" expr ":" expr)?
// logical    = equality ("||" equality | "&&" equality)*
// equality   = relational ("==" relational | "!=" relational)*
// relational = add ("<" add | "<=" add | ">" add | ">=" add)*
// add        = mul ("+" mul | "-" mul)*
// mul        = unary ("*" unary | "/" unary　| "%" unary)*
// unary      = "sizeof" unary
//				| ("+" | "++" | "-" | "*"* | "&" | "!")? primary
// primary    = literal_str
//				| ident "(" args? ")"
//				| ident ("++")?
// 				| "(" expr ")"
// 				| "({" stmt "})"
//				| (primary | num_char) ( "(" index ")" )? index*
//				| primary ("." ident )*
// index      = "[" primary "]"
// args       = expr ("," expr)*
// decl_a     = ("int" | "char" | "struct" | "enum") "*"* (pointed_id | ident)
// decl_b     = decl_a ("[" num_char "]")* TODO 構造体とenumの宣言
// decl_c     = decl_a "[]"? ("[" num_char? "]")* "=" (array_init | expr)
// pointed_id = "(" "*"* ident ")"
// ident      =
// literal_str
// num_char

Function *function(Type *return_type);

Node *stmt(void);

Node *expr(void);

Node *assign(void);

Node *ternary(void);

Node *logical(void);

Node *equality(void);

Node *relational(void);

Node *add(void);

Node *mul(void);

Node *unary(void);

Node *primary(void);

//////////////////////////////////////////////////////////////////

Token *consume(char *op);

Token *consume_ident();


void expect(char *op);

int expect_num_char();

bool check(char *op);

//////////////////////////////////////////////////////////////////

Node *new_node(enum NodeKind kind, Node *lhs, Node *rhs);

Node *new_node_num(int val);

Node *pointer_calc(enum NodeKind kind, Node *left, Node *right);

Node *new_node_assign(char *loc, Node *lhs, Node *rhs);

void assert_assignable(char *loc,
                       Type *left_type,
                       Node *rhs);

Node *array_initializer(Node *array_variable, Type *type);

////////////////////////////////////////////////////////////////// array.c

NodeArray *create_node_array(int capacity);

NodeArray *push_node(NodeArray *array, Node *node);

Node *with_index(Node *left);

////////////////////////////////////////////////////////////////// enum.c

typedef struct {
    Global **memory;
    int count;
    int capacity;
} EnumMembers;

struct ENUM_INFO {
    const char *type_name;
    int name_length;
    // 宣言のみの時点では空（だけど定義必須にしておいてもいいかも）
    EnumMembers *members;
};

EnumMembers *create_enum_member(int capacity);

bool consume_enum_def(Type *base);

////////////////////////////////////////////////////////////////// global.c

void init_globals();

void add_globals(Global *next);

Global *get_globals();

Global *find_global_variable_by_name(char *name, int len);

Global *find_enum_member(char *name, int len);

Directives *global_initializer(char *loc, Type *type, Node *node);

Node *new_node_string_literal();

Node *new_node_global_variable(char *str, int len);

void global_variable_declaration(Token *variable_name, Type *type);

////////////////////////////////////////////////////////////////// struct.c

void init_struct_registry();

void push_struct(STRUCT_INFO *info);

Variable *find_struct_member(Type *type, const char *name, int len);

////////////////////////////////////////////////////////////////// switch.c

struct Case *consume_case();

////////////////////////////////////////////////////////////////// type_def.c

void register_type_def(Type *type, Token *alias_t);

Type *find_alias(Token *alias_t);

//////////////////////////////////////////////////////////////////
