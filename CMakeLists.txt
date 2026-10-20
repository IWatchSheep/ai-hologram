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
    src/agent/observation.h
    src/agent/footprint.h
    src/agent/footprint.cpp
    src/agent/dead_squares.h
    src/agent/dead_squares.cpp
    src/agent/heuristic.h
    src/agent/heuristic.cpp
    src/agent/computation_observer.h
    src/agent/step_predictor.h
    src/agent/step_predictor.cpp
    src/agent/records.h
    src/agent/agent_model.h
    src/agent/agent_model.cpp
    src/agent/investigation.h
    src/agent/investigation.cpp
    src/agent/verification.h
    src/agent/verification.cpp
    src/agent/planner.h
    src/agent/planner.cpp
    src/agent/planner_fallback.cpp
    src/agent/agent_log.h
    src/agent/agent_log.cpp
    src/agent/single_agent.h
    src/agent/single_agent.cpp
    src/agent/single_agent_check.cpp
    src/agent/single_agent_investigate.cpp
    src/agent/single_agent_choose.cpp
    src/agent/single_agent_search.cpp
    src/agent/naive_agent.h
    src/agent/naive_agent.cpp
    src/agent/run_loop.h
    src/agent/run_loop.cpp
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
    src/demo/agent_demo.h
    src/demo/agent_demo.cpp
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
    src/experiment/seeded_random.h
    src/experiment/seeded_random.cpp
    src/experiment/true_search.h
    src/experiment/true_search.cpp
    src/experiment/layout_generator.h
    src/experiment/layout_generator.cpp
    src/experiment/fault_event.h
    src/experiment/fault_events.h
    src/experiment/fault_events.cpp
    src/experiment/fault_injector.h
    src/experiment/fault_injector.cpp
    src/experiment/fault_apply.cpp
    src/experiment/fault_run.h
    src/experiment/fault_run.cpp
    src/experiment/scorer.h
    src/experiment/scorer.cpp
    src/experiment/scorer_life.cpp
    src/experiment/scorer_commit.cpp
    src/experiment/scorer_observe.cpp
    src/experiment/shadow.h
    src/experiment/shadow.cpp
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
    tests/test_footprint.cpp
    tests/test_footprint_cases.cpp
    tests/test_dead_squares.cpp
    tests/test_dead_squares_cases.cpp
    tests/test_step_predictor.cpp
    tests/test_planner.cpp
    tests/test_investigation.cpp
    tests/test_investigation_cases.cpp
    tests/test_verification.cpp
    tests/test_verification_cases.cpp
    tests/test_verification_trap.cpp
    tests/test_planner_cases.cpp
    tests/test_single_agent.cpp
    tests/test_single_agent_cases.cpp
    tests/test_single_agent_more.cpp
    tests/test_single_agent_sp7.cpp
    tests/test_single_agent_stops.cpp
    tests/test_naive_agent.cpp
    tests/test_agent_demo.cpp
    tests/test_seeded_random.cpp
    tests/test_layout_generator.cpp
    tests/test_layout_generator_sets.cpp
    tests/test_fault_events.cpp
    tests/test_fault_events_sets.cpp
    tests/test_fault_injector.cpp
    tests/test_fault_injector_f3.cpp
    tests/test_fault_injector_more.cpp
    tests/test_fault_injector_runs.cpp
    tests/test_fault_run.cpp
    tests/test_fault_run_demo.cpp
    tests/test_fault_run_empty.cpp
    tests/test_scorer.cpp
    tests/test_scorer_life.cpp
    tests/test_scorer_demo.cpp
    tests/test_shadow.cpp
    tests/test_shadow_more.cpp
    tests/test_shadow_sites.cpp
    tests/test_shadow_sites_more.cpp
    tests/test_shadow_runs.cpp
    tests/test_shadow_runs_demo.cpp
)

# Linking lets the test program call functions from the shared core library.
target_link_libraries(ai_hologram_tests PRIVATE ai_hologram_core)

# CTest runs registered tests and reports whether each program passed or failed.
enable_testing()

# A zero return value means the test passed; a nonzero value means it failed.
add_test(NAME correctness_tests COMMAND ai_hologram_tests)
