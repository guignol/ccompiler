#include "9cc.h"

int main(int argc, char **argv)
{
	if (argc != 2)
	{
		fprintf(stderr, "引数の個数が正しくありません\n");
		return 1;
	}

	Token *token = tokenize(argv[1]);
	// Node *code[100];
	Node **code = (Node **)malloc(sizeof(Node *) * 100);
	Variable *locals;
	program(token, code, &locals);

	printf(".intel_syntax noprefix\n");
	{
		printf(".LC0:\n");
		printf("  .string \"%s\"\n", "moji: %i\\n");
		printf("debug_moji: \n");
		printf("  .quad .LC0\n");
	}
	Function *function;
	function->name = "main";
	function->parameters = NULL;
	function->locals = locals;
	function->body = code;
	generate(function);
	
	return 0;
}