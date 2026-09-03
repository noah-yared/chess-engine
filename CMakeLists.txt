cmake_minimum_required(VERSION 3.20)

project(engine VERSION 1.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# enable compile_commands.json for linters
set(CMAKE_EXPORT_COMPILE_COMMANDS ON CACHE BOOL "Generate compile_commands.json for linters" FORCE)

# enable address sanitizer for debugging
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -O0 -fsanitize=address -fno-omit-frame-pointer")

# remove exception handling and rtti and enable optimizations
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -march=native -mtune=native -fomit-frame-pointer -flto -fno-exceptions -fno-rtti -DNDEBUG")

set (CMAKE_CXX_FLAGS_RELWITHDEBINFO "-g -O3 -DNDEBUG")

# generate engine_config.h
configure_file(${CMAKE_SOURCE_DIR}/src/app/engine_config.h.in ${CMAKE_BINARY_DIR}/engine_config.h)

# engine library for reuse in other targets
add_library(engine_lib STATIC
  src/board/bitboards.cpp
  src/board/board_state.cpp
  src/eval/evaluate.cpp
  src/move/move_list.cpp
)
target_include_directories(engine_lib
  PUBLIC
    ${CMAKE_SOURCE_DIR}/src
)

# engine binary
add_executable(engine
  src/app/cli.cpp
  src/app/main.cpp
)
target_include_directories(engine
  PRIVATE
    ${CMAKE_BINARY_DIR}
)
target_link_libraries(engine
  PRIVATE
    engine_lib
)

# disable googl test/benchmark installation
set(INSTALL_GTEST OFF CACHE BOOL "Disable GoogleTest installation" FORCE)
set(BENCHMARK_ENABLE_INSTALL OFF CACHE BOOL "Disable GoogleBenchmark installation" FORCE)

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
  tests/perft_test.cpp
  tests/search_test.cpp
  tests/thread_pool_test.cpp
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

if (INSTALL_ENGINE)
  # install engine binary
  install(TARGETS engine DESTINATION bin)

  # write the binary installation path to engine_config.json (for portability)
  set(ENGINE_BINARY_INSTALL_PATH ${CMAKE_INSTALL_PREFIX}/bin/engine)
  configure_file(
    ${CMAKE_SOURCE_DIR}/engine_config.json.in
    ${CMAKE_SOURCE_DIR}/engine_config.json
    FILE_PERMISSIONS OWNER_READ GROUP_READ WORLD_READ # read only permissions
    @ONLY
  )
endif()
