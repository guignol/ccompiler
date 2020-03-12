#include "common.h"

struct Declaration_C {
    Declaration *head;
    Declaration *tail;
};
// 関数宣言
struct Declaration_C *declarations;

void init_functions() {
    declarations = calloc(1, sizeof(struct Declaration_C));
}

void ___add_function_declaration(Declaration *next) {
    if (!declarations->head) {
        declarations->head = next;
        declarations->tail = next;
        return;
    }
    declarations->tail = declarations->tail->next = next;
}

Declaration *find_function(char *name, int len) {
    for (Declaration *d = declarations->head; d; d = d->next)
        if (d->len == len && !strncmp(name, d->name, d->len))
            return d;
    return NULL;
}

void assert_function_signature(const Declaration *const old, const Declaration *const new) {
    char *const loc = new->name;
    // 定義 ⇒ 定義
    if (old->defined && new->defined) {
        error_at(loc, "関数定義が重複しています");
        exit(1);
    }
    // 返り値の型チェック
    if (!are_same_type(old->return_type, new->return_type)) {
        error_at(loc, "関数の返り値の型が一致しません");
        exit(1);
    }

    /**
     * Cの仕様としては、
     * 引数が空で定義された関数の呼び出しでは引数のチェックは行われない
     *   int foo() {}
     *   int main(void) { foo(1, 2, 3); return 0; }
     * http://0x19f.hatenablog.com/entry/2019/04/17/213231
     *
     * 仕様かどうか分からない挙動として、
     * 引数が空で宣言された関数と、引数が定義された関数は互換があると見なされる
     * ただし、charかboolを用いるとエラーになる
     * 　　void foo();
     *　という宣言があるとき、
     *　　　void foo(int a) {}
     *　　　void foo(char *a) {}
     *　という定義は書けるけど
     *　　　void foo(char a) {}
     *　　　void foo(bool a) {}
     * はエラーになる。
     * 引数が複数ある場合も、charかboolを含むとエラーになる。
     * その他に問題となる方は未確認
     */
    Variable *const old_type = old->type_only;
    Variable *const new_type = new->type_only;
    if (old_type == NULL && new_type == NULL) {
        // 引数なし（古、新）
        return;
    } else if (old_type == NULL) {
        // 引数なし（古）
        if (old->defined) {
            // 定義（なし） ⇒ 宣言（あり）
            // [new] int main(int argc);
            // [old] int main() { return 0; }
            error_at(loc, "関数の引数の型が一致しません");
            exit(1);
        } else {
            // 宣言（なし） ⇒ 定義（あり）
            // [old] int main(int argc);
            // [new] int main() { return 0; }
            // または、
            // 宣言（なし） ⇒ 宣言（あり）
            // TODO charを含むとエラー
            return;
        }
    } else if (new_type == NULL) {
        // 引数なし（新）
        if (old->defined) {
            // 定義（あり） ⇒ 宣言（なし）
            // [new] int main(int argc);
            // [old] int main() { return 0; }
            // TODO charを含むとエラー
            return;
        } else if (new->defined) {
            // 宣言（あり） ⇒ 定義（なし）
            // [old] int main(int argc);
            // [new] int main() { return 0; }
            error_at(loc, "関数の引数の型が一致しません");
            exit(1);
        } else {
            // 宣言（あり） ⇒ 宣言（なし）
            return;
        }
    } else {
        Variable *type_1 = old_type;
        Variable *type_2 = new_type;
        // 引数の型チェック
        while (type_1) {
            if (type_2) {
                if (!are_same_type(type_1->type, type_2->type)) {
                    error_at(loc, "関数の引数の数が一致しません");
                    exit(1);
                }
                type_1 = type_1->next;
                type_2 = type_2->next;
            } else {
                // 型2が足りなかったら
                error_at(loc, "関数の引数の数が一致しません");
                exit(1);
            }
        }
        // 引数の型チェック
        if (type_2) {
            // 型2が余っていたら
            error_at(loc, "関数の引数の数が一致しません");
            exit(1);
        }
    }
}

void add_function_declaration(Type *returnType,
                              Token *function_name,
                              Variable *type_only,
                              bool definition) {
    Declaration *const new_d = calloc(1, sizeof(Declaration));
    new_d->return_type = returnType;
    new_d->name = function_name->str;
    new_d->len = function_name->len;
    new_d->type_only = type_only;
    new_d->defined = definition;

    // 同名の関数宣言または定義を探す
    Declaration *const old_d = find_function(function_name->str, function_name->len);
    if (old_d) {
        assert_function_signature(old_d, new_d);
        if (old_d->type_only == NULL) {
            // 既存の宣言が引数なしだった場合は上書きする
            // （宣言が重複した場合の唯一の副作用）
            old_d->type_only = new_d->type_only;
        }
        old_d->defined |= new_d->defined;
    } else {
        ___add_function_declaration(new_d);
    }
}