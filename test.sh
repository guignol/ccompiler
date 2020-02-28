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
