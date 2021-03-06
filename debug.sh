#!/bin/bash

# TODO gdb入門
# $ gdb ./tmp
# (gdb) display /9i $pc
# (gdb) start
# (gdb) stepi
# (gdb) info r

/usr/bin/cmake -DCMAKE_BUILD_TYPE=Debug -G "CodeBlocks - Unix Makefiles" -B./build/ -H.
/usr/bin/cmake --build ./build --target clean -- -j 4
/usr/bin/cmake --build ./build --target ccompiler -- -j 4
./build/ccompiler "--file" "$(pwd)/_test/debug.c" >tmp.s && gcc -static -o tmp tmp.s && ./tmp
echo "$?"

# アセンブリ修正時
# gcc -static -o tmp tmp.s && ./tmp ; echo "$?"