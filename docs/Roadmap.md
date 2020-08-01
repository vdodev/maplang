There is a lot of work to do. Contributions and partnerships are welcome.

1. Graph loader - reads a JSON configuration file and creates a graph.
1. Graph UI - use open source diagramming software as a base.
1. Standard library - commonly used components which are always available.
1. Market
    * Free - Vendors can provide Nodes free of charge if they choose. We support several free Nodes that you can use in your projects.
    * Sell Nodes for a 1-time fee, or recurring fee. Code runs on your servers.
    * Offer Node service on seller-owned servers (perhaps specialized - GPUs, custom ASICs, etc).
1. Graph Manager + API
    * Cloud provider integrations for orchestration
    * On-prem integrations (e.g. vmware)
1. Packet recorder - inserted into test code or possibly production code to capture a "Packet trace" to help replicate issues.
1. Parameter validation - json-schema is used to specify valid input and output parameters.
    * Schemas are part of the architecture. They are created in a text editor, with auto-complete to some degree. They can be referenced in diagrams as the schema for an input or output.
    * Debug "guards" that validate the input and output parameters match the diagram.
1. IDE plugin
    * Auto-complete in C++ based on schemas.
    * Diagram editor plugin.
    * Click-through from a node to its implementation (if source is available).
    * Ability to auto-generate skeleton code based on a Node in the diagram.
    * Error highlighting if a value is access that is not present in the schema, or is out of range.
