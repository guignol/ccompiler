#include "common.h"

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
static const char *const registers_64[] = {"rdi",
                                           "rsi",
                                           "rdx",
                                           "rcx",
                                           "r8",
                                           "r9"};
static const char *const registers_32[] = {"edi",
                                           "esi",
                                           "edx",
                                           "ecx",
                                           "r8d",
                                           "r9d"};
static const char *const registers_8[] = {"DIL",
                                          "SIL",
                                          "DL",
                                          "CL",
                                          "R8B",
                                          "R9B"};

void ___COMMENT___(char *message) {
    printf("  # %s\n", message);
}

void ___COMMENT___2(const char *fmt, const int arg_1, const char *arg_2) {
    printf("  # ");
    printf(fmt, arg_1, arg_2);
    printf("\n");
}

const char *size_prefix(int size) {
    switch (size) {
        case 1:
            // 1byte == 8bit
            return "BYTE PTR";
        case 4:
            // 4byte == 32bit
            return "DWORD PTR";
        case 8:
            // 8byte == 64bit
            return "QWORD PTR";
        default:
            error("[size_prefix]64bit以上のメモリは扱えません\n");
            exit(1);
    }
}

const char *register_name_rax(int size) {
    switch (size) {
        case 1:
            // 1byte == 8bit
            return "al";
        case 4:
            // 4byte == 32bit
            return "eax";
        case 8:
            // 8byte == 64bit
            return "rax";
        default:
            error("[register_name_rax]64bit以上のメモリは扱えません\n");
            exit(1);
    }
}

const char *register_name_for_args(int size, int index) {
    switch (size) {
        case 1:
            // 1byte == 8bit
            return registers_8[index];
        case 4:
            // 4byte == 32bit
            return registers_32[index];
        case 8:
            // 8byte == 64bit
            return registers_64[index];
        default:
            error("[register_name_for_args]64bit以上のメモリは扱えません\n");
            exit(1);
    }
}

void load(Node *node) {
    // アドレスを取り出す
    printf("  pop rax\n");
    // アドレスの値を取り出す
    Type *const type = find_type(node);
    const int type_size = get_size(type);
    const char *const prefix = size_prefix(type_size);
    switch (type_size) {
        case 1:
            // 1byte == 8bit
            printf("  movsx eax, %s [rax]\n", prefix);
            break;
        case 4:
            // 4byte == 32bit
            printf("  mov eax, %s [rax]\n", prefix);
            break;
        default:
            // 8byte == 64bit
            printf("  mov rax, [rax]\n");
            break;
    }
    printf("  push rax\n");
}

void gen(Node *node);

void gen_address(Node *node) {
    switch (node->kind) {
        case ND_VARIABLE:
        case ND_VARIABLE_ARRAY:
            printf("  # variable [%.*s]\n", node->len, node->name);
            // TODO 構造体のサイズは、AST側でどうにかしたほうがよさそう
            if (node->is_local) {
                printf("  lea rax, [rbp - %d]\n", node->offset);
            } else {
                if (node->offset) {
                    // 構造体のメンバーアクセス
                    printf("  # member [%.*s]\n", node->len, node->name);
                    printf("  lea rax, %.*s[rip+%d]\n", node->len, node->name, node->offset);
                } else {
                    printf("  lea rax, %.*s[rip]\n", node->len, node->name);
                }
            }
            printf("  push rax\n");
            break;
        case ND_MEMBER:
            /*
             * struct Box b;
             * b.a = 22;
             */
            printf("  # begin: member %.*s\n", node->len, node->name);
            gen_address(node->lhs);
            gen(node->rhs);
            printf("  pop rdi\n");
            printf("  pop rax\n");
            printf("  add rax, rdi\n");
            printf("  push rax\n");
            printf("  # end: member %.*s\n", node->len, node->name);
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
        case ND_DEREF_ARRAY_POINTER: {
            /*
             * int global_array_array[2][3];
             * int (*test)[3] = &(global_array_array[1]);
             *                                        ↑
             */
            gen(node->lhs);
            break;
        }
        default: {
            error("代入の左辺値が変数ではありません\n");
            exit(1);
        }
    }
}

