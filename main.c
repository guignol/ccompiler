#include "9cc.h"

// https://www.sigbus.info/compilerbook#ステップ26-入力をファイルから読む

// 指定されたファイルの内容を返す
char *read_file(char *path) {
    // ファイルを開く
    FILE *fp = fopen(path, "r");
    if (!fp) {
        error("cannot open %s: %s", path, strerror(errno));
    }

    // ファイルの長さを調べる
    if (fseek(fp, 0, SEEK_END) == -1) {
        error("%s: fseek: %s", path, strerror(errno));
    }
    size_t size = ftell(fp);
    if (fseek(fp, 0, SEEK_SET) == -1) {
        error("%s: fseek: %s", path, strerror(errno));
    }

    // ファイル内容を読み込む
    char *buf = calloc(1, size + 2);
    fread(buf, size, 1, fp);

    // ファイルが必ず"\n\0"で終わっているようにする
    if (size == 0 || buf[size - 1] != '\n') {
        buf[size++] = '\n';
    }
    buf[size] = '\0';
    fclose(fp);
    return buf;
}

int main(int argc, char **argv) {
    char *input;
    if (argc == 2) {
        input = argv[1];
    } else if (argc == 3) {
        input = read_file(argv[2]);
    } else {
        error("引数の個数が正しくありません\n");
        return 1;
    }

    Token *token = tokenize(input);
    struct Program *prog = program(token);

    printf(".intel_syntax noprefix\n");
    {
        if (prog->globals) {
            printf("\n");
            printf(".data\n");
        }
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
                    printf("  .string \"%.*s\"\n", target->literal_length, target->literal);
                    break;
                }
            }
        }
    }
    printf("\n");
    printf(".text\n");
    generate(prog->functions);

    return 0;
}

void error(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
}