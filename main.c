#include "9cc.h"

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "引数の個数が正しくありません\n");
        return 1;
    }

    char *input = argv[1];
//    input = "int main() { \
//              int a[2][3]; \
//              (a[0])[1] = 12; \
//              return (a[0])[1]; \
//            }";
    Token *token = tokenize(input);
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