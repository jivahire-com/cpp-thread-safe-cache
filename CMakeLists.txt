cmake_minimum_required(VERSION 3.14)
project(lru_cache_challenge CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

file(GLOB TEST_SOURCES tests/*.cpp)

add_executable(tests ${TEST_SOURCES})
target_include_directories(tests PRIVATE include)
# No external test framework required; tests use a simple runner
