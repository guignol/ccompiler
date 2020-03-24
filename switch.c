#include "parse.h"

Node *consume_case_statement() {
    Node *const block = calloc(1, sizeof(Node));
    // スコープを作らないブロックとしてまとめる
    block->kind = ND_BLOCK;
    block->statement = create_node_array(3);
    // caseやdefaultが続く場合がある
    bool no_statement = check("case") || check("default");
    while (!no_statement) {
        // 末尾チェック
        if (check("}")) {
            break;
        }
        push_node(block->statement, stmt());
        no_statement = check("case") || check("default");
    }
    return block;
}

struct Case *consume_default_() {
    if (consume("default")) {
        struct Case *const case_ = malloc(sizeof(struct Case));
        case_->default_ = true;
        expect(":");
        case_->statement = consume_case_statement();
        return case_;
    }
    return NULL;
}

struct Case *consume_case_() {
    if (consume("case")) {
        Token *const var = consume_ident();
        struct Case *const case_ = malloc(sizeof(struct Case));
        case_->default_ = false;
        if (var) {
            Global *const member = find_enum_member(var->str, var->len);
            // TODO 定数なら使えるけど、他に定数はまだ無いはず？
            // TODO 1 + 2 とかも定数扱いになるっぽい？
            if (member == NULL) {
                error_at(var->str, "定数ではありません");
                exit(1);
            }
            case_->value = member->target->value;
        } else {
            case_->value = expect_num_char();
        }
        expect(":");
        case_->statement = consume_case_statement();
        return case_;
    }
    return NULL;
}

struct Case *consume_case() {
    struct Case head;
    head.next = NULL;
    struct Case *tail = &head;
    while (true) {
        struct Case *const c = consume_case_();
        if (c) {
            tail = tail->next = c;
            continue;
        }
        struct Case *const d = consume_default_();
        if (d) {
            tail = tail->next = d;
            continue;
        }
        break;
    }
    return head.next;
}