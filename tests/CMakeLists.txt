cmake_minimum_required(VERSION 3.21)
project(tests)

set(CMAKE_CXX_STANDARD 17)

find_package(GTest REQUIRED)
find_package(nlohmann_json REQUIRED)

add_executable(dummy-test dummy-test.cpp)
add_executable(state-test state-test.cpp ../core/src/state/state.cpp)
add_executable(source-test source-test.cpp ../core/src/source/source.cpp)

target_link_libraries(dummy-test ${GTEST_LIBRARIES})
target_link_libraries(state-test ${GTEST_LIBRARIES})
target_link_libraries(source-test ${GTEST_LIBRARIES} nlohmann_json::nlohmann_json)

add_test(dummy-test "./dummy-test")
add_test(state-test "./state-test")
add_test(source-test "./source-test")