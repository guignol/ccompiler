#include "parse.h"

struct Labels {
    Token **memory;
    int count;
    int capacity;
};

struct Labels *gotos;
struct Labels *labels;

void init_goto_label() {
    if (gotos) {
        free(gotos->memory);
        // 使わない関数のほうが多そうなので必要になってから作る
        gotos = NULL;
    }
    if (labels) {
        free(labels->memory);
        // 使わない関数のほうが多そうなので必要になってから作る
        labels = NULL;
    }
}

void push_goto_label(struct Labels **p, Token *target) {
    struct Labels *lb = *p;
    if (lb == NULL) {
        // 使わない関数のほうが多そうなので必要になってから作る
        const int capacity = 5;
        lb = *p = malloc(sizeof(struct Labels));
        lb->memory = malloc(sizeof(Token *) * capacity);
        lb->count = 0;
        lb->capacity = capacity;
    } else if (lb->count == lb->capacity) {
        lb->memory = realloc(lb->memory, sizeof(Token) * lb->capacity * 2);
        lb->capacity = lb->capacity * 2;
    }
    lb->memory[lb->count] = target;
    lb->count++;
}

bool find_goto_label(struct Labels **p, Token *target) {
    struct Labels *lb = *p;
    if (lb == NULL) {
        return false;
    }
    char *const str = target->str;
    const int len = target->len;
    for (int i = 0; i < lb->count; ++i) {
        Token *s = lb->memory[i];
        if (s->len == len && !memcmp(str, s->str, len)) {
            return true;
        }
    }
    return false;
}

void assert_goto_label() {
    // 関数内の後ろで定義されるのが普通なので関数を読み終わってから確認する
    if (gotos == NULL) {
        return;
    }
    for (int i = 0; i < gotos->count; ++i) {
        Token *gt = gotos->memory[i];
        if (find_goto_label(&labels, gt)) {
            continue;
        }
        error_1_1("ラベル %.*s: が定義されていません。\n", gt->len, gt->str);
        exit(1);
    }
}

Node *new_node_goto(Token *function_name, Token *label) {
    push_goto_label(&gotos, label);

    Node *const node = calloc(1, sizeof(Node));
    node->kind = ND_GOTO;
    // ラベル名
    node->label = label->str;
    node->label_length = label->len;
    // 所属する関数名
    node->name = function_name->str;
    node->len = function_name->len;
    return node;
}

Node *new_node_label(Token *function_name, Token *label) {
    if (find_goto_label(&labels, label)) {
        error_at(label->str, "このラベルは定義済みです");
        exit(1);
    }
    push_goto_label(&labels, label);

    Node *const node = calloc(1, sizeof(Node));
    node->kind = ND_LABEL;
    // ラベル名
    node->label = label->str;
    node->label_length = label->len;
    // 所属する関数名
    node->name = function_name->str;
    node->len = function_name->len;
    return node;
}