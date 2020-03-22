cmake_minimum_required(VERSION 3.10)
project(ccompiler C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-builtin-declaration-mismatch")
#set(CMAKE_C_FLAGS  "-Wall -Wextra -Wconversion")


include_directories(.)

add_executable(ccompiler
        array.c
        codegen.c
        common.h
        collection.c
        collection.h
        enum.c
        function_declare.c
        global.c
        goto.c
        main.c
        parse.c
        parse.h
        std_alternative.h
        struct.c
        switch.c
        tokenize.c
        type.c
        type_def.c
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