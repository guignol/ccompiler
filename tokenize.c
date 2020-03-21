#include "common.h"

// 入力プログラム
char *user_input;

int print_with_line_number(const char *loc) {
    // locが含まれている行の開始地点と終了地点を取得
    const char *line_begin = loc;
    while (user_input < line_begin && line_begin[-1] != '\n') {
        line_begin = line_begin - 1;
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
    fprintf(stderr, "%.*s\n", /** (int) */ (line_end - line_begin), line_begin);
    int pos = /** (int) */ (loc - line_begin) + indent;
    return pos;
}

/*
 * エラー箇所を報告する
 * https://www.sigbus.info/compilerbook#ステップ26-入力をファイルから読む
 */
void error_at(const char *loc, const char *message) {
    // エラー箇所を"^"で指し示して、エラーメッセージを表示
    int pos = print_with_line_number(loc);
    fprintf(stderr, "%*s", pos, ""); // pos個の空白を出力
    fprintf(stderr, "^ ");
    fprintf(stderr, "%s", message);
    fprintf(stderr, "\n");
}

void error_at_1(const char *loc, const char *fmt, const char *message) {
    // エラー箇所を"^"で指し示して、エラーメッセージを表示
    int pos = print_with_line_number(loc);
    fprintf(stderr, "%*s", pos, ""); // pos個の空白を出力
    fprintf(stderr, "^ ");
    fprintf(stderr, fmt, message);
    fprintf(stderr, "\n");
}

void error_at_2_2(const char *loc, const char *fmt,
                  int len_1, const char *str_1,
                  int len_2, const char *str_2) {
    // エラー箇所を"^"で指し示して、エラーメッセージを表示
    int pos = print_with_line_number(loc);
    fprintf(stderr, "%*s", pos, ""); // pos個の空白を出力
    fprintf(stderr, "^ ");
    fprintf(stderr, fmt, len_1, str_1, len_2, str_2);
    fprintf(stderr, "\n");
}

// TODO
bool warning = false;

void warn_at(const char *loc, const char *message) {
    if (!warning) {
        return;
    }

    // エラー箇所を"^"で指し示して、エラーメッセージを表示
    int pos = print_with_line_number(loc);
    fprintf(stderr, "%*s", pos, ""); // pos個の空白を出力
    fprintf(stderr, "^ ");
    fprintf(stderr, "%s", message);
    fprintf(stderr, "\n");
}

void warn_at_with_c(const char *loc, const char *fmt, char c) {
    if (!warning) {
        return;
    }

    // エラー箇所を"^"で指し示して、エラーメッセージを表示
    int pos = print_with_line_number(loc);
    fprintf(stderr, "%*s", pos, ""); // pos個の空白を出力
    fprintf(stderr, "^ ");
    fprintf(stderr, fmt, c);
    fprintf(stderr, "\n");
}

bool start_with(const char *p, const char *str) {
    return strncmp(p, str, strlen(str)) == 0;
}

//#include <ctype.h>
// isspace
// 指定された文字がホワイトスペース、すなわち
// 空白 (0x20)、
// 改頁 (0x0c)、
// 改行 (0x0a)、
// 復帰 (0x0d)、
// 水平タブ (0x09) または
// 垂直タブ (0x0b) かどうか調べます。
// https://ja.cppreference.com/w/c/string/byte/isspace
bool is_space(char c) {
    // cf. man ascii
    return c == 32 || // 32    20    SPACE
           c == 12 || // 12    0C    FF  '\f' (form feed)
           c == 10 || // 10    0A    LF  '\n' (new line)
           c == 13 || // 13    0D    CR  '\r' (carriage ret)
           c == 9 || // 9     09    HT  '\t' (horizontal tab)
           c == 11;   // 11    0B    VT  '\v' (vertical tab)
}

bool is_alpha(char c) {
    return ('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z') || c == '_';
}

bool is_number(char c) {
    return ('0' <= c && c <= '9');
}

bool is_alnum(char c) {
    return is_alpha(c) || is_number(c);
}

// 新しいトークンを作成してcurに繋げる
Token *new_token(enum TokenKind kind, Token *cur, char *str, size_t len) {
    Token *tok = calloc(1, sizeof(Token));
    tok->kind = kind;
    tok->str = str;
    tok->len = len;
    cur->next = tok;
    return tok;
}

// Keyword
char *kws[] = {
        "sizeof",
        "return",
        "if",
        "else",
        "while",
        "for",
        "int",
        "char",
        "void",
        "_Bool",
        "struct",
        "enum",
        "switch",
        "case",
        "break",
        "default",
        "extern",
        "typedef",
};

// two-letter punctuator
char *ops[] = {"==", "!=", "<=", ">=", "||", "&&", "->", "++"};

int reserved(const char *p) {
    for (int i = 0; i < sizeof(kws) / sizeof(*kws); i++) {
        char *keyword = kws[i];
        size_t len = strlen(keyword);
        char next = p[len];
        if (strncmp(p, keyword, len) == 0 && !is_alnum(next))
            return len;
    }

    for (int i = 0; i < sizeof(ops) / sizeof(*ops); i++) {
        char *operator = ops[i];
        if (strncmp(p, operator, 2) == 0)
            return 2;
    }

    // Single-letter punctuator
    if (strchr("+-*/()<>;={},&[].!:\"%?", *p))
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
            warn_at_with_c(p, "Unknown escape sequence: \\%c", c);
            return c;
    }
}

char *const SINGLE_QUOTE = "'";
char *const DOUBLE_QUOTE = "\"";

// 入力文字列pをトークナイズしてそれを返す
Token *tokenize(char *p) {
    user_input = p;
    Token head;
    head.next = NULL;
    Token *cur = &head;

    while (*p) {
        // 空白文字をスキップ
        if (is_space(*p)) {
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
        // プリプロセスの名残を無視する
        if (start_with(p, "#")) {
            p++;
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
        if (start_with(p, SINGLE_QUOTE)) {
            char *const start = ++p;
            while (!start_with(p + 1, SINGLE_QUOTE)) {
                p++;
            }
            const int length = /** (int) */ (p - start) + 1;
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

        if (start_with(p, DOUBLE_QUOTE)) {
            p++;
            char *start = p;
            while (!start_with(p, DOUBLE_QUOTE)) {
                if (*p == '\\') {
                    // TODO エスケープできない文字
                    p++;
                    p++;
                } else {
                    p++;
                }

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

        // TODO
        if (start_with(p, "const")) {
            char *s = p + 5;
            // 空白文字かどうか確認
            if (is_space(*s)) {
                s++;
                p = s;
                continue;
            }
        }

        if (is_alpha(*p)) {
            char *q = p++;
            while (is_alnum(*p))
                p++;
            cur = new_token(TK_IDENT, cur, q, p - q);
            continue;
        }

        // Integer literal
        if (is_number(*p)) {
            cur = new_token(TK_NUM, cur, p, 0);
            char *q = p;
            cur->val = /** (int) */ strtol(p, &p, 10);
            cur->len = p - q;
            continue;
        }

        error_at(p, "トークナイズできません");
        exit(1);
    }

    new_token(TK_EOF, cur, p, 0);
    return head.next;
}