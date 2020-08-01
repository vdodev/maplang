# Maplang

Maplang is a diagram-based programming language.

### What problem does Maplang solve?

Logic in your code is processed by the language centers in your brain.

Architecture is different. Architecture is structure, and structure is visual. We should use our visual cortex to build architecture, not the language center of our brain.

Architecture diagrams have pros and cons. They are great for understanding how the code is structured, but can be seen as overhead, and can get out-of-sync with the codebase.

That's what Maplang solves.

Maplang diagrams are code. Logic is wired together in a diagram, and parameters flow along the connections you create between logic nodes.

Nodes (or components) contain your logic. Logic is still created using the IDE and language you choose. Nodes are then wired together in a diagram, and can be reused or swapped as needed.

### Increase velocity

Building architecture and wiring up your code can be time consuming and error prone. It's hard to keep the whole picture in your head, especially if you are working with code someone else wrote.

Maplang lets you absorb others design more naturally, and modify it with fewer errors.

### Testing is easier

Node inputs and outputs use nlohmann::json objects, so they don't need to be embedded in a live system to run.

You can create your own input and output test data, or save a sequence from production.

### Generic Data Format

nlohmann::json objects are used between nodes, and is defined with a [json-schema](https://json-schema.org). A json-schema is not limited to Javascript, and can be used in a variety of languages to describe and constrain data structures.

It is not parsed or serialized for same-thread communication.

Cross-thread communication does not parse/serialize either, and uses lock-free queues to pass parameters between threads.

For other cases like inter-process communication or network-based communication, the nlohmann::json objects can be transmitted in any of the formats that nlohman::json supports: JSON, MessagePack, CBOR, BSON, and UBJSON.

### Multi-threading

A node instance only ever executes in a single thread.

Multi-threading is achieved with nodes running in separate threads. This could mean several connected nodes each running in separate threads, or it could mean several instances of a node, each runnning in a separate thread processing a portion of the incoming data.

Nodes typically process their inputs on the same thread as those inputs are delivered, however it is possible for nodes to create their own background threads. When these asynchronous nodes produce parameters, they are pushed to a lock-free queue and delivered to the next node(s) on the thread they were originally delivered on.

### SOLID Principles are more natural in Maplang.

**Single responsibility** 

The software design phase comes first (or should!). When creating the diagram, it's natural to separate logical responsibilities. Nodes are then mapped to responsibilities.

**Open-closed**

Maplang nodes are open for extension, and closed for modification.

Extension in Maplang is more broad than object-oriented languages. Provided the input and output data formats and pre/post conditions are met, the interface is satisfied. This allows different implementations to be used based on changing requirements, or in different contexts, similar to the Strategy design pattern.

**Liskov Substitution**

This principle says that a function's pre-conditions cannot be strengthened, and post-conditions cannot be weakened.

Maplang pre-conditions cannot be strengthened beyond what you create in the diagram. A node cannot require more parameters, or a wider range of values than is set in the diagram. A node is free to weaken the pre-conditions, however. Nodes are free to be able to function with a wider range of values than the diagram requires, and nodes are not required to use every input parameter.

Post-conditions cannot be weakened. A node must output the parameters specified in the diagram (and range, if specified). The node is free to strengthen post-conditions though. It may output in a tighter range than required, or may send more parameters than required by the diagram.

**Interface Segregation**

Implementing an interface in Maplang is accomplished by wiring up a node's connections, which can lead wherever necessary.

Maplang goes a step further than traditional interface segregation by allowing an interface to be "implemented" by several other (potentially unrelated) nodes.

**Dependency Inversion**

The Dependency Inversion principle says that the abstraction should not depend on the details - the details should depend on the abstraction.

The abstraction is the data format being passed between nodes. Input and output parameters are specified in the diagram (abstraction). The nodes consume and must produce in the format set in the diagram.

This allows nodes to be swapped, even at run-time if needed.

### Diagram UI

The diagram UI uses [mxGraph](https://github.com/jgraph/mxgraph), the same library used in [Diagrams.net](https://www.diagrams.net/) (formerly Draw.io).

The diagram UI is currently a work-in-progress, and is a critical piece of Maplang. It's the piece that moves code into our visual cortex, and the piece that makes architecure self-documenting.

***
**Contributions are Welcome!**

Bug reports and pull requests can be submitted on [GitHub](https://github.com/vdodev/maplang).

You are welcome to review the existing code, submit use-cases, criticize the code, argue with us, become one of us, and anything else that improves Maplang.
