#include "9cc.h"

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "引数の個数が正しくありません\n");
        return 1;
    }

    char *input = argv[1];
    Token *token = tokenize(input);
    struct Program *prog = program(token);

    int label_suffix = -1;
    printf(".intel_syntax noprefix\n");
    {
        for (Global *global = prog->globals; global; global = global->next) {
            // ラベル
            printf("%.*s:\n", global->label_length, global->label);
            directive_target *target = global->target;
            switch (global->directive) {
                case _zero:
                    printf("  .zero %d\n", target->value);
                    break;
                case _long:
                    printf("  .long %d\n", target->value);
                    break;
                case _quad:
                    printf("  .quad %.*s\n", target->label_length, target->label);
                    break;
                case _string: {
                    label_suffix++;
                    printf("  .quad .LC%d\n", label_suffix);
                    printf(".LC%d:\n", label_suffix);
                    printf("  .string \"%.*s\"\n", target->literal_length, target->literal);
                    break;
                }
            }
        }
    }
    generate(prog->functions);

    return 0;
}

void error(char *message) {
    fprintf(stderr, "%s", message);
}