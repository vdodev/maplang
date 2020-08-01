cmake_minimum_required(VERSION 3.14)

include(FetchContent)
FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG release-1.10.0
)

FetchContent_MakeAvailable(googletest)

project(data_graph_tests)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(
        data_graph_tests
        MemoryStreamTests.cpp
        HttpRequestExtractorTests.cpp
        DataGraphTests.cpp)

target_link_libraries(
        data_graph_tests
        data_graph_lib
        gtest_main
)

target_include_directories(data_graph_tests PRIVATE ..)

add_test(NAME data_graph_tests1 COMMAND data_graph_tests)
