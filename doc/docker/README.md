# Pocketnet.Core

[Pocketnet](https://pocketnet.app/about) is a decentralized social network based on the blockchain.
There is no central authority or corporation. Platform is run by equal
nodes on a blockchain with no centralized server. All revenue is split
between node operators and content creators. Node operators stake Pocketcoin
in order to mint blocks with rewards and transactions fees. Half of rewards
in each block go to content creators based on ratings their content gathers
from users. [Read more in the article.](https://pocketnet.app/docs/Pocketnet%20Whitepaper%20Draft%20v2.pdf)

## Getting Started

This software allows you to participate in the work of the blockchain network - [`https://pocketnet.app/help`](https://pocketnet.app/help?page=faq).

To start a node independently, you need basic skills of working with the operating system, understanding the principle of the blockchain network. A deeper level of personal computer proficiency is welcome.

## Minimum System Requirements
Below are the minimum system requirements to run a PocketNet node to use as a wallet, for coin staking to earn PKOIN, and to use as a local node on your home network. A node with these requirements may need to be upgraded within the next 1 - 2 years.

- 2 core x86-64 CPU
- 4GB RAM
- 100 GB harddrive
- 10 Mbps internet connection

## Recommended System Requirements
The below system requirements are recommended for a node which will act as a public node to service the Bastyon front end, in addition to wallet, and coin staking capabilities. The recommended requirements should provide enough headroom to support the PocketNet network for at least 2 years into the future.

- 4 core x86-64 CPU
- 16 GB RAM
- 500 GB SSD Harddrive
- 100 Mbps internet connection
- Publicly accessible IP address and ports (see Network Setup below)

## Docker installation
In order to run this container you'll need docker installed.

* [Windows](https://docs.docker.com/windows/started)
* [OS X](https://docs.docker.com/mac/started/)
* [Linux](https://docs.docker.com/linux/started/)

## Usage

### Just run:
```shell
$ docker run -d \
    --name=pocketnet.core \
    --ulimit nofile=65536:65536 \
    --log-driver local \
    --log-opt max-size=10m \
    --log-opt max-file=3 \
    -p 37070:37070 \
    -p 37071:37071 \
    -p 38081:38081 \
    -p 8087:8087 \
    -v ~/.pocketcoin:/home/pocketcoin/.pocketcoin \
    pocketnetteam/pocketnet.core:latest
```

### We recommend using `docker-compose.yml` to configure the node:
```yml
version: '3.7'
services:
  pocketnet.core:
    container_name: pocketnet.core
    image: pocketnetteam/pocketnet.core:latest
    restart: on-failure
    stop_grace_period: 1m30s
    # Increasing the number of available file descriptors
    ulimits:
      nofile:
        soft: "65536"
        hard: "65536"
    # Create a Volume for the Blockchain database directory
    volumes:
      - ~/.pocketcoin:/home/pocketcore/.pocketcoin 
    ports:
      # To accept connections from other network nodes
      - 37070:37070
      # Manage node. Be careful - port 37071 opens access to your node and wallet
      - 37071:37071
      # To accept HTTP POST requests along the path 127.0.0.1:38081/public/
      - 38081:38081
      # For the ability to establish a WebSocket connection to a node to support notifications
      - 8087:8087
    logging:
      driver: "local"
      options:
        max-size: "10m"
        max-file: "3"
```

### Access to node API over CLI tool
```shell
$ docker exec -it pocketnet.core pocketcoin-cli help
$ docker exec -it pocketnet.core /bin/sh
```

## Help
You can get help and useful information from different sources:
- https://pocketnet.app/help
- https://github.com/pocketnetteam/pocketnet.core/tree/master/doc/help
- https://github.com/pocketnetteam/pocketnet.core/blob/master/share/examples/pocketcoin.conf
- Contact section below

## License
Pocketnet Core is released under the terms of the Apache 2.0 license. See [LICENSE](https://github.com/pocketnetteam/pocketnet.core/blob/master/LICENSE) for more
information or see https://opensource.org/licenses/Apache-2.0.

## Find Us

* [GitHub](https://github.com/pocketnetteam/pocketnet.core)
* [pocketnet.app](https://pocketnet.app)

## Contacts
support@pocketnet.app - general questions

core@pocketnet.app - blockchain nodes