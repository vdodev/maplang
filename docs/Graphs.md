A DataGraph is all nodes connected to each other in a single process which will all receive packets on the same thread.

An Atlas is all nodes connected to each other, and can span multiple DataGraphs. This can mean multiple threads in a single process, multiple processes, and multiple servers.

A single Node instance can only run on a single thread (without explicit multi-threading). Packets are still received on the same thread, and Packets should be sent from a single thread if the order of the outgoing Packets matters.

The Atlas could be stored in a database. It's only needed to connect nodes.

There is a single libuv context per DataGraph. It can be used directly if needed (I/O, message passing, etc.). There are also existing nodes that wrap libuv to handle most use cases.

Only Nodes in a PartitionedNode can share native objects.

Atlas::connect()
The Atlas can lookup the DataGraph for a given node.
It connects to the source's DataGraph, and adds a connection form the source to the target's node.
Only the source DataGraph is aware of the connection. The source node, target node, and target DataGraph are not aware of the connection.
