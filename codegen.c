#include "9cc.h"

// RAX	返り値 / 引数の数	✔
// RDI	第1引数	✔
// RSI	第2引数	✔
// RDX	第3引数	✔
// RCX	第4引数	✔
// RBP	ベースポインタ
// RSP	スタックポインタ
// RBX	（特になし）
// R8	第5引数	✔
// R9	第6引数	✔
static char *registers[] = {"rdi",
                            "rsi",
                            "rdx",
                            "rcx",
                            "r8",
                            "r9"};

void ___COMMENT___(char *format, ...) {
    va_list ap;
    va_start(ap, format);

    printf("  # ");
    vfprintf(stdout, format, ap);
    printf("\n");
}

void gen(Node *node);

void gen_address(Node *node) {
    switch (node->kind) {
        case ND_VARIABLE:
            printf("  mov rax, rbp\n");
            printf("  sub rax, %d\n", node->offset);
            printf("  push rax # variable [%.*s]\n", node->len, node->name);
            break;
        case ND_DEREF:
            /*
                int x = 3;
                int *y = &x;
                *y = 4; // この代入とか
                return x; // => 4
            */
            gen(node->lhs);
            break;
        default:
            error("代入の左辺値が変数ではありません");
            break;
    }
}

int labelseq = 0;
char *function_name;

