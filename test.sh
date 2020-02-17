#!/bin/bash

COUNT=$#

try() {
  assert "$1" "int main() { $2 }"
}

assert2() {
  ./build/ccompiler "--file" "$(pwd)/_test/test_0.c" >tmp.s
  gcc -static -o tmp tmp.s build/libfoo.a
  ./tmp
}

assert() {
  expected="$1"
  input="$2"

  ./build/ccompiler "$input" >tmp.s
  gcc -static -o tmp tmp.s build/libfoo.a
  if [ "$COUNT" == 0 ]; then
    ./tmp >/dev/null
  else
    ./tmp
  fi
  actual="$?"

  printf "▓"

  if [ "$actual" != "$expected" ]; then
    echo
    echo "$input => $expected expected, but got $actual"
    exit 1
  fi
}

# TODO メモ
# popとpushは64bitのレジスタやアドレスのみ
# mov rax, [rbp-4]はできるが
# mov [rbp-8], raxは足りない分がゼロクリアされる
# mov DWORD PTR [rbp-4], 5で即値を32bit枠に入れられる
# TODO アセンブリのビルドと実行と確認
# $ gcc -static -o tmp tmp.s; ./tmp; echo $?
# TODO gdb入門
# $ gdb ./tmp
# (gdb) display /4i $pc
# (gdb) start
# (gdb) stepi

#make
# For CMake 3.13 or later, use these options to set the source and build folders
# $ cmake -B/path/to/my/build/folder -S/path/to/my/source/folder
# For older CMake, use some undocumented options to set the source and build folders:
# $ cmake -B/path/to/my/build/folder -H/path/to/my/source/folder
# https://stackoverflow.com/a/24435795
/usr/bin/cmake -DCMAKE_BUILD_TYPE=Debug -G "CodeBlocks - Unix Makefiles" -Bbuild/ -H.
/usr/bin/cmake --build ./build --target ccompiler -- -j 4
/usr/bin/cmake --build ./build --target foo -- -j 4

assert2

#assert 8 "$(
#  cat <<END
#int main() {
#  return a();
#}
#END
#)"

#assert 7 "$(
#  cat <<END
#int deja;
#int deja() {
#  return 3;
#}
#int main() {
#  return deja() + 4;
#}
#END
#)"

#assert 3 "$(
#  cat <<END
#int main() {
#  int *a;
#  *a = 1; // 未定義動作っぽい？
#  return 1;
#}
#END
#)"

try 99 'int a; a = 0; int i; for (i = 0; i < 10; i = i + 1) a = a + 1; return 99;'
try 10 'int a; a = 0; int i; for (i = 0; i < 10; i = i + 1) a = a + 1; return a;'
try 10 'int a; a = 0; while (a < 10) a = a + 1; return a;'

try 37 'int a; int b; a = 1983; b = 2020; if ((b - a) == 37) return 37; else return 36;'
try 12 'int a; a = 13; if (a == 0) return 3; else return 12;'
try 12 'if (0) return 3; else return 12;'

try 25 'int a_3; int _loc; a_3 = 12; _loc = 3; return a_3 * _loc - 11;'
try 25 'int a_3; int _loc; a_3 = 12; _loc = 3; return a_3 * _loc - 11; 24;'

try 5 'int aaa; aaa = 3; aaa + 2;'
try 5 'int b29; int aaa; aaa = 3; b29 = 2; b29 + aaa;'
try 5 'int O0; O0 = 3; O0 = 2; O0 + 3;'

try 5 'int a; a = 3; a + 2;'
try 3 'int a; a = 3; a;'
try 3 'return 3;'
try 5 'int a; int z; a = 3; z = 2; return a + z;'

try 0 "return 0;"
try "42" "return 42;"
try 42 "40+2;"
try 0 '100-100;'
try 10 '100-100+10;'
try 111 '100 + 10 +1;'
try 100 '100 * 10 / 10;'
try 101 '100 + 10 / 10;'
try 0 '10 * -1 + 10;'
try 90 '100 + -1 * 10;'

try 0 '0 == 1;'
try 1 '42 == 42;'
try 1 '0 != 1;'
try 0 '42 != 42;'
try 1 '42 != 42 + 1;'
try 1 '2 + 42 != 42;'
try 1 '(2 + 42) != 42;'

try 1 '0 < 1;'
try 0 '1 < 1;'
try 0 '2 < 1;'
try 1 '0 <= 1;'
try 1 '1 <= 1;'
try 0 '2 <= 1;'

try 1 '1 > 0;'
try 0 '1 > 1;'
try 0 '1 > 2;'
try 1 '1 >= 0;'
try 1 '1 >= 1;'
try 0 '1 >= 2;'

echo OK
