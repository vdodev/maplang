# Graph Config File Format

Graphs are loaded from a JSON blob.

Parts
* Graphs
    * Name (Must be unique)
    * May add other attributes later, like control APIs or required hardware, location, etc.
* Node Types
    * { "name": string, "channels": see next }
    * Output channels
        * { "name": string, "schemaUri": string }
    * Init schema
    * Input schema
* Node Instances
    * Node Type
    * Name/ID
    * Graph
    * Cohesive Group
* Node implementations
    * Maps a Node Instance to an implementation, so the class can be instantiated.
* Default node implementations
    * Maps a Node Type to an implementation, which is used for instances without a specified implementation.
* Cohesive Groups - groups of nodes which share native objects and must be located in the same DataGraph.
* Connections
    * From Instance
    * From Channel
    * From Pathable ID (optional)
    * To Instance
    * To Pathable ID (optional)

Simple example
```
"graphs": {
  "Graph 1": {} // May contain more info later
},
"nodeTypes": {
  "Timer": {
    "channels": [
      "Timer Fired": {
        "schemaUri": "timer.jsonSchema",
        "bufferCount": 0
      }
    ]
  },
  "Print Time": {
    "schemaUri": "timer.jsonSchema",
    "channels": [],
  }
},
"cohesiveGroups": {},
"nodeInstances": {
  "timer 1": {
    "type": "Timer",
    "graph": "Graph 1"
  },
  "time logger": {
    "type": "Print Time",
    "graph": "Graph 1"
  }
},
"defaultTypeImplementations": {
  "Timer": { "source": "maplang:Timer" }, // Used for all instance of this type
},
"instanceImplementations": {
  "time logger": { "source": "https://maplang.com/impl/print-time.zip" } // Used for a single instance, overriding anything specified for its type.
},
"cohesiveGroupImplementations": {},
"connections": [
  {
    "fromInstance": "timer 1",
    "fromChannel": "Timer Fired",
    "toInstance": "time logger"
  }
]
```

Factory nodes - accept a graphConfig as input, construct and connect.

How do they connect to existing nodes?
Using the instance ID.

What if two factories create nodes that should be connected?
When a factory finishes building and connecting, it sends an event containing the constructed config, with a map from the "blueprint instance ID" -> "built instance ID"

A parent factory can use sub-factories. The parent attaches a node ID to a sub-factory's blueprint node ID.
The parent may connect one sub-factory's blueprint node ID to another sub-factory's blueprint node ID.

Each factory has a "build" partition.
The main graph is also built with a factory, and not attached to anything.

When a factory is building, it uses the blueprint id to map it to a constructed id before connecting.
What about factories that create multiple subgraphs?
A Factory without subfactories doesn't do anything.
The parent factory triggers the subfactory to create the instances, then the parent connects the new instances.

Factories create nodes, and ship the parts with assembly required and assembly instructions (not assembler language).

A node can send a message to the parent factory to build. The parameters contain the blueprint.
The connections can be existing instance IDs, or reference a blueprint ID. To differentiate, the connections blob contains a "location" parameter for the to and from nodes which can be "in-graph" or "blueprint".
Maybe location could be another existing graph at some point.

Could rework it so connections always specify the graph with the connection. A graph ID could be routed to different locations if a failure occurs.

A node that produces a blueprint may not always produce the same blueprint. It may create nodes dynamically (say N nodes if there are N cores).

Example: demuxer, decoder, multiple encoders, muxer.
1. A request is received, and the node sends a blueprint to the factory to create a "buffer queue" and "demuxer format discovery node" to the graph.
1. The buffer queue and discovery node are both attached to the same previous node, so they each get the same packet (they are not attached to each other).
1. Once the format is determined, the discovery node sends a blueprint to the factory, containing connections (connect to the buffer queue), and disconnections (to disconnect itself).
1. The factory "build" node is pathable, so once the "built" event is sent, it send a payload to the buffer queue's "send" partition.
1. (When a blueprint is built, pathable IDs are created unique to each build, so when the newly build buffer queue sends to the factory's build node, the pathable gets sent back to its "send" partition.
1. A second node may be attached after the factory's "build" pathable, which gets a "built" config, which contains the running state, including active instance IDs.
1. The node can use this for whatever purpose, like mapping from another blueprint's created instances to a new blueprint ndoes that it creates later.

Sending to other graphs
1. nodes can be public or private
1. Public nodes are responsible for any authorization if needed.
1. A remote graph could create a public node with a unique ID, and send that ID back to the requester.
1. The local graph could then send a blueprint to its factory which only has a "connect" field.
1. On the remote side, the graph can be filtered by the connection ID, credentials, or request signing (e.g. if the user switches from wifi to data).
1. Authorization is up to the engineer.

Graph bridges - can be one-way or two-way. Allows packets to be sent from any node, to any public node in the other graph.
1. Can send packet serially, or can interleave packets. Bridge implementation should be selectable, just like any other node.
1. The IPacketPusher implementation in the Graph will route packets as needed on its incoming thread.
    1. Packets may go back to its own thread, to another in-process graph, to another process, or to another machine.
    1. Packets are always queued to the receiving graph. They may be routed to a bridge node (doesn't matter what type of node).
    1. Does the graph need to know about tunneling?
        1. No, the bridge can be multiple partitions with a shared transport. Wrapping and unwrapping happens in the partitions (possibly contextual nodes instead of partitions).
1. Packet tunneling
    1. Parameters are wrapped in a "tunneledParameters" object.
    1. Outer parameters contain a toInstanceId or toPathableId.
    1. Bridges to not use buffers, so the buffers from the source packet are passed-through in the same indices.
1. If the packet is tunneled to another thread, the parameters are simply unwrapped, and it is delivered to the correct node.
1. If the packet is tunneled to another process or server, it is serialized, transmitted, deserialized, then routed to the correct node.
1. RDMA needs to be supported.
    1. The "serializer" will handle this. It will serialize the parameters, then send the parameter data and buffer data via RDMA.
    1. The way the bridge endpoints communicate is an implementation detail.
1. If a Packet is not (fully) delivered, the bridge will attemt to re-connect. If that fails, the bridge node emits an error.
1. Bridge fail-over is up to the bridge. It may have multiple paths to the other graph. It may connect to a different graph (e.g. multi-master database).

##### Sending packets

Send from a: node, channel, (optionally) pathable id
Send to a: node, (optionally) pathable id

How does it get to another graph?
The connection has this information.

How is it specified when a connection is made?
Graphs have unique GUIDs.
A Node has a Graph object.
We don't have a native pointer to a remote graph object.
connect() needs to use IDs instead of native objects.

How does it differentiate between local and remote?
NodeIDs need a syntax
URIs? Could be relative Node ID (same graph), or could be a full URI to another graph's node.
What about nodes in the same process, but another graph? NodeID is unique within a process

What about nodes in the same machine, but another process? file://full/path/to/socket/or/named/pipe/NodeId
What about pipes on the command line? fd://1/NodeID or fd://stdout/NodeID

How to tell a graph where to listen? Stdin, websocket, raw socket, Infiniband, etc?

Remote graphs may expose their factory's "build" node.
An app could connect to a server and receive a blueprint to build the app.
It will need node implementations locally (because code signing).
But some types of updates will be instant, and can happen in the middle of using the app.

How can nodes send packets over a pipe if the unique nodeId isn't known?
Some nodes need static names.
The first public nodes that are built by the root factory have the same name as in the blueprint.

