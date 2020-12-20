## Implementing an Interface

1. Interface
1. Mapping
1. Component Realization

**Interface** is the set of public Sinks and output channels, JSON schemas for
input and output Packets, and optionally tests that validate behavior. It does
not contain any implementation.

An Interface an ICohesiveGroup and an ISource. The ICohesiveGroup's sub-nodes
are the Interface's inputs, and the output channels of the Interface's ISource
are the Interface's outputs.

**Mapping** maps inputs from the Interface to the Component Realization, and
outputs of the Component Realization to the Interface's outputs.

**Component Realization** is the implementation, contained in a DataGraphNode.

## Instantiating an Interface

The Interface takes a Mapping JSON object, and the Component Realization JSON
object. It constructs a DataGraphNode using the Component Realization JSON, and
uses the Mapping JSON to map it's public Nodes to the created DataGraphNode's
internal nodes. The Interface also creates one PassThroughNode for each output
Channel and adds it to the DataGraphNode.

## Schema differences between the Interface and Realization

It is possible to use different schemas for the Interface and Realization,
subject to Liskov's Substitution Principle: An Interface's input cannot be
mapped to a Realized Node with a stronger schema than the Interface's input, and
a Realized Node's output channel schema cannot be weaker than the Interface
output's schema.

**OK**

Interface Input: Requires "a", "b", "c"
Realization Input: Requires "a"

**NOT OK**

Interface Input: Requires "a"
Realization Input: Requires "a", "b", "c"

**OK**

Interface Output: Requires "a"
Realization Output: Requires "a", "b", "c"

**NOT OK**

Interface Output: Requires "a", "b", "c"
Realization Output: Requires "a"

## Example JSON

Interface JSON

```json5
{
  "inputs": {
    "INPUT_NAME_1": {
      "inputSchema": "path/to/schema"
    },
    "INPUT_NAME_2": {
      "inputSchema": "another/schema"
    }
  },
  "outputChannels": {
    "OUTPUT_CHANNEL_1": {
      "outputSchema": "path/to/output/schema"
    }
  },
  "tests": {
    "TEST_NAME": {
      "test": "path/to/test/config",
      // One input and one output
      "testType": "simple",
      "inputNode": "INPUT_NAME_1",
      "outputChannel": "OUTPUT_CHANNEL_1"
    }
  }
}
```

Component Realization JSON

```json5
{
  "nodesToCreate": {
    "COMPONENT_OBJECT_NAME": {
      // NodeRegistration implementation name
      "nodeImplementation": "IMPL",
      // NodeRegistration initParameters
      "initParameters": {}
    },
    "ANOTHER_COMPONENT_OBJECT_NAME": {
      "nodeImplementation": "IMPL2",
      "initParameters": {},
      "inputSchema": "path/to/schema",
      "outputSchemas": {
        "SOME_OUTPUT_CHANNEL": "path/to/schema"
      }
    }
  },
  "connections": [
    {
      "fromNode": "COMPONENT_OBJECT_NAME",
      // optional
      "fromPathableId": "PATHABLE_ID_1",
      "channel": "FROM_CHANNEL",
      "toNode": "ANOTHER_COMPONENT_OBJECT_NAME",
      // optional
      "toPathableId": "PATHABLE_ID_2",
      // optional
      "packetDeliveryType": "DIRECT_OR_QUEUE_TYPE"
    }
  ]
}
```

Mapping JSON

```json5
{
  "inputMapping": [
    {
      "interfaceNode": "INPUT_NAME_1",
      "realizedNode": "COMPONENT_OBJECT_NAME"
    },
    {
      "interfaceNode": "INPUT_NAME_2",
      "realizedNode": "ANOTHER_COMPONENT_OBJECT_NAME"
    }
  ],
  "outputMapping": [
    {
      "realizedNode": "ANOTHER_COMPONENT_OBJECT_NAME",
      "realizedNodeChannel": "OUTPUT_CHANNEL_OF_NODE_IMPL",
      "interfaceOutputChannel": "OUTPUT_CHANNEL_1"
    }
  ]
}
```
