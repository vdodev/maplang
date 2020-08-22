cmake_minimum_required(VERSION 3.14)

project(maplang)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)



add_library(maplang
        logging.h
        include/maplang/ISource.h
        include/maplang/ISink.h
        DataGraph.cpp
        include/maplang/DataGraph.h
        include/maplang/Buffer.h
        include/maplang/Packet.h
        include/maplang/PathablePacket.h
        include/maplang/JsonGraphBuilder.h
        JsonGraphBuilder.cpp
        include/maplang/INode.h
        nodes/tcp/UvTcpClientGroup.h
        nodes/tcp/UvTcpClientGroup.cpp
        include/maplang/ISubgraphContext.h
        include/maplang/ICohesiveGroup.h
        nodes/SendOnce.h
        nodes/SendOnce.cpp
        nodes/VolatileKeyValueStore.h
        nodes/VolatileKeyValueStore.cpp
        nodes/HttpResponseWriter.h
        nodes/HttpResponseWriter.cpp
        nodes/HttpResponseWithAddressAsBody.h
        nodes/HttpResponseWithAddressAsBody.cpp
        nodes/tcp/UvTcpServerGroup.h
        nodes/tcp/UvTcpServerGroup.cpp
        include/maplang/NodeRegistration.h
        NodeRegistration.cpp
        include/maplang/IPathable.h
        include/maplang/IPacketPusher.h
        nodes/HttpRequestExtractor.h
        nodes/HttpRequestExtractor.cpp
        nodes/ContextualNode.h
        include/maplang/MemoryStream.h
        nodes/ContextualNode.cpp
        MemoryStream.cpp
        include/maplang/graph/DefaultGraphEdge.h
        include/maplang/graph/GraphElement.h
        include/maplang/graph/Graph.h
        include/maplang/graph/GraphImpl.h
        include/maplang/UvLoopRunner.h
        UvLoopRunner.cpp
        nodes/PacketWriter.h
        nodes/PacketWriter.cpp
        nodes/PacketReader.h
        nodes/PacketReader.cpp
        nodes/VolatileKeyValueSet.cpp
        nodes/VolatileKeyValueSet.h
        BlueprintBuilder.cpp
        include/maplang/BlueprintBuilder.h
        include/maplang/Errors.h
        LibuvUtilities.cpp
        LibuvUtilities.h)

target_include_directories(maplang PUBLIC /usr/local/include)
target_include_directories(maplang PUBLIC ${PROJECT_SOURCE_DIR}/include)
target_include_directories(maplang PRIVATE ${PROJECT_SOURCE_DIR}/include-private)
set_target_properties(maplang PROPERTIES maplang_MODULE_DIRECTORY ${PROJECT_SOURCE_DIR}/include)

target_link_libraries(maplang /usr/local/lib/libuv_a.a dl pthread)

add_executable(http-echo main.cpp src/Errors.cpp)
target_link_libraries(http-echo maplang)

option(USE_TESTS "Enable unit testing" ON)
if(USE_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
