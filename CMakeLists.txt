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
    tests/graph_examples.cpp
    src/context.cpp
    src/instruction.cpp
)

target_include_directories(graph_examples PRIVATE include)

add_executable(loop_examples
    tests/loop_analyser_examples.cpp
    src/context.cpp
    src/instruction.cpp
)

target_include_directories(loop_examples PRIVATE include)


find_package(GTest QUIET)
if(NOT GTest_FOUND)
    include(FetchContent)
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG        v1.14.0
    )
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)
    # Create aliases matching the find_package target names
    if(NOT TARGET GTest::GTest)
        add_library(GTest::GTest ALIAS gtest)
    endif()
    if(NOT TARGET GTest::Main)
        add_library(GTest::Main ALIAS gtest_main)
    endif()
endif()

add_executable(run_tests tests/test_optimizations.cpp
    src/instruction.cpp
    src/context.cpp
)

target_link_libraries(run_tests GTest::GTest GTest::Main)
target_include_directories(run_tests PRIVATE include)

add_executable(run_liveness_tests tests/test_liveness.cpp
    src/instruction.cpp
    src/context.cpp
)
target_link_libraries(run_liveness_tests GTest::GTest GTest::Main)
target_include_directories(run_liveness_tests PRIVATE include)

enable_testing()
add_test(NAME GraphExamples COMMAND graph_examples)
add_test(NAME OptimizationsTest COMMAND run_tests)
add_test(NAME LivenessTest COMMAND run_liveness_tests)