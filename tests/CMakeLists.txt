cmake_minimum_required(VERSION 3.21)
project(tests)

set(CMAKE_CXX_STANDARD 17)

find_package(GTest REQUIRED)

add_executable(dummy-test dummy-test.cpp)

target_link_libraries(dummy-test ${GTEST_LIBRARIES})

add_test(dummy-test "./dummy-test")