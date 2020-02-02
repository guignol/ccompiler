#include "9cc.h"

// 入力プログラム
char *user_input;

int error_with_front_space(const char *message) {
    int space = 0;
    for (int i = 0; i < strlen(message); ++i) {
        if (isspace(message[i])) {
            space++;
        } else {
            break;
        }
    }
    fprintf(stderr, "%*s", space * 4, ""); // pos個の空白を出力
    fprintf(stderr, "%s\n", message + space);
    return space;
}

// エラー箇所を報告する
void error_at(const char *loc, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);

    char *head = user_input;
    char *tail = strtok(user_input, "\n");
    while (tail) {
        if (loc < tail) {
            int pos = (int) (loc - head);
            int front_space = error_with_front_space(head);
            fprintf(stderr, "%*s", front_space * 4, "");
            fprintf(stderr, "%*s", pos - front_space, "");
            fprintf(stderr, "^ ");
            vfprintf(stderr, fmt, ap);
            fprintf(stderr, "\n");
            error_with_front_space(tail);
            return;
        }
        head = tail;
        tail = strtok(NULL, "\n");
    }

    fprintf(stderr, "%s\n", head);
    fprintf(stderr, "%*s", (int) (loc - user_input), "");
    fprintf(stderr, "^ ");
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
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
            "int"
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
    if (strchr("+-*/()<>;={},&[]", *p))
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