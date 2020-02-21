#include "common.h"

// 入力プログラム
char *user_input;

int print_with_line_number(const char *loc) {
    // locが含まれている行の開始地点と終了地点を取得
    const char *line_begin = loc;
    while (user_input < line_begin && line_begin[-1] != '\n') {
        line_begin--;
    }
    const char *line_end = loc;
    while (*line_end != '\n') {
        line_end++;
    }

    // 見つかった行が全体の何行目なのかを調べる
    int line_num = 1;
    for (char *p = user_input; p < line_begin; p++) {
        if (*p == '\n') {
            line_num++;
        }
    }

    int indent = fprintf(stderr, "%d: ", line_num);
    fprintf(stderr, "%.*s\n", (int) (line_end - line_begin), line_begin);
    int pos = (int) (loc - line_begin) + indent;
    return pos;
}

/*
 * エラー箇所を報告する
 * https://www.sigbus.info/compilerbook#ステップ26-入力をファイルから読む
 */
void error_at(const char *loc, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);

    // エラー箇所を"^"で指し示して、エラーメッセージを表示
    int pos = print_with_line_number(loc);
    fprintf(stderr, "%*s", pos, ""); // pos個の空白を出力
    fprintf(stderr, "^ ");
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    va_end(ap);
}

bool start_with(const char *p, const char *str) {
    return strncmp(p, str, strlen(str)) == 0;
}

bool is_alpha(char c) {
    return ('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z') || c == '_';
}

bool is_alnum(char c) {
    return is_alpha(c) || ('0' <= c && c <= '9');
}

// 新しいトークンを作成してcurに繋げる
Token *new_token(TokenKind kind, Token *cur, char *str, size_t len) {
    Token *tok = calloc(1, sizeof(Token));
    tok->kind = kind;
    tok->str = str;
    tok->len = len;
    cur->next = tok;
    return tok;
}

int reserved(const char *p) {
    // Keyword
    static char *kws[] = {
            "sizeof",
            "return",
            "if",
            "else",
            "while",
            "for",
            "int",
            "char",
            "void",
    };
    for (int i = 0; i < sizeof(kws) / sizeof(*kws); i++) {
        char *keyword = kws[i];
        size_t len = strlen(keyword);
        char next = p[len];
        if (strncmp(p, keyword, len) == 0 && !is_alnum(next))
            return len;
    }

    // Multi-letter punctuator
    static char *ops[] = {"==", "!=", "<=", ">="};
    for (int i = 0; i < sizeof(ops) / sizeof(*ops); i++) {
        char *operator = ops[i];
        size_t len = strlen(operator);
        if (strncmp(p, operator, len) == 0)
            return len;
    }

    // Single-letter punctuator
    if (strchr("+-*/()<>;={},&[]\"", *p))
        return 1;

    return 0;
}

// 入力文字列pをトークナイズしてそれを返す
Token *tokenize(char *p) {
    user_input = p;
    Token head;
    head.next = NULL;
    Token *cur = &head;

    while (*p) {
        // 空白文字をスキップ
        if (isspace(*p)) {
            p++;
            continue;
        }

        if (start_with(p, "//")) {
            p += 2;
            while (!start_with(p, "\n")) {
                p++;
            }
            p++;
            continue;
        }

        char *const DOUBLE_QUOTE = "\"";
        if (start_with(p, DOUBLE_QUOTE)) {
            p++;
            char *start = p;
            while (!start_with(p, DOUBLE_QUOTE)) {
                p++;
            }
            cur = new_token(TK_STR_LITERAL, cur, start, p - start);
            p++;
            continue;
        }

        int len = reserved(p);
        if (0 < len) {
            cur = new_token(TK_RESERVED, cur, p, len);
            p += len;
            continue;
        }

        if (is_alpha(*p)) {
            char *q = p++;
            while (is_alnum(*p))
                p++;
            cur = new_token(TK_IDENT, cur, q, p - q);
            continue;
        }

        // Integer literal
        if (isdigit(*p)) {
            cur = new_token(TK_NUM, cur, p, 0);
            char *q = p;
            cur->val = (int) strtol(p, &p, 10);
            cur->len = p - q;
            continue;
        }

        error_at(p, "トークナイズできません");
        exit(1);
    }

    new_token(TK_EOF, cur, p, 0);
    return head.next;
}