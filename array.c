#include "parse.h"

NodeArray *create_node_array(int capacity) {
    if (capacity < 1) {
        capacity = 10;
    }
    NodeArray *array = malloc(sizeof(NodeArray));
    array->memory = malloc(sizeof(Node) * capacity);
    array->count = 0;
    array->capacity = capacity;
    return array;
}

void push_node(NodeArray *array, Node *node) {
    if (array->count == array->capacity) {
        array->memory = realloc(array->memory, sizeof(Node) * array->capacity * 2);
        array->capacity = array->capacity * 2;
    }
    array->memory[array->count] = node;
    array->count++;
}

void set_last_node(NodeArray *array, Node *node) {
    array->memory[array->count - 1] = node;
}

Node *peek_last_node(NodeArray *array) {
    return array->memory[array->count -1];
}

/////////////////////////

Node *create_node_block(int size) {
    Node *const block = calloc(1, sizeof(Node));
    block->kind = ND_BLOCK;
    block->statement = create_node_array(size);
    return block;
}

/////////////////////////

void assert_indexable(Node *left, Node *right) {
    // 片方がintで、もう片方が配列orポインタ
    Type *left_type = find_type(left);
    Type *right_type = find_type(right);
    switch (left_type->ty) {
        case TYPE_VOID:
            error("voidで[]は使えません？\n");
            exit(1);
        case TYPE_CHAR:
        case TYPE_INT: {
            // 左がint
            switch (right_type->ty) {
                case TYPE_VOID:
                    error("voidで[]は使えません？\n");
                    exit(1);
                case TYPE_CHAR:
                case TYPE_INT:
                    error("整数間で[]は使えません？\n");
                    exit(1);
                case TYPE_POINTER:
                case TYPE_ARRAY:
                    break;
                case TYPE_STRUCT:
                    // TODO
                    error("[parse]構造体実装中\n");
                    exit(1);
                case TYPE_ENUM:
                    // TODO
                    error("[parse]enum実装中\n");
                    exit(1);
            }
            break;
        }
        case TYPE_POINTER:
        case TYPE_ARRAY: {
            // 左が配列orポインタ
            switch (right_type->ty) {
                case TYPE_VOID:
                    error("voidで[]は使えません？\n");
                    exit(1);
                case TYPE_CHAR:
                case TYPE_INT:
                    break;
                case TYPE_POINTER:
                case TYPE_ARRAY:
                    error("not_int[not_int]はできません？");
                    exit(1);
                case TYPE_STRUCT:
                    // TODO
                    error("[parse]構造体実装中\n");
                    exit(1);
                case TYPE_ENUM:
                    // TODO
                    error("[parse]enum実装中\n");
                    exit(1);
            }
            break;
        }
        case TYPE_STRUCT:
            // TODO
            error("[parse]構造体実装中\n");
            exit(1);
        case TYPE_ENUM:
            // TODO
            error("[parse]enum実装中\n");
            exit(1);
    }
}

Node *new_node_array_index(Node *const left, Node *const right, const enum NodeKind kind) {
    // int a[2];
    // a[1] = 3;
    // は
    // *(a + 1) = 3;
    // の省略表現
    assert_indexable(left, right);
    Node *pointer = pointer_calc(ND_ADD, left, right);
    return new_node(kind, pointer, NULL);
}

Node *with_index(Node *left) {
    bool consumed = false;
    while (consume("[")) {
        consumed = true;
        left = new_node_array_index(left, expr(), ND_DEREF_ARRAY_POINTER);
        expect("]");
    }
    Type *type = find_type(left);
    if (consumed && type->ty != TYPE_ARRAY) {
        // 配列の指すところまでアクセスしたらポインタの値をデリファレンスする
        left->kind = ND_DEREF;
    }
    return left;
}

