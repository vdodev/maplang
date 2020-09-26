There are two ways to test individual nodes - with input and output json, or by creating test Nodes to feed and receive data from the Node under test. This works well for nodes which both receive and send data. Nodes which only produce or only consume data are better tested with either custom test Nodes driving the test, or alternative testing methods.

**JSON Test Cases** are comprised of the node implementation, an input JSON blob, and the expected output JSON blob. For example:

```json
{
  "inputPacket": {
    "parameters": {},
    "buffers": [
      {"contentsFromFile": "file.bin"}
    ]
  },
  "expectedOutputPacket": {
    "fromChannel": "On Encoded",
    "parameters": {},
    "buffers": [
      {"stringContents": "TWFwbGFuZyB0ZXN0"}
    ]
  }
}
```

Notice that the Node's implementation was not specified. This test can be associated with one or more types in the Maplang UI. It can also be associated with specific implementations in the UI when necessary.

The output packet from the "ConvertToBase64" node is compared against the expected output packet. All non-objects and non-arrays listed in expectedOutputPacket are checked for equivalence. The Node may have added other parameters, which are ignored. If any parameter in expectedOutputPacket is missing or not equivalent, the test will fail. 

It's also possible to vary the number of input or output Packets. The order of packets received only matters from when those packets come from the same channel ("fromChannel").

```json
{
  "inputPackets": [
    {
      "parameters": {},
      "buffers": [
        {"stringContentsWithEscapes": "GET / HTTP/1.1\r\n\r\n"}
      ]
    }
  ],
  "outputPackets": [
    {
      "fromChannel": "On HTTP Request Headers",
      "parameters": {
        "http-version": "1.1",
        "http-path": "/",
        "http-method": "GET"
      }
    },
    {
      "fromChannel": "On HTTP Body Data",
      "parameters": {},
      "buffers": [
        {"stringContents": "<html><body></body></html>"}
      ]
    }
  ]
}
```
