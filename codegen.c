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
	printf("  push rax # variable [%s]\n", node->name);
}

int labelseq = 0;

void gen(Node *node)
{
	switch (node->kind)
	{
	case ND_FUNC:
	{
		int count = 0;
		if (node->args)
		{
			// RAX	返り値 / 引数の数	✔
			// RDI	第1引数	✔
			// RSI	第2引数	✔
			// RDX	第3引数	✔
			// RCX	第4引数	✔
			// RBP	ベースポインタ
			// RSP	スタックポインタ
			// RBX	（特になし）
			// R8	第5引数	✔
			// R9	第6引数	✔
			static char *registers[] = {"rdi",
										"rsi",
										"rdx",
										"rcx",
										"r8",
										"r9"};
			for (Node *arg = node->args; arg; arg = arg->args)
			{
				gen(arg);
				count++;
			}
			for (size_t i = 0; i < count; i++)
			{

				printf("  pop %s\n", registers[count - i - 1]);
			}
		}
		else
		{
			// デバッグ用: 引数なしのprintf()に固定の文字列を渡す
			// スタックポインタのアラインなしだとクラッシュする場合があるのでその確認
			if (memcmp(node->name, "printf", strlen("printf")) == 0)
			{
				count = 2;
				printf("  mov rdi, QWORD PTR debug_moji[rip]\n");
				printf("  mov rsi, 2\n");
			}
		}
		static bool doAlign = true;
		if (doAlign)
		{
			// https://github.com/rui314/chibicc/commit/aedbf56c3af4914e3f183223ff879734683bec73
			// We need to align RSP to a 16 byte boundary before
			// calling a function because it is an ABI requirement.
			// RAX is set to 0 for variadic function.
			int seq = labelseq++;
			printf("  mov rax, rsp\n");
			printf("  and rax, 15\n");
			printf("  jnz .Lcall%d\n", seq);
			printf("  mov rax, 0\n");
			printf("  call %s\n", node->name);
			printf("  jmp .Lend%d\n", seq);
			printf(".Lcall%d:\n", seq);
			printf("  sub rsp, 8\n");
			printf("  mov rax, 0\n");
			printf("  call %s\n", node->name);
			printf("  add rsp, 8\n");
			printf(".Lend%d:\n", seq);
			printf("  push rax\n");
		}
		else
		{
			printf("  mov rax, %i\n", count);
			printf("  call %s\n", node->name);
			printf("  push rax\n");
		}

		return;
	}
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

void generate(Function *func) {
	char *function_name = func->name;
	printf(".global %s\n", function_name);
	printf("%s:\n", function_name);
	Variable *locals = func->locals;

	// プロローグ
	printf("  push rbp\n");
	printf("  mov rbp, rsp\n");
	if (locals)
	{
		printf("  sub rsp, %i  # %i %s\n", locals->offset, locals->offset / 8, "variables prepared");

		// xやyといったローカル変数が存在するものとしてコンパイルして、
		// 関数のプロローグの中で、レジスタの値をそのローカル変数のためのスタック上の領域に書き出してください。
		// そうすれば、その後は特に引数とローカル変数を区別することなく扱えるはずです。
		// https://www.sigbus.info/compilerbook#%E3%82%B9%E3%83%86%E3%83%83%E3%83%9715-%E9%96%A2%E6%95%B0%E3%81%AE%E5%AE%9A%E7%BE%A9%E3%81%AB%E5%AF%BE%E5%BF%9C%E3%81%99%E3%82%8B

		// 使っていればローカル変数と同じようにparseされてるはず？
		// だとすると、スタックは確保できてるので書き込むだけ？
	}

	for (size_t i = 0; i < 100; i++)
	{
		Node *node = func->body[i];
		if (node == NULL)
			break;

		// 抽象構文木を下りながらコード生成
		gen(node);
		// 式の評価結果としてスタックに一つの値が残っている
		// はずなので、スタックが溢れないようにポップしておく
		printf("  pop rax\n");
	}

	// エピローグ
	// 最後の式の結果がRAXに残っているのでそれが返り値になる
	printf("  mov rsp, rbp\n");
	printf("  pop rbp\n");
	printf("  ret\n");
}