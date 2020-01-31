#include "9cc.h"

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "引数の個数が正しくありません\n");
        return 1;
    }

    Token *token = tokenize(argv[1]);
    Function *function = program(token);

    printf(".intel_syntax noprefix\n");
    {
        printf(".LC0:\n");
        printf("  .string \"%s\"\n", "moji: %i\\n");
        printf("debug_moji: \n");
        printf("  .quad .LC0\n");
    }
    generate(function);

    return 0;
}

void error(char *message) {
    fprintf(stderr, "%s", message);
}