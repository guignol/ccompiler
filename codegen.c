#include "9cc.h"

void ___COMMENT___(char *format, ...)
{
	va_list ap;
	va_start(ap, format);

	printf("  # ");
	vfprintf(stdout, format, ap);
	printf("\n");
}

void error(char *message)
{
	fprintf(stderr, "%s", message);
	exit(1);
}

void gen_lval(Node *node)
{
	if (node->kind != ND_LVAR)
		error("代入の左辺値が変数ではありません");
	printf("  mov rax, rbp\n");
	printf("  sub rax, %d\n", node->offset);
	printf("  push rax # variable [%s]\n", node->debug_name);
}

void gen(Node *node);

void generate(Node node[])
{
	gen(node);
}

int labelseq = 0;

void gen(Node *node)
{
	switch (node->kind)
	{
	case ND_BLOCK:
	{
		___COMMENT___("block begin");
		gen(node->statement);
		for (Node *next = node->statement->statement;
			 next;
			 next = next->statement)
		{
			printf("  pop rax\n");
			gen(next);
		}
		___COMMENT___("block end");
		return;
	}
	case ND_FOR:
	{
		___COMMENT___("for begin");
		int seq = labelseq++;
		// init
		if (node->lhs)
			gen(node->lhs);
		// begin
		printf(".Lbegin%d:\n", seq);
		// condition
		if (node->condition)
		{
			gen(node->condition);
			printf("  pop rax\n");
			printf("  cmp rax, 0\n");
			// if 0, goto end
			printf("  je  .Lend%d\n", seq);
		}
		// execute
		gen(node->execution);
		// post execute
		if (node->rhs)
		{
			gen(node->rhs);
		}
		// goto begin
		printf("  jmp .Lbegin%d\n", seq);
		// end:
		printf(".Lend%d:\n", seq);
		___COMMENT___("for end");
		return;
	}
	case ND_WHILE:
	{
		___COMMENT___("while begin");
		int seq = labelseq++;
		// begin:
		printf(".Lbegin%d:\n", seq);
		// condition
		gen(node->condition);
		printf("  pop rax\n");
		printf("  cmp rax, 0\n");
		// if 0, goto end
		printf("  je  .Lend%d\n", seq);
		// execute & goto begin
		gen(node->lhs);
		printf("  jmp .Lbegin%d\n", seq);
		// end:
		printf(".Lend%d:\n", seq);
		___COMMENT___("while end");
		return;
	}
	case ND_IF:
	{
		int seq = labelseq++;
		if (node->rhs)
		{
			gen(node->condition);
			printf("  pop rax\n");
			printf("  cmp rax, 0\n");
			printf("  je  .Lelse%d\n", seq);
			gen(node->lhs);
			printf("  jmp .Lend%d\n", seq);
			printf(".Lelse%d:\n", seq);
			gen(node->rhs);
			printf(".Lend%d:\n", seq);
		}
		else
		{
			gen(node->condition);
			printf("  pop rax\n");
			printf("  cmp rax, 0\n");
			printf("  je  .Lend%d\n", seq);
			gen(node->lhs);
			printf(".Lend%d:\n", seq);
		}
		return;
	}
	case ND_RETURN:
		gen(node->lhs);
		printf("  pop rax\n");
		printf("  mov rsp, rbp\n");
		printf("  pop rbp\n");
		printf("  ret\n");
		return;
	case ND_NUM:
		printf("  push %d\n", node->val);
		return;
	case ND_LVAR:
		gen_lval(node);
		printf("  pop rax\n");
		printf("  mov rax, [rax]\n");
		printf("  push rax\n");
		return;
	case ND_ASSIGN:
		___COMMENT___("assign begin");
		gen_lval(node->lhs);
		gen(node->rhs);

		printf("  pop rdi\n");
		printf("  pop rax\n");
		printf("  mov [rax], rdi\n");
		printf("  push rdi\n");
		___COMMENT___("assign end");
		return;
	}

	// 2項演算
	gen(node->lhs);
	gen(node->rhs);
	printf("  pop rdi\n");
	printf("  pop rax\n");

	switch (node->kind)
	{
	case ND_ADD:
		printf("  add rax, rdi\n");
		break;
	case ND_SUB:
		printf("  sub rax, rdi\n");
		break;
	case ND_MUL:
		printf("  imul rax, rdi\n");
		break;
	case ND_DIV:
		printf("  cqo\n");
		printf("  idiv rdi\n");
		break;
	case ND_EQL:
		printf("  cmp rax, rdi");
		___COMMENT___("==");
		printf("  sete al\n");
		printf("  movzb rax, al\n");
		break;
	case ND_NOT:
		printf("  cmp rax, rdi");
		___COMMENT___("!=");
		printf("  setne al\n");
		printf("  movzb rax, al\n");
		break;
	case ND_LESS:
		printf("  cmp rax, rdi");
		___COMMENT___("<");
		printf("  setl al\n");
		printf("  movzb rax, al\n");
		break;
	case ND_LESS_EQL:
		printf("  cmp rax, rdi");
		___COMMENT___("<=");
		printf("  setle al\n");
		printf("  movzb rax, al\n");
		break;
	}

	printf("  push rax\n");
}