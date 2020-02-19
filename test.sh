#!/bin/bash

COUNT=$#

assert() {
  ./build/ccompiler "--file" "$(pwd)/_test/test_0.c" >tmp.s
  gcc -static -o tmp tmp.s build/libfoo.a
  if [ "$COUNT" == 0 ]; then
    ./tmp >/dev/null
  else
    ./tmp
  fi
  return "$?"
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

assert
if [ $? != 0 ]; then
    exit 1
fi

echo OK