void gen(Node *node) {
    switch (node->kind) {
        case ND_NOTHING:
            return;
        case ND_FUNC: {
            ___COMMENT___2("begin function call [%.*s]", node->len, node->name);
            int count = 0;
            if (node->args) {
                for (Node *arg = node->args; arg; arg = arg->args) {
                    gen(arg);
                    count++;
                }
                for (int i = 0; i < count; i++) {
                    printf("  pop %s\n", registers_64[count - i - 1]);
                }
            }
            static bool doAlign = true;
            // RSPはpushやpopで64bit(=8bytes)ずつ動く
            // 関数のプロローグで32bit(=4bytes)ずつ動くこともあるが、それについては調整済み
            if (doAlign) {
                // https://github.com/rui314/chibicc/commit/aedbf56c3af4914e3f183223ff879734683bec73
                // We need to align RSP to a 16 byte boundary before
                // calling a function because it is an ABI requirement.
                // RAX is set to 0 for variadic function.
                int context = node->contextual_suffix;
                printf("  mov rax, rsp\n");
                printf("  and rax, 0xF\n");
                printf("  jnz .Lcall%d\n", context);
                printf("  mov rax, 0\n");
                printf("  call %.*s\n", node->len, node->name);
                printf("  jmp .Lend%d\n", context);
                printf(".Lcall%d:\n", context);
                printf("  sub rsp, 8\n");
                printf("  mov rax, 0\n");
                printf("  call %.*s\n", node->len, node->name);
                printf("  add rsp, 8\n");
                printf(".Lend%d:\n", context);
                printf("  push rax\n");
            } else {
                printf("  mov rax, %i\n", count);
                printf("  call %.*s\n", node->len, node->name);
                printf("  push rax\n");
            }
            ___COMMENT___2("end function call [%.*s]", node->len, node->name);

            return;
        }
        case ND_BLOCK: {
            ___COMMENT___("block begin");
            for (Node *next = node->statement;
                 next;
                 next = next->statement) {
                gen(next);
            }
            ___COMMENT___("block end");
            return;
        }
        case ND_BREAK: {
            const int context = node->contextual_suffix;
            printf("  jmp  .Lbreak%d\n", context);
            return;
        }
        case ND_SWITCH: {
            ___COMMENT___("switch begin");
            // 条件式の右辺を評価してスタックに積む
            gen(node->lhs);
            int count = 0;
            const int context = node->contextual_suffix;
            // 条件式の右辺をスタックから降ろす
            printf("  pop rax\n");
            bool has_default = false;
            for (struct Case *c = node->cases; c; c = c->next) {
                if (c->default_) {
                    count++;
                    has_default = true;
                } else {
                    printf("  cmp rax, %d\n", c->value);
                    printf("  je .Lcase%d_%d\n", context, count++);
                }
            }
            if (has_default) {
                // 全部当てはまらなかったらdefaultラベルへ
                printf("  je .Lcase%d_default\n", context);
            } else {
                // defaultラベルも無ければ終了
                printf("  jmp  .Lbreak%d\n",context);
            }
            count = 0;
            for (struct Case *c = node->cases; c; c = c->next) {
                if (c->default_) {
                    count++;
                    printf(".Lcase%d_default:\n", context);
                } else {
                    printf(".Lcase%d_%d:\n", context, count++);
                }
                // caseまたはdefaultが続いていて、statementが無い場合もある
                if (c->statement) {
                    gen(c->statement);
                }
            }
            // end:
            printf(".Lbreak%d:\n", context);
            ___COMMENT___("switch end");
            return;
        }
        case ND_FOR: {
            ___COMMENT___("for begin");
            int context = node->contextual_suffix;
            // init
            if (node->lhs)
                gen(node->lhs);
            // begin
            printf(".Lbegin%d:\n", context);
            // condition
            if (node->condition) {
                gen(node->condition);
                printf("  pop rax\n");
                printf("  cmp rax, 0\n");
                // if 0, goto end
                printf("  je .Lbreak%d\n", context);
            }
            // execute
            gen(node->execution);
            // post execute
            if (node->rhs) {
                gen(node->rhs);
            }
            // goto begin
            printf("  jmp .Lbegin%d\n", context);
            // end:
            printf(".Lbreak%d:\n", context);
            ___COMMENT___("for end");
            return;
        }
        case ND_WHILE: {
            ___COMMENT___("while begin");
            int context = node->contextual_suffix;
            // begin:
            printf(".Lbegin%d:\n", context);
            // condition
            gen(node->condition);
            printf("  pop rax\n");
            printf("  cmp rax, 0\n");
            // if 0, goto end
            printf("  je  .Lend%d\n", context);
            // execute & goto begin
            gen(node->lhs);
            printf("  jmp .Lbegin%d\n", context);
            // end:
            printf(".Lend%d:\n", context);
            ___COMMENT___("while end");
            return;
        }
        case ND_IF: {
            int context = node->contextual_suffix;
            if (node->rhs) {
                gen(node->condition);
                printf("  pop rax\n");
                printf("  cmp rax, 0\n");
                printf("  je  .Lelse%d\n", context);
                gen(node->lhs);
                printf("  jmp .Lend%d\n", context);
                printf(".Lelse%d:\n", context);
                gen(node->rhs);
                printf(".Lend%d:\n", context);
            } else {
                gen(node->condition);
                printf("  pop rax\n");
                printf("  cmp rax, 0\n");
                printf("  je .Lend%d\n", context);
                gen(node->lhs);
                printf(".Lend%d:\n", context);
            }
            return;
        }
        case ND_LOGICAL_OR: {
            ___COMMENT___("|| begin");
            // 短絡評価
            int context = node->contextual_suffix;
            // 左辺
            gen(node->lhs);
            printf("  pop rax\n");
            printf("  cmp rax, 0\n");
            printf("  jne .Ltrue%d\n", context);
            // 右辺
            gen(node->rhs);
            printf("  pop rax\n");
            printf("  cmp rax, 0\n");
            printf("  jne .Ltrue%d\n", context);
            // どちらもfalse
            printf("  mov rax, 0\n");
            printf("  jmp .Lend%d\n", context);
            // true
            printf(".Ltrue%d:\n", context);
            printf("  mov rax, 1\n");
            printf(".Lend%d:\n", context);
            printf("  push rax\n");
            ___COMMENT___("|| end");
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
            printf("  jmp .Lreturn.%.*s\n", node->len, node->name);
            return;
        case ND_NUM:
            printf("  push %d\n", node->val);
            return;
        case ND_STR_LITERAL:
            printf("  mov rax, OFFSET FLAT:%.*s\n", node->label_length, node->label);
            printf("  push rax\n");
            return;
        case ND_MEMBER:
        case ND_VARIABLE:
            // 変数のアドレスから、そのアドレスにある値を取り出す
            gen_address(node);
            load(node);
            return;
        case ND_VARIABLE_ARRAY:
            // 配列変数は不変で、先頭アドレスをそのまま使う
            gen_address(node);
            return;
        case ND_ASSIGN: {
            ___COMMENT___("assign begin");
            // 型ごとのサイズ
            const int type_size = get_size(find_type(node));
            const char *const prefix = size_prefix(type_size);
            const char *const register_name = register_name_rax(type_size);
            if (node->rhs->kind == ND_NUM) {
                gen_address(node->lhs);
                printf("  pop rdi\n");
                // 即値
                const int imm = node->rhs->val;
                printf("  mov %s [rdi], %d\n", prefix, imm);
                printf("  push %d\n", imm);
            } else {
                gen_address(node->lhs);
                gen(node->rhs);
                printf("  pop rax\n");
                printf("  pop rdi\n");
                printf("  mov %s [rdi], %s\n", prefix, register_name);
                printf("  push rax\n");
            }
            ___COMMENT___("assign end");
            return;
        }
        case ND_ADDRESS:
            // 変数のアドレスをスタックに積むだけ
            gen_address(node->lhs);
            return;
        case ND_INVERT: {
            gen(node->lhs);
            // TODO godbolt.orgは
            //  intやsize_tの反転だと
            //   test eax, eax
            //  boolの反転だと
            //   test eax, eax
            //   に加えて何やら複雑なことをしてる
            //   （boolが1バイトだから？）
            //  testは論理積が0ならzeroフラグを立てる
            /*
             * TESTはANDと同じ、ただし第一オペランドが変化しない
             * CMPはSUBと同じ、ただし第一オペランドが変化しない
             */
            ___COMMENT___("!");
            // 0と比較して
            printf("  cmp rax, 0\n");
//            printf("  test rax, rax\n");
            // 等しければ1をセット
            printf("  sete al\n");
            printf("  movzb rax, al\n");
            printf("  push rax\n");
            return;
        }
        case ND_DEREF:
            gen(node->lhs); // ポインタ変数の値（アドレス）をスタックに積む
            load(node); // それをロードする
            return;
        case ND_DEREF_ARRAY_POINTER:
            gen(node->lhs); // ポインタ変数の値（アドレス）をスタックに積む
            return;
        case ND_ADD:
        case ND_SUB:
        case ND_MUL:
        case ND_DIV:
        case ND_MOD:
        case ND_EQL:
        case ND_NOT:
        case ND_LESS:
        case ND_LESS_EQL:
            break;
    }

    // 2項演算
    gen(node->lhs);
    gen(node->rhs);
    printf("  pop rdi\n");
    printf("  pop rax\n");

    switch (node->kind) {
        case ND_ADD:
            printf("  add rax, rdi\n");
            break;
        case ND_SUB:
            printf("  sub rax, rdi\n");
            break;
        case ND_MUL:
            printf("  imul rax, rdi\n");
            break;
        case ND_DIV:
            // cqo命令を使うと、RAXに入っている64ビットの値を128ビットに伸ばしてRDXとRAXにセットする
            printf("  cqo\n");
            // idivは暗黙のうちにRDXとRAXを取って、それを合わせたものを128ビット整数とみなして、
            // それを引数のレジスタの64ビットの値で割り、商をRAXに、余りをRDXにセットする
            printf("  idiv rdi\n");
            break;
        case ND_MOD:
            printf("  cqo\n");
            printf("  idiv rdi\n");
            printf("  mov rax, rdx\n");
            break;
        case ND_EQL:
            ___COMMENT___("==");
            printf("  cmp rax, rdi\n");
            printf("  sete al\n");
            printf("  movzb rax, al\n");
            break;
        case ND_NOT:
            ___COMMENT___("!=");
            printf("  cmp rax, rdi\n");
            printf("  setne al\n");
            printf("  movzb rax, al\n");
            break;
        case ND_LESS:
            ___COMMENT___("<");
            printf("  cmp rax, rdi\n");
            printf("  setl al\n");
            printf("  movzb rax, al\n");
            break;
        case ND_LESS_EQL:
            ___COMMENT___("<=");
            printf("  cmp rax, rdi\n");
            printf("  setle al\n");
            printf("  movzb rax, al\n");
            break;
        case ND_LOGICAL_OR:
        case ND_RETURN:
        case ND_EXPR_STMT:
        case ND_IF:
        case ND_WHILE:
        case ND_FOR:
        case ND_SWITCH:
        case ND_BREAK:
        case ND_BLOCK:
        case ND_FUNC:
        case ND_NUM:
        case ND_STR_LITERAL:
        case ND_VARIABLE:
        case ND_VARIABLE_ARRAY:
        case ND_ADDRESS:
        case ND_INVERT:
        case ND_DEREF:
        case ND_DEREF_ARRAY_POINTER:
        case ND_ASSIGN:
        case ND_NOTHING:
        case ND_MEMBER:
            break;
    }

    printf("  push rax\n");
}

void arguments_to_stack(Variable *param) {
    // 関数の引数
    printf("  lea rax, [rbp - %d]\n", param->offset);
    int len = param->len;
    char *name = param->name;
    const char *const prefix = size_prefix(param->type_size);
    const char *const reg = register_name_for_args(param->type_size, param->index);
    printf("  # parameter [%.*s]\n", len, name);
    printf("  mov %s [rax], %s\n", prefix, reg);
}

void prepare_stack(int stack_size, Variable *const parameters) {
    if (stack_size) {
        while (stack_size % 16 != 0) {
            stack_size += 1;
        }
        // スタックに領域を確保
        printf("  sub rsp, %i  # stack size\n", stack_size);

        int count = 0;
        for (Variable *param = parameters; param; param = param->next) {
            if (param->index < 0) {
                // 引数がvoidの場合
                continue;
            }
            // レジスタで受け取った引数の値をスタックに積む
            arguments_to_stack(param);
            count++;
        }
        printf("  # %i %s\n", count, "arguments prepared");
    }
}

void generate_function(Function *func) {
    printf(".global %s\n", func->name);
    printf("%s:\n", func->name);

    // プロローグ
    printf("  push rbp\n");
    printf("  mov rbp, rsp\n");
    prepare_stack(func->stack_size, func->parameters);

    for (int i = 0; i < func->body->count; ++i) {
        Node *node = func->body->memory[i];
        if (node == NULL)
            break;

        // 抽象構文木を下りながらコード生成
        gen(node);
    }

    // エピローグ
    // 最後の式の結果がRAXに残っているのでそれが返り値になる
    printf(".Lreturn.%s:\n", func->name);
    printf("  mov rsp, rbp\n");
    printf("  pop rbp\n");
    printf("  ret\n");

    if (func->next) generate_function(func->next);
}

void generate_global(Global *globals) {
    for (Global *global = globals; global; global = global->next) {
        if (global->target->directive == _enum) {
            continue;
        }
        // ラベル
        printf("%.*s:\n", global->label_length, global->label);
        for (Directives *target = global->target; target; target = target->next) {
            switch (target->directive) {
                case _zero: {
                    Type *const type = target->backwards_struct;
                    if (type) {
                        load_struct(type);
                        target->value = get_size(type);
                    }
                    printf("  .zero %d\n", target->value);
                    break;
                }
                case _byte:
                    printf("  .byte %d\n", target->value);
                    break;
                case _long:
                    printf("  .long %d\n", target->value);
                    break;
                case _quad: {
                    int offset = target->value;
                    if (0 < offset) {
                        printf("  .quad %.*s%s%d\n",
                               target->reference_length, target->reference, "+", offset);
                    } else if (target->value < 0) {
                        printf("  .quad %.*s%s%d\n",
                               target->reference_length, target->reference, "-", offset);
                    } else {
                        printf("  .quad %.*s\n",
                               target->reference_length, target->reference);
                    }
                    break;
                }
                case _string: {
                    printf("  .string \"%.*s\"\n", target->literal_length, target->literal);
                    break;
                }
                case _enum:
                    break;
            }
        }
    }
}

void generate(struct Program *program) {
    printf(".intel_syntax noprefix\n");
    if (program->globals) {
        printf("\n");
        printf(".data\n");
        generate_global(program->globals);
    }
    printf("\n");
    printf(".text\n");
    generate_function(program->functions);
}