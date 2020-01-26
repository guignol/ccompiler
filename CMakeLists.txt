cmake_minimum_required(VERSION 3.10)
project(ccompiler C)

set(CMAKE_C_STANDARD 11)

include_directories(.)

add_executable(ccompiler
        9cc.h
        codegen.c
        main.c
        parse.c
        tokenize.c)
