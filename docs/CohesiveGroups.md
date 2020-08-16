A CohesiveGroup is a group of nodes that can share native objects.

They serve as implementations for Nodes.

```
"cohesiveGroupInstances": {
    "HTTP TCP Server": {}
},
"cohesiveGroupImplementations": {
    "HTTP TCP Server": {
        "type": "internal",
        "name": "UV TCP Server"
    }
},
"instanceImplementations": {
    "TCP Receive": {
        "type": "CohesiveGroupNode",
        "cohesiveGroupInstance": "HTTP TCP Server",
        "cohesiveNode": "TCP Receiver" 
    }
}
```
