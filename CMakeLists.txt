cmake_minimum_required(VERSION 3.14)

project(maplang)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(maplang
        logging.h
        ISource.h
        ISink.h
        DataGraph.cpp
        DataGraph.h
        Buffer.h
        Packet.h
        PathablePacket.h
        JsonGraphBuilder.h
        JsonGraphBuilder.cpp
        INode.h
        nodes/tcp/UvTcpClient.h
        nodes/tcp/UvTcpClient.cpp
        nodes/tcp/UvTcpSender.h
        ISubgraphContext.h
        ICohesiveNode.h
        nodes/tcp/UvTcpReceiver.h
        nodes/tcp/UvTcpConnector.h
        nodes/SendOnce.h
        nodes/SendOnce.cpp
        nodes/VolatileKeyValueStore.h
        nodes/VolatileKeyValueStore.cpp
        nodes/HttpResponseWriter.h
        nodes/HttpResponseWriter.cpp
        nodes/HttpResponseWithAddressAsBody.h
        nodes/HttpResponseWithAddressAsBody.cpp
        nodes/tcp/UvTcpServerNode.h
        NodeRegistration.h
        NodeRegistration.cpp
        IPathable.h
        IPacketPusher.h
        nodes/HttpRequestExtractor.h
        nodes/HttpRequestExtractor.cpp
        ContextualNode.h
        MemoryStream.h
        ContextualNode.cpp
        MemoryStream.cpp
        nodes/tcp/UvTcpServerNode.cpp
        graph/DefaultGraphEdge.h
        graph/GraphElement.h
        graph/Graph.h
        graph/GraphImpl.h
        UvLoopRunner.h
        UvLoopRunner.cpp
        nodes/PacketWriter.h
        nodes/PacketWriter.cpp
        nodes/PacketReader.h
        nodes/PacketReader.cpp
        nodes/SipRequestExtractor.cpp
        nodes/SipRequestExtractor.h
        nodes/VolatileKeyValueSet.cpp
        nodes/VolatileKeyValueSet.h
        BlueprintBuilder.cpp
        BlueprintBuilder.h)

target_include_directories(maplang PUBLIC /usr/local/include)
target_link_libraries(maplang /usr/local/lib/libuv_a.a dl pthread)

add_executable(http-echo main.cpp )
target_link_libraries(http-echo maplang)

add_executable(sip-minimal sip-minimal-main.cpp)
target_link_libraries(sip-minimal maplang)
option(USE_TESTS "Enable unit testing" ON)
if(USE_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
