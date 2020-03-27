#include "parse.h"

struct TypeDef {
    Type *type;
    char *alias;
    int alias_length;

    struct TypeDef *next;
};

struct TypeDef *type_def_head = NULL;
struct TypeDef *type_def_tail = NULL;

struct TypeDef *create_type_def(Type *type, char *alias, int alias_len) {
    struct TypeDef *type_def = malloc(sizeof(struct TypeDef));
    type_def->type = type;
    type_def->alias = alias;
    type_def->alias_length = alias_len;
    type_def->next = NULL;
    return type_def;
}

void register_type_def(Type *type, Token *alias_t) {
    char *const alias = alias_t->str;
    const int alias_length = alias_t->len;
    if (type_def_head == NULL) {
        type_def_head = type_def_tail = create_type_def(type, alias, alias_length);
    } else {
        // 重複チェック
        for (struct TypeDef *d = type_def_head; d; d = d->next) {
            if (d->alias_length == alias_length && !strncmp(alias, d->alias, alias_length)) {
                if (!are_same_type(d->type, type)) {
                    // 同じ名前で別の型の場合
                    error_at(d->alias, "");
                    error_at(alias, "異なる型に同じ名前がついています。");
                    exit(1);
                }
            }
        }
        struct TypeDef *const type_Def = create_type_def(type, alias, alias_length);
        type_def_tail = type_def_tail->next = type_Def;
    }
}

Type *find_alias(Token *alias_t) {
    char *const alias = alias_t->str;
    const int alias_length = alias_t->len;
    for (struct TypeDef *d = type_def_head; d; d = d->next) {
        // TODO dにアクセスしたらstage3コンパイラをコンパイルするときに Segmentation fault
        if (d->alias_length == alias_length && !strncmp(alias, d->alias, alias_length)) {
            // 定義済みの構造体型情報があれば読み込む
            load_struct(d->type);
            return d->type;
        }
    }
    return NULL;
}
