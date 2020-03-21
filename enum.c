#include "parse.h"

EnumMembers *create_enum_member(int capacity) {
    if (capacity < 1) {
        capacity = 10;
    }
    EnumMembers *members = malloc(sizeof(EnumMembers));
    members->memory = malloc(sizeof(Global) * capacity);
    members->count = 0;
    members->capacity = capacity;
    return members;
}

EnumMembers *push_enum_member(EnumMembers *members, Global *new) {
    if (members->count == members->capacity) {
        members->memory = realloc(members->memory, sizeof(Global) * members->capacity * 2);
        members->capacity *= 2;
    }
    members->memory[members->count] = new;
    members->count++;
    return members;
}

bool consume_enum_def(Type *base) {
    // enumの定義
    if (base->ty == TYPE_ENUM && consume("{")) {
        int count = 0;
        do {
            Token *const member = consume_ident();
            if (member == NULL) {
                if (count == 0) {
                    error_at(token->str, "enumのメンバーがありません");
                    exit(1);
                }
                // 末尾に , はアリ
                break;
            }
            if (consume("=")) {
                count = expect_num_char();
            }
            // https://docs.microsoft.com/ja-jp/cpp/c-language/c-enumeration-declarations?view=vs-2019
            //　TODO enumのメンバーはグローバル変数相当の定数で、
            //  同じ名前はグローバルスコープで宣言できず、ローカルでは可能
            Global *g = calloc(1, sizeof(Global));
            g->type = shared_int_type();
            g->label = member->str;
            g->label_length = member->len;
            g->target = calloc(1, sizeof(Directives));
            g->target->directive = _enum;
            g->target->value = count++;
            // TODO 重複チェック
            add_globals(g);
            // enumのメンバーとして登録
            push_enum_member(base->enum_info->members, g);

        } while (consume(","));
        expect("}");
        expect(";");
        return true;
    }
    return false;
}