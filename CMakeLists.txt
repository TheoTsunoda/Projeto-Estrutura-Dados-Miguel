cmake_minimum_required(VERSION 3.15)
project(imdb_ds C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# -O2 for realistic benchmark numbers; -g keeps debug symbols for CLion's debugger.
# -Wall -Wextra surface bugs early.
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O2 -g -Wall -Wextra")

add_executable(estrutura_dados_projeto
        src/main.c
        src/alloc.c
        src/timer.c
        src/loader.c
        src/linkedlist.c
        src/menu.c
        src/hashtable.c
        src/AVL.c
        src/skiplist.c
        src/trie.c
        src/radix.c
        src/operations.c
        src/benchmark.c
)

# So #include "title.h" etc. resolve from the src directory.
target_include_directories(estrutura_dados_projeto PRIVATE src)
