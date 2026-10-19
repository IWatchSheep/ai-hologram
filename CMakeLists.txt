# CMake is a build-configuration tool. It reads this file and generates the
# instructions used to compile and link the project's C++ files.
# This file selects the language rules, compiler warnings, and build targets.

# Require CMake 3.20 or newer. This is the minimum version needed by the
# project's planned CTest command, `ctest --test-dir`.
cmake_minimum_required(VERSION 3.20)

# Give the project its name, version, and implementation language.
project(ai_hologram VERSION 0.0.0 LANGUAGES CXX)

# Use the C++17 language standard. "Required" means do not silently fall back
# to an older standard. Turning extensions off avoids compiler-specific
# additions, keeping the code within standard C++.
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Ask compilers to report likely mistakes and questionable code. Warnings are
# not errors: they inform the learner without making every warning stop a build.
if(MSVC)
    # /W4 enables a broad, useful level of warnings in Microsoft's compiler.
    add_compile_options(/W4)
else()
    # These enable common, extra, and standard-conformance warnings elsewhere.
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# A target is one named result CMake knows how to build. A library is compiled
# code for other targets to use; unlike a program, it has no main() entry point.
add_library(ai_hologram_core STATIC
    src/core/project_info.cpp
    src/world/position.h
    src/world/action.h
    src/world/action.cpp
    src/world/world_state.h
    src/world/world_state.cpp
    src/world/rules.h
    src/world/rules.cpp
    src/world/world_text.h
    src/world/world_text.cpp
    src/world/environment.h
    src/world/environment.cpp
    src/demo/scripted_demo.h
    src/demo/scripted_demo.cpp
    src/demo/prediction_demo.h
    src/demo/prediction_demo.cpp
    src/model/belief.h
    src/model/prediction.h
    src/model/budget_counter.h
    src/model/budget_counter.cpp
    src/model/world_model.h
    src/model/world_model.cpp
    src/compare/state_difference.h
    src/compare/state_difference.cpp
    src/compare/difference_text.h
    src/compare/difference_text.cpp
)

# Tell this target and its users to search src/ when resolving project headers.
# PUBLIC makes this include path available to code that links to the library.
target_include_directories(ai_hologram_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# An executable target is a runnable program with its own main() function.
add_executable(ai_hologram
    src/main.cpp
)

# Linking connects a program with the compiled library code it calls.
target_link_libraries(ai_hologram PRIVATE ai_hologram_core)

# This separate program checks the same shared core code as the main program.
add_executable(ai_hologram_tests
    tests/test_main.cpp
    tests/test_support.h
    tests/test_support.cpp
    tests/test_suites.h
    tests/test_project_info.cpp
    tests/test_world_text.cpp
    tests/test_rules.cpp
    tests/test_environment.cpp
    tests/test_budget_counter.cpp
    tests/test_world_model.cpp
    tests/test_compare.cpp
)

# Linking lets the test program call functions from the shared core library.
target_link_libraries(ai_hologram_tests PRIVATE ai_hologram_core)

# CTest runs registered tests and reports whether each program passed or failed.
enable_testing()

# A zero return value means the test passed; a nonzero value means it failed.
add_test(NAME correctness_tests COMMAND ai_hologram_tests)
