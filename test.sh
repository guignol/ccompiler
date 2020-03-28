#!/bin/bash

check() {
  if [ "$1" == 0 ]; then
    echo "$2"
  else
    exit "$1"
  fi
}

#make
# For CMake 3.13 or later, use these options to set the source and build folders
# $ cmake -B/path/to/my/build/folder -S/path/to/my/source/folder
# For older CMake, use some undocumented options to set the source and build folders:
# $ cmake -B/path/to/my/build/folder -H/path/to/my/source/folder
# https://stackoverflow.com/a/24435795
/usr/bin/cmake -DCMAKE_BUILD_TYPE=Debug -G "CodeBlocks - Unix Makefiles" -B./build/ -H.
/usr/bin/cmake --build ./build --target clean -- -j 4
/usr/bin/cmake --build ./build --target ccompiler -- -j 4
check "$?" "OK: stage1 compiler is built"

mkdir -p stage1
rm -f stage1/ccompiler
cp build/ccompiler stage1

# テストコード
TEST_CODE="$(pwd)/_test/test_0.c"
#TEST_CODE="$(pwd)/_test/debug.c"

./stage1/ccompiler "--file" "$TEST_CODE" >stage1/tmp.s
check "$?" "OK: stage1 test is compiled"
gcc -static -o stage1/tmp stage1/tmp.s
check "$?" "OK: stage1 test is built"
./stage1/tmp >/dev/null
check "$?" "OK: stage1 test is passed"

if [ "$1" != "self" ]; then
  exit 0
fi

# stage2 compiler
mkdir -p stage2
rm -f stage2/ccompiler
rm -f stage2/tmp.s
rm -f stage2/tmp

test_self_compile() {
  FILE_NAME="$1"
#  echo ${FILE_NAME}.c
  # プリプロセス
  gcc -E ${FILE_NAME}.c | cat -s >stage2/${FILE_NAME}_.c
  # コンパイルに成功するまでファイルを吐かない
  t=$(mktemp)
  ./stage1/ccompiler "--file" "$(pwd)/stage2/${FILE_NAME}_.c" >$t && cat $t >stage2/${FILE_NAME}_.s
  rm -f $t
  # 検索の邪魔なので削除しておく
  rm -f stage2/${FILE_NAME}_.c
}

FILES=()
FILES+=("array")
FILES+=("codegen")
FILES+=("collection")
FILES+=("enum")
FILES+=("function_declare")
FILES+=("global")
FILES+=("goto")
FILES+=("main")
FILES+=("parse")
FILES+=("struct")
FILES+=("switch")
FILES+=("tokenize")
FILES+=("type")
FILES+=("type_def")

for e in "${FILES[@]}"; do
    test_self_compile "${e}"
done

pushd stage2 >/dev/null || exit 1
gcc -static -o ccompiler "${FILES[@]/%/_.s}"
#gcc -c -o "parse_.o" "parse_.s"
#gcc -c -o "struct_.o" "struct_.s"
result="$?"
for e in "${FILES[@]}"; do rm -f "${e}_.s" ; done
check "$result" "OK: stage2 compiler is built"

# stage2 test to be compiled by stage2 compiler
popd >/dev/null || exit 1
t=$(mktemp)
./stage2/ccompiler "--file" "$TEST_CODE" >$t && cat $t >./stage2/tmp.s
#./stage2/ccompiler "--file" "$TEST_CODE" >$t && cat $t >./stage2/tmp.s
result="$?"
rm -f $t
check "$result" "OK: stage2 test is compiled"

# stage2 test のアセンブル、リンク
gcc -static -o ./stage2/tmp ./stage2/tmp.s
check "$?" "OK: stage2 test is built"

# stage2 test の実行
./stage2/tmp
check "$?" "OK: stage2 test is passed"

#gdb --args ./stage2/ccompiler "--file" "$(pwd)/_test/test_0.c"
