cmake_minimum_required(VERSION 3.14)

project(engine VERSION 1.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# enable compile_commands.json for linters
set(CMAKE_EXPORT_COMPILE_COMMANDS ON CACHE BOOL "Generate compile_commands.json for linters" FORCE)

# enable address sanitizer for debugging
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -O0 -fsanitize=address -fno-omit-frame-pointer")

# remove exception handling and rtti and enable optimizations
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -march=native -mtune=native -fomit-frame-pointer -flto -fno-exceptions -fno-rtti -DNDEBUG")

# generate engine_config.h
configure_file(${CMAKE_SOURCE_DIR}/include/engine_config.h.in ${CMAKE_BINARY_DIR}/engine_config.h)

# engine library for reuse in other targets
add_library(engine_lib STATIC
  src/bitboards.cpp
  src/board_state.cpp
  src/evaluate.cpp
  src/move_list.cpp
  src/position.cpp
)
target_include_directories(engine_lib
  PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)

# engine binary
add_executable(engine
  src/main.cpp
)
target_include_directories(engine
  PRIVATE
    ${CMAKE_BINARY_DIR}
    ${CMAKE_SOURCE_DIR}/external/crow/include
    ${CMAKE_SOURCE_DIR}/external/asio/asio/include
)
target_link_libraries(engine
  PRIVATE
    engine_lib
)

# googletest for testing
include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/309dab8d4bbfcef0ef428762c6fec7172749de0f.zip
)

# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()

add_executable(engine_tests # declare test binary
  # test files
  tests/check_detection_test.cpp
  tests/move_apply_test.cpp
  tests/move_undo_test.cpp
  tests/move_generation_test.cpp
)
target_include_directories(engine_tests
  PUBLIC
    ${CMAKE_SOURCE_DIR}/tests/include
)
target_link_libraries(engine_tests
  PRIVATE
    GTest::gtest_main
    engine_lib
) # provides basic main function to run tests

# Enable gtest to discover tests in test binary
include(GoogleTest)
gtest_discover_tests(engine_tests)

# googlebenchmark for benchmarking
FetchContent_Declare(
  googlebenchmark
  GIT_REPOSITORY https://github.com/google/benchmark.git
  GIT_TAG v1.9.4
)

# Disable benchmark testing
set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "Disable benchmark testing")
FetchContent_MakeAvailable(googlebenchmark)

# benchmarks
foreach(bench_name move_gen move_apply search static_eval)
  add_executable(bench_${bench_name}
    bench/${bench_name}_benchmark.cpp
  )
  target_include_directories(bench_${bench_name}
    PUBLIC
      ${CMAKE_SOURCE_DIR}/bench/include
  )
  target_link_libraries(bench_${bench_name}
    PRIVATE
      benchmark::benchmark
      benchmark::benchmark_main
      engine_lib
  )
endforeach()