void visit_array_initializer(NodeArray *array, Type *type) {
    // 配列サイズが明示されていない場合
    const bool implicit_size = type->array_size == 0;
    /*
     * char charara[2][4] = {"abc", "def"};
     *
     * 0: pointer  (1 * 8 byte)
     * 1: pointer
     *
     * ↑ではなく↓
     *
     * 0: char char char (3 * 1 byte)
     * 1: char char char
     *
     * ↓ x86-64 gcc 9.1以降（これに近い方式を採用する）
     *
     *   sub rsp, 16
     *   mov WORD PTR [rbp-6], 25185
     *   mov BYTE PTR [rbp-4], 99
     *   mov WORD PTR [rbp-3], 25956
     *   mov BYTE PTR [rbp-1], 102
     *
     * ↓ x86-64 gcc 4.7.4以前
     *
     *   sub rsp, 16
     *   mov WORD PTR [rbp-16], 25185
     *   mov BYTE PTR [rbp-14], 99
     *   mov WORD PTR [rbp-13], 25956
     *   mov BYTE PTR [rbp-11], 102
     *
     * ↓ その他のgcc
     *
     * .LC0:
     *   .ascii "abc"
     *   .ascii "def"
     * ---------------
     *   sub rsp, 16
     *   mov eax, DWORD PTR .LC0[rip]
     *   mov DWORD PTR [rbp-6], eax
     *   movzx eax, WORD PTR .LC0[rip+4]
     *   mov WORD PTR [rbp-2], ax
     *
     */
    if (consume("{")) {
        if (type->ty != TYPE_ARRAY) {
            error_at(token->str, "配列の初期化式ではありません？");
            exit(1);
        }
        int index = 0;
        do {
            if (!implicit_size && type->array_size <= index) {
                warn_at(token->str, "excess elements in array initializer");
                token = token->next;
            } else {
                visit_array_initializer(array, type->point_to);
            }
            consume(","); // 末尾に残ってもOK
            index++;
        } while (!consume("}"));
        if (implicit_size) {
            // サイズを明示していない配列のサイズを決定する
            type->array_size = index;
        }
        // 初期化式の要素数が配列のサイズより小さい場合、0で埋める。
        while (index < type->array_size) {
            push_node(array, new_node_num(0));
            index++;
        }
    } else if (token->kind == TK_STR_LITERAL && type->ty == TYPE_ARRAY) {
        int index = 0;
        for (; index < token->len; index++) {
            if (!implicit_size && type->array_size <= index) {
                warn_at(token->str, "initializer-string for array of chars is too long");
                break;
            }
            char c = token->str[index];
            Node *const node = new_node_num(c);
            push_node(array, node);
        }
        if (implicit_size) {
            index++;
            // 末尾を\0
            Node *const zero = new_node_num(0);
            push_node(array, zero);
            // サイズを明示していない配列のサイズを決定する
            type->array_size = index;
        }
        // 初期化式の要素数が配列のサイズより小さい場合、0で埋める。
        while (index < type->array_size) {
            push_node(array, new_node_num(0));
            index++;
        }
        token = token->next;
    } else {
        Node *const node = expr();
        push_node(array, node);
    }
}

/*
 * 配列の初期化時のみ可能な式がいくつか
 * int array[4] = {0, 1, 2, 3};
 * int array[4] = {0, 1, 2};
 * int array[] = {0, 1, 2, 3};
 * char msg[4] = {'f', 'o', 'o', '_'}; => 警告なし
 * char msg[4] = {'f', 'o', 'o', '\0'};
 * char msg[] = {'f', 'o', 'o', '\0'};
 * char *s1[2] = {"abc", "def"};
 * char *s1[] = {"abc", "def"};
 * char msg[] = "foo";
 * char msg[10] = "foo";
 * char msg[3] = "message"; => initializer-string for array of chars is too long
 * char s1[2][3] = {"abc", "def"};
 * char s1[][3] = {"abc", "def"};
 * int array[3][2] = {{3, 3}, {3, 3}, {3, 3}};
 * int array[][2] = {{3, 3}, {3, 3}, {3, 3}};
 *
 * C99にはさらに色々ある
 * https://kaworu.jpn.org/c/%E9%85%8D%E5%88%97%E3%81%AE%E5%88%9D%E6%9C%9F%E5%8C%96_%E6%8C%87%E7%A4%BA%E4%BB%98%E3%81%8D%E3%81%AE%E5%88%9D%E6%9C%9F%E5%8C%96%E6%8C%87%E5%AE%9A%E5%AD%90
 */
Node *array_initializer(Node *const array_variable, Type *const type) {
    char *loc = token->str;
    int capacity = get_element_count(type);
    NodeArray *nodeArray = create_node_array(capacity);
    visit_array_initializer(nodeArray, type);

    // ポインター型への変換
    Type *pointed = type->point_to;
    while (pointed->ty == TYPE_ARRAY) {
        pointed = pointed->point_to;
    }
    array_variable->type = create_pointer_type(pointed);
    Node *pointer = array_variable;

    /*
      int x[] = {1, 2, foo()};
      ↓ ↓　↓　↓
      ブロックでまとめる
      ↓ ↓　↓　↓
      int x[3];
      {
        x[0] = 1;
        x[1] = 2;
        x[2] = foo();
      }
     */
    Node *const block = create_node_block(5);
    for (int i = 0; i < nodeArray->count; ++i) {
        Node *n = nodeArray->memory[i];
        Node *const indexed = new_node_array_index(pointer, new_node_num(i), ND_DEREF);
        Node *const assign = new_node_assign(loc, indexed, n);
        push_node(block->statement, assign);
    }
    return block;
}