void gen(Node *node) {
    switch (node->kind) {
        case ND_NOTHING:
            return;
        case ND_FUNC: {
            ___COMMENT___("begin function call [%.*s]", node->len, node->name);
            int count = 0;
            if (node->args) {
                for (Node *arg = node->args; arg; arg = arg->args) {
                    gen(arg);
                    count++;
                }
                for (size_t i = 0; i < count; i++) {

                    printf("  pop %s\n", registers[count - i - 1]);
                }
            } else {
                // デバッグ用: 引数なしのprintf()に固定の文字列を渡す
                // スタックポインタのアラインなしだとクラッシュする場合があるのでその確認
                if (memcmp(node->name, "printf", strlen("printf")) == 0) {
                    count = 2;
                    printf("  mov rdi, QWORD PTR debug_moji[rip]\n");
                    printf("  mov rsi, 2\n");
                }
            }
            static bool doAlign = true;
            if (doAlign) {
                // https://github.com/rui314/chibicc/commit/aedbf56c3af4914e3f183223ff879734683bec73
                // We need to align RSP to a 16 byte boundary before
                // calling a function because it is an ABI requirement.
                // RAX is set to 0 for variadic function.
                int seq = labelseq++;
                printf("  mov rax, rsp\n");
                printf("  and rax, 15\n");
                printf("  jnz .Lcall%d\n", seq);
                printf("  mov rax, 0\n");
                printf("  call %.*s\n", node->len, node->name);
                printf("  jmp .Lend%d\n", seq);
                printf(".Lcall%d:\n", seq);
                printf("  sub rsp, 8\n");
                printf("  mov rax, 0\n");
                printf("  call %.*s\n", node->len, node->name);
                printf("  add rsp, 8\n");
                printf(".Lend%d:\n", seq);
                printf("  push rax\n");
            } else {
                printf("  mov rax, %i\n", count);
                printf("  call %.*s\n", node->len, node->name);
                printf("  push rax\n");
            }
            ___COMMENT___("end function call [%.*s]", node->len, node->name);

            return;
        }
        case ND_BLOCK: {
            ___COMMENT___("block begin");
            gen(node->statement);
            for (Node *next = node->statement->statement;
                 next;
                 next = next->statement) {
                gen(next);
            }
            ___COMMENT___("block end");
            return;
        }
        case ND_FOR: {
            ___COMMENT___("for begin");
            int seq = labelseq++;
            // init
            if (node->lhs)
                gen(node->lhs);
            // begin
            printf(".Lbegin%d:\n", seq);
            // condition
            if (node->condition) {
                gen(node->condition);
                printf("  pop rax\n");
                printf("  cmp rax, 0\n");
                // if 0, goto end
                printf("  je  .Lend%d\n", seq);
            }
            // execute
            gen(node->execution);
            // post execute
            if (node->rhs) {
                gen(node->rhs);
            }
            // goto begin
            printf("  jmp .Lbegin%d\n", seq);
            // end:
            printf(".Lend%d:\n", seq);
            ___COMMENT___("for end");
            return;
        }
        case ND_WHILE: {
            ___COMMENT___("while begin");
            int seq = labelseq++;
            // begin:
            printf(".Lbegin%d:\n", seq);
            // condition
            gen(node->condition);
            printf("  pop rax\n");
            printf("  cmp rax, 0\n");
            // if 0, goto end
            printf("  je  .Lend%d\n", seq);
            // execute & goto begin
            gen(node->lhs);
            printf("  jmp .Lbegin%d\n", seq);
            // end:
            printf(".Lend%d:\n", seq);
            ___COMMENT___("while end");
            return;
        }
        case ND_IF: {
            int seq = labelseq++;
            if (node->rhs) {
                gen(node->condition);
                printf("  pop rax\n");
                printf("  cmp rax, 0\n");
                printf("  je  .Lelse%d\n", seq);
                gen(node->lhs);
                printf("  jmp .Lend%d\n", seq);
                printf(".Lelse%d:\n", seq);
                gen(node->rhs);
                printf(".Lend%d:\n", seq);
            } else {
                gen(node->condition);
                printf("  pop rax\n");
                printf("  cmp rax, 0\n");
                printf("  je  .Lend%d\n", seq);
                gen(node->lhs);
                printf(".Lend%d:\n", seq);
            }
            return;
        }
        case ND_EXPR_STMT:
            gen(node->lhs);
            // 式文では値をスタックに残さない
            printf("  add rsp, 8\n");
            return;
        case ND_RETURN:
            gen(node->lhs);
            printf("  pop rax\n");
            printf("  jmp .Lreturn.%s\n", function_name);
            return;
        case ND_NUM:
            printf("  push %d\n", node->val);
            return;
        case ND_VARIABLE:
            // 変数のアドレスから、そのアドレスにある値を取り出す
            gen_address(node);
            printf("  pop rax\n");
            printf("  mov rax, [rax]\n");
            printf("  push rax\n");
            return;
        case ND_ASSIGN:
            ___COMMENT___("assign begin");
            gen_address(node->lhs);
            gen(node->rhs);

            printf("  pop rdi\n");
            printf("  pop rax\n");
            printf("  mov [rax], rdi\n");
            printf("  push rdi\n");
            ___COMMENT___("assign end");
            return;
        case ND_ADDRESS:
            // 変数のアドレスをスタックに積むだけ
            gen_address(node->lhs);
            return;
        case ND_DEREF:
            /*
                int var = 3;
                int *p = &var;
                &p; // => 3
              */
            gen(node->lhs); // ポインタ変数の値（アドレス）をスタックに積む
            printf("  pop rax\n");
            printf("  mov rax, [rax]\n"); // そのアドレスの値を取り出す
            printf("  push rax\n");
            return;
    }

    // 2項演算
    gen(node->lhs);
    gen(node->rhs);
    printf("  pop rdi\n");
    printf("  pop rax\n");

    switch (node->kind) {
        case ND_ADD:
            // TODO
            printf("  add rax, rdi\n");
            break;
        case ND_SUB:
            // TODO
            printf("  sub rax, rdi\n");
            break;
        case ND_MUL:
            printf("  imul rax, rdi\n");
            break;
        case ND_DIV:
            printf("  cqo\n");
            printf("  idiv rdi\n");
            break;
        case ND_EQL:
            printf("  cmp rax, rdi");
            ___COMMENT___("==");
            printf("  sete al\n");
            printf("  movzb rax, al\n");
            break;
        case ND_NOT:
            printf("  cmp rax, rdi");
            ___COMMENT___("!=");
            printf("  setne al\n");
            printf("  movzb rax, al\n");
            break;
        case ND_LESS:
            printf("  cmp rax, rdi");
            ___COMMENT___("<");
            printf("  setl al\n");
            printf("  movzb rax, al\n");
            break;
        case ND_LESS_EQL:
            printf("  cmp rax, rdi");
            ___COMMENT___("<=");
            printf("  setle al\n");
            printf("  movzb rax, al\n");
            break;
    }

    printf("  push rax\n");
}

void generate(Function *func) {
    function_name = func->name;
    printf(".global %s\n", function_name);
    printf("%s:\n", function_name);

    // プロローグ
    printf("  push rbp\n");
    printf("  mov rbp, rsp\n");
    Variable *locals = func->locals;
    if (locals) {
        printf("  sub rsp, %i  # %i %s\n", locals->offset, locals->offset / 8, "variables prepared");

        for (Variable *param = locals; param; param = param->next) {
            if (0 <= param->index) {
                printf("  mov rax, rbp\n");
                printf("  sub rax, %d\n", param->offset);
                printf("  mov [rax], %s  # parameter [%.*s]\n", registers[param->index], param->len, param->name);
            }
        }
    }

    for (size_t i = 0; i < 100; i++) {
        Node *node = func->body[i];
        if (node == NULL)
            break;

        // 抽象構文木を下りながらコード生成
        gen(node);
    }

    // エピローグ
    // 最後の式の結果がRAXに残っているのでそれが返り値になる
    printf(".Lreturn.%s:\n", function_name);
    printf("  mov rsp, rbp\n");
    printf("  pop rbp\n");
    printf("  ret\n");

    if (func->next)
        generate(func->next);
}