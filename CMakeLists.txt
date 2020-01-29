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

add_library(foo STATIC
        _test/foo.c
        )

#enable_testing()
#
#find_program(WSL_SHELL bash)
#if (WSL_SHELL)
#    add_test(
#            NAME mytest
#            COMMAND ${WSL_SHELL} ${CMAKE_CURRENT_SOURCE_DIR}/test.sh
#            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
#    )
#endif (WSL_SHELL)