cmake_minimum_required(VERSION 3.14)

project(maplang)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(maplang
        include-private/logging.h
        include/maplang/ISource.h
        include/maplang/ISink.h
        src/DataGraph.cpp
        include/maplang/DataGraph.h
        include/maplang/Buffer.h
        include/maplang/Packet.h
        include/maplang/PathablePacket.h
        include/maplang/JsonGraphBuilder.h
        src/JsonGraphBuilder.cpp
        include/maplang/INode.h
        include/maplang/ISubgraphContext.h
        include/maplang/ICohesiveGroup.h
        include-private/nodes/SendOnce.h
        src/nodes/SendOnce.cpp
        include-private/nodes/VolatileKeyValueStore.h
        src/nodes/VolatileKeyValueStore.cpp
        include-private/nodes/HttpResponseWriter.h
        src/nodes/HttpResponseWriter.cpp
        include-private/nodes/UvTcpConnectionGroup.h
        src/nodes/UvTcpConnectionGroup.cpp
        include/maplang/NodeRegistration.h
        src/NodeRegistration.cpp
        include/maplang/IPathable.h
        include/maplang/IPacketPusher.h
        include-private/nodes/HttpRequestExtractor.h
        src/nodes/HttpRequestExtractor.cpp
        include-private/nodes/ContextualNode.h
        include/maplang/MemoryStream.h
        src/nodes/ContextualNode.cpp
        src/MemoryStream.cpp
        include/maplang/graph/DefaultGraphEdge.h
        include/maplang/graph/GraphElement.h
        include/maplang/graph/Graph.h
        include/maplang/graph/GraphImpl.h
        include/maplang/UvLoopRunner.h
        src/UvLoopRunner.cpp
        include/maplang/IUvLoopRunnerFactory.h
        include/maplang/UvLoopRunnerFactory.h
        include-private/nodes/PacketWriter.h
        src/nodes/PacketWriter.cpp
        include-private/nodes/PacketReader.h
        src/nodes/PacketReader.cpp
        src/nodes/VolatileKeyValueSet.cpp
        include-private/nodes/VolatileKeyValueSet.h
        src/BlueprintBuilder.cpp
        include/maplang/BlueprintBuilder.h
        include/maplang/Errors.h
        src/Errors.cpp
        src/LibuvUtilities.cpp
        include-private/LibuvUtilities.h
        include-private/Cleanup.h
        src/nodes/OrderedPacketSender.cpp
        include-private/nodes/OrderedPacketSender.h
        src/nodes/HttpRequestHeaderWriter.cpp
        include-private/nodes/HttpRequestHeaderWriter.h
        src/nodes/HttpResponseExtractor.cpp
        include-private/nodes/HttpResponseExtractor.h
        src/HttpUtilities.cpp
        include/maplang/HttpUtilities.h
        include/maplang/LambdaSink.h
        src/PathablePacket.cpp
        src/BufferPool.cpp
        include/maplang/BufferPool.h
        include/maplang/concurrentqueue.h
        include/maplang/blockingconcurrentqueue.h
        include/maplang/lightweightsemaphore.h
        src/nodes/ParameterExtractor.cpp
        include-private/nodes/ParameterExtractor.h
        include/maplang/JsonUtil.h
        include/maplang/Util.h
        src/nodes/AddParametersNode.cpp
        include-private/nodes/AddParametersNode.h
        src/TaskLooper.cpp
        include/maplang/TaskLooper.h
        include/maplang/BlockOnEmptyConcurrentQueue.h
        include/maplang/BlockingBufferPool.h
        src/BlockingBufferPool.cpp
        src/nodes/DataGraphNode.cpp
        include-private/nodes/DataGraphNode.h
        src/nodes/PassThroughNode.cpp
        include-private/nodes/PassThroughNode.h)

target_include_directories(maplang PUBLIC /usr/local/include)
target_include_directories(maplang PUBLIC ${PROJECT_SOURCE_DIR}/include)
target_include_directories(maplang PRIVATE ${PROJECT_SOURCE_DIR}/include-private)
set_target_properties(maplang PROPERTIES maplang_MODULE_DIRECTORY ${PROJECT_SOURCE_DIR}/include)

target_link_libraries(maplang /usr/local/lib/libuv_a.a dl pthread)

add_executable(http-ip-echo
        ip-echo-demo/ip-echo-main.cpp
        ip-echo-demo/HttpResponseWithAddressAsBody.cpp
        ip-echo-demo/HttpResponseWithAddressAsBody.h)

target_link_libraries(http-ip-echo maplang)

#add_executable(http-get
#        http-get-demo/http-get-main.cpp
#        http-get-demo/PrintBufferAsString.cpp
#        http-get-demo/PrintBufferAsString.h)
#target_link_libraries(http-get maplang)

option(USE_TESTS "Enable unit testing" OFF)
if(USE_TESTS)
    message("Maplang tests enabled.")
    enable_testing()
    add_subdirectory(tests)
endif()
