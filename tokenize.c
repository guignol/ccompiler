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

// TODO
bool warning = false;

void warn_at(const char *loc, const char *fmt, ...) {
    if (!warning) {
        return;
    }

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
            "struct",
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

char escaped(char *p) {
    char c = *p;
    switch (c) {
        case 'a':
            return '\a';
        case 'b':
            return '\b';
        case 't':
            return '\t';
        case 'n':
            return '\n';
        case 'v':
            return '\v';
        case 'f':
            return '\f';
        case 'r':
            return '\r';
        case 'e':
            return 27;
        case '0':
            return '\0';
        default:
            warn_at(p, "Unknown escape sequence: \\%c", c);
            return c;
    }
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
        if (start_with(p, "/*")) {
            p += 2;
            while (!start_with(p, "*/")) {
                p++;
            }
            p += 2;
            continue;
        }

        static char *const SINGLE_QUOTE = "'";
        if (start_with(p, SINGLE_QUOTE)) {
            char *const start = ++p;
            while (!start_with(p + 1, SINGLE_QUOTE)) {
                p++;
            }
            const int length = (int) (p - start) + 1;
            cur = new_token(TK_CHAR_LITERAL, cur, start, length);
            if (1 < length && *(p - 1) == '\\') {
                cur->val = escaped(p);
                p++; // 文字の分
                p++; // クオートの分
                continue;
            }
            if (is_alnum(*p)) {
                if (1 < length) {
                    // TODO warning
                }
                cur->val = *p;
                p++; // 文字の分
                p++; // クオートの分
                continue;
            }
            error_at(p, "charとして使えない文字です？");
            exit(1);
        }

        static char *const DOUBLE_QUOTE = "\"";
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