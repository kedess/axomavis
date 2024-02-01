cmake_minimum_required(VERSION 3.21)

project(axomavis)

add_subdirectory(core core)
if (TEST)
    enable_testing()
    add_subdirectory(tests tests)
endif ()


