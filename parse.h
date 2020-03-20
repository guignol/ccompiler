#include "common.h"


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
//				| ("+" | "-" | "*"* | "&" | "!")? primary
// primary    = literal_str
//				| ident "(" args? ")"
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

void global_variable_declaration(Token *variable_name, Type *type);

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

////////////////////////////////////////////////////////////////// switch.c

struct Case *consume_case();