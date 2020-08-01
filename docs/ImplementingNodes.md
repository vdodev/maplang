# Maplang Node Implementor Overview

There is a standard library of Nodes that can be used in Maplang diagrams. Inevitably though, there will be some modules that need to be implemented.

Note: Currently nodes are implemented using C++, but other languages will be added in the future.

### Definitions

**Node** A component which handles a single responsibility. This is implemented in a traditional programming language, and is connected to other nodes to form a program.

**Packet** Contains a set of parameters, and optionally buffers (0 or more).

**Sink** A Node which receives Packets.

**Source** A Node which produces Packets.

**Pathable** A Node which receives and produces packets. It is different than an object which is both a Source and a Sink - it is possible that the path packets are pushed into the Graph differs depending on the source of an incoming Packet.

**Graph** An internal class which drives 

