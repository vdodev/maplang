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
        maplang_tests
        MemoryStreamTests.cpp
        HttpRequestExtractorTests.cpp
        DataGraphTests.cpp
        BlockingObjectPoolTests.cpp
        BufferPoolTests.cpp
        ParameterExtractorTests.cpp
        AddParameterTests.cpp
        PassThroughNodeTests.cpp
        DotExporterTests.cpp
        GraphBuilderTests.cpp
        ParameterRouterTests.cpp
        StreamUtilTests.cpp
        BufferAccumulatorNodeTests.cpp
        RingStreamTests.cpp
)

target_link_libraries(
        maplang_tests
        maplang
        gtest_main
)

target_include_directories(maplang_tests PUBLIC ${PROJECT_SOURCE_DIR}/../include)
target_include_directories(maplang_tests PRIVATE ${PROJECT_SOURCE_DIR}/../include-private)

add_test(NAME maplang_tests1 COMMAND maplang_tests)
