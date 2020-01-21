#include "9cc.h"

int main(int argc, char **argv)
{
  if (argc != 2)
  {
    fprintf(stderr, "引数の個数が正しくありません\n");
    return 1;
  }

  // トークナイズする
  Token *token = tokenize(argv[1]);
  // Node *code[100];
  Node **code = (Node **)malloc(sizeof(Node *) * 100);
  program(token, code);

  // アセンブリの前半部分を出力
  printf(".intel_syntax noprefix\n");
  printf(".global main\n");
  printf("main:\n");

  // プロローグ
  // 変数26個分の領域を確保する
  printf("  push rbp\n");
  printf("  mov rbp, rsp\n");
  if (locals)
  {
    printf("  sub rsp, %i\n", locals->offset);
  }

  for (size_t i = 0; i < 100; i++)
  {
    Node *node = code[i];
    if (node == NULL)
    {
      continue;
    }
    // 抽象構文木を下りながらコード生成
    generate(node);
    // 式の評価結果としてスタックに一つの値が残っている
    // はずなので、スタックが溢れないようにポップしておく
    printf("  pop rax\n");
  }

  // エピローグ
  // 最後の式の結果がRAXに残っているのでそれが返り値になる
  printf("  mov rsp, rbp\n");
  printf("  pop rbp\n");
  printf("  ret\n");
  return 0;
}