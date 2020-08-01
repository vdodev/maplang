cmake_minimum_required(VERSION 3.14)

project(data_graph)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(data_graph_lib
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
        IPartitionedNode.h
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
        ISignalBroadcastContext.h
        graph/Edge.h
        graph/GraphElement.h
        graph/SignalBroadcaster.h
        graph/SignalBroadcaster.cpp
        graph/Graph.h
        graph/Graph.cpp
        UvLoopRunner.h
        UvLoopRunner.cpp
        nodes/PacketWriter.h
        nodes/PacketWriter.cpp
        nodes/PacketReader.h
        nodes/PacketReader.cpp
        nodes/SipRequestExtractor.cpp
        nodes/SipRequestExtractor.h
        nodes/VolatileKeyValueSet.cpp
        nodes/VolatileKeyValueSet.h sip-minimal-main.cpp)

target_include_directories(data_graph_lib PUBLIC /usr/local/include)
target_link_libraries(data_graph_lib /usr/local/lib/libuv_a.a dl pthread)

add_executable(data_graph main.cpp GraphLoader.cpp GraphLoader.h)
target_link_libraries(data_graph data_graph_lib)

option(USE_TESTS "Enable unit testing" ON)
if(USE_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
