#!/bin/bash

COUNT=$#

try() {
  assert "$1" "int main() { $2 }"
}

assert2() {
  ./build/ccompiler "--file" "$(pwd)/_test/test_0.c" >tmp.s
  gcc -static -o tmp tmp.s build/libfoo.a
  ./tmp
  return "$?"
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
if [ $? != 0 ]; then
    exit 1
fi

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
