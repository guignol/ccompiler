#!/bin/bash

#make
# For CMake 3.13 or later, use these options to set the source and build folders
# $ cmake -B/path/to/my/build/folder -S/path/to/my/source/folder
# For older CMake, use some undocumented options to set the source and build folders:
# $ cmake -B/path/to/my/build/folder -H/path/to/my/source/folder
# https://stackoverflow.com/a/24435795
/usr/bin/cmake -DCMAKE_BUILD_TYPE=Debug -G "CodeBlocks - Unix Makefiles" -B./build/ -H.
/usr/bin/cmake --build ./build --target clean -- -j 4
/usr/bin/cmake --build ./build --target ccompiler -- -j 4

./build/ccompiler "--file" "$(pwd)/_test/test_0.c" >tmp.s
gcc -static -o tmp tmp.s
if [ "$#" == 0 ]; then
  ./tmp >/dev/null
else
  ./tmp
fi

if [ $? == 0 ]; then
  echo "OK"
else
  echo "exit code: $?"
  echo "NNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNG"
fi

exit 0;

# 1ファイルずつセルフホストを試す
mkdir -p build_tmp

test_self_compile() {
  FILE_NAME="$1"
  echo ${FILE_NAME}.c
  # プリプロセス
  gcc -E ${FILE_NAME}.c | cat -s > build_tmp/${FILE_NAME}_.c
  # コンパイルに成功するまでファイルを吐かない
  t=$(mktemp)
  ./build/ccompiler "--file" "$(pwd)/build_tmp/${FILE_NAME}_.c" >$t && cat $t > build_tmp/${FILE_NAME}_.s
  rm -f $t
  if [ -e build_tmp/${FILE_NAME}_.s ]; then
    echo "success!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"
  fi
  # 検索の邪魔なので削除しておく
  rm -f build_tmp/${FILE_NAME}_.c
}

test_self_compile "codegen"
test_self_compile "collection"
test_self_compile "enum"
test_self_compile "function_declare"
test_self_compile "global"
test_self_compile "main"
test_self_compile "parse"
test_self_compile "struct"
test_self_compile "tokenize"
test_self_compile "type"
