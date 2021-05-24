# To open access to your node's public data, you need to prepare the node.
## Basic requirements:
1. Availability of a dedicated public IPv4 address
2. Open ports 37070, 38081 and 8087
3. If your node gets access to the global Internet, port forwarding through your router is required
4. Valid node settings to allow accepting requests to public methods

## Details:

1.  A dedicated IPv4 address can be obtained from an Internet service provider or a hosting service provider.
    When you rent a virtual cloud machine, you usually already get an additional public address, often even a static one.
    If you are running a node on a home computer, then you need to clarify the possibility of getting a dedicated IPv4.
    > Search query: "How to get a public IP address"

2.  Depending on your network and hardware configuration, you may also need to open additional ports.:
    37070 - to accept connections from other network nodes
    38081 - to accept HTTP POST requests along the path *.*.*.*:38081/public
    8087  - for the ability to establish a WebSocket connection to a node to support notifications
    Opening ports may require additional work in the settings of the router, operating system, or hosting control panels.
    > Search query: "How to open a port windows" or "How to open a port linux" or "How to open a port <your_router_model>"

3.  If your host is hidden from the Internet by a router or router you may also need to forward the ports
    (37070, 38081, 8087).
    > Search query: "How to forward a port <your_router_model>"

4.  Example of node configuration (pocketcoin.conf):
    ```
    server=1
    listen=1
    port=37070
    rpcport=38081
    # This opens access to the node management interface from external network
    rpcallowip=0.0.0.0/0
    rpcthreads=1
    rpcworkqueue=1    
    rpcpostthreads=3
    rpcpostworkqueue=100
    rpcpublicthreads=5
    rpcpublicworkqueue=100
    wsuse=1
    wsport=8087
    ```
