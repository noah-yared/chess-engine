cmake_minimum_required(VERSION 3.14)

project(engine VERSION 1.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON CACHE BOOL "Generate compile_commands.json for linters" FORCE)

set(CMAKE_CXX_FLAGS_DEBUG "-fsanitize=address -fno-omit-frame-pointer")

# Define ASIO_STANDALONE to use standalone Asio instead of Boost.Asio
add_definitions(-DASIO_STANDALONE)

include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/309dab8d4bbfcef0ef428762c6fec7172749de0f.zip
)

# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

configure_file(${CMAKE_SOURCE_DIR}/include/engine_config.h.in ${CMAKE_BINARY_DIR}/engine_config.h)

add_executable(engine src/evaluate.cpp src/main.cpp src/movegen.cpp)

target_include_directories(engine PUBLIC ${CMAKE_SOURCE_DIR}/include PRIVATE ${CMAKE_BINARY_DIR} PRIVATE ${CMAKE_SOURCE_DIR}/external/crow/include PRIVATE ${CMAKE_SOURCE_DIR}/external/asio/asio/include)

enable_testing()

add_executable(engine_tests # declare test binary
    tests/check_detection_test.cpp
    tests/move_apply_test.cpp
    tests/move_undo_test.cpp
    tests/move_generation_test.cpp)
target_include_directories(engine_tests PUBLIC ${CMAKE_SOURCE_DIR}/include PUBLIC ${CMAKE_SOURCE_DIR}/tests/include)
target_link_libraries(engine_tests GTest::gtest_main) # provides basic main function to run tests

# Enable gtest to discover tests in test binary
include(GoogleTest)
gtest_discover_tests(engine_tests)
