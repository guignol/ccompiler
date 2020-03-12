cmake_minimum_required(VERSION 3.10)
project(ccompiler C)

set(CMAKE_C_STANDARD 11)
#set(CMAKE_C_FLAGS  "-Wall -Wextra -Wconversion")

include_directories(.)

add_executable(ccompiler
        common.h
        codegen.c
        function_declare.c
        global.c
        main.c
        parse.c
        tokenize.c
        type.c
        )

# TODO IDEに認識させるため
add_executable(test_0
        _test/test_0.c
        _test/debug.c
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