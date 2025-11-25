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

add_executable(graph_examples
    examples.cpp
    src/context.cpp
    src/instruction.cpp
)

target_include_directories(graph_examples PRIVATE include)

add_executable(loop_examples
    loop_analyser_examples.cpp
    src/context.cpp
    src/instruction.cpp
)

target_include_directories(loop_examples PRIVATE include)


find_package(GTest REQUIRED)

add_executable(run_tests test_optimizations.cpp
    src/instruction.cpp
    src/context.cpp
)

target_link_libraries(run_tests GTest::GTest GTest::Main)
target_include_directories(run_tests PRIVATE include)

enable_testing()
add_test(NAME GraphExamples COMMAND graph_examples)
add_test(NAME OptimizationsTest COMMAND run_tests)