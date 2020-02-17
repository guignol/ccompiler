.intel_syntax noprefix

.data
.LC.0:
  .string "日本語ですね\n"
.LC.1:
  .string "abc"

.text
.global main
main:
  push rbp
  mov rbp, rsp

  mov rax, OFFSET FLAT:.LC.1
  mov rsi, OFFSET FLAT:.LC.0
  sub rax, rsi
  mov rdx, rax # 文字数
  mov rsi, OFFSET FLAT:.LC.0 # 文字列の先頭アドレス
  mov rdi, 1 # 標準出力
  mov rax, 1 # write
  syscall

  mov rax, 111
  mov rsp, rbp
  pop rbp
  ret
