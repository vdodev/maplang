

### Scaling

Scalability is achieved through Graph-splitting.

Nodes can run on separate threads, in separate processes or on separate systems. Those systems can be local or hosted with your favorite cloud provider.

When load becomes too high (on a thread, in a system, bandwidth in a data center, etc.), node instances are created in another graph.

Contextual Nodes are distributed automatically. A Contextual Node creates a new node instance for a given key/value pair in the parameters, which can easily be moved to another thread. When scaling to a separate physical CPU or system is needed, existing nodes cannot be moved, but new instances are created in other Graphs.

Multi-threading can be done manually too. Divide-and-conquer algorithms can accept an input buffer like an image, send that image to several worker Nodes along with bounds to process, and each worker writes its results into a provided buffer.

### Redundancy

A hot spare of a Graph can be kept running in the event of a crash.

It's also possible to send a Packet along redundant paths - each path being identical. At the end of a redunant path, the first packet to arrive for a given index is used.

### Fail-over

Failures happen. Power goes out, generators and battery backups fail, hardware fails, software crashes, configurations get messed up, somebody forgets to pay the internet bill, etc.


