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

if [ $? ]; then
  echo "OK"
else
  echo "exit code: $?"
  echo "NNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNG"
fi

exit 0;

# 1ファイルずつセルフホストを試す
mkdir -p build_tmp
FILE_NAME="codegen"
FILE_NAME="parse"
FILE_NAME="function_declare"
FILE_NAME="tokenize"
FILE_NAME="type"
FILE_NAME="main"
FILE_NAME="struct"
FILE_NAME="global"
gcc -E ${FILE_NAME}.c | cat -s > build_tmp/${FILE_NAME}_.c
./build/ccompiler "--file" "$(pwd)/build_tmp/${FILE_NAME}_.c" > build_tmp/${FILE_NAME}_.s
