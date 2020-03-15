#include "common.h"

// 指定されたファイルの内容を返す
// https://www.sigbus.info/compilerbook#ステップ26-入力をファイルから読む
char *read_file(char *path) {
    // ファイルを開く
    /** FILE */ void *fp = fopen(path, "r");
    if (!fp) {
        const int err_num = *__errno_location();
        error_2("cannot open %s: %s", path, strerror(err_num));
        exit(err_num);
    }

    // ファイルの長さを調べる
    if (fseek(fp, 0, SEEK_END) == -1) {
        const int err_num = *__errno_location();
        error_2("%s: fseek: %s", path, strerror(err_num));
        exit(err_num);
    }
    /** size_t */ int size = ftell(fp);
    if (fseek(fp, 0, SEEK_SET) == -1) {
        const int err_num = *__errno_location();
        error_2("%s: fseek: %s", path, strerror(err_num));
        exit(err_num);
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
    struct Program *prog = parse(token);
    generate(prog);

    return 0;
}

void error(const char *message) {
    fprintf(stderr, "%s", message);
}

void error_2(const char *fmt, const char *arg_1, const char *arg_2) {
    fprintf(stderr, fmt, arg_1, arg_2);
}
