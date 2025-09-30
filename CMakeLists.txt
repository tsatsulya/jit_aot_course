cmake_minimum_required(VERSION 3.16)
project(jit_aot_course VERSION 1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic")
endif()

add_executable(jit_aot_course
    main.cpp
    src/context.cpp
    src/instruction.cpp
)

target_include_directories(jit_aot_course PRIVATE include)