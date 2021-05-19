
# Pocketnet CORE
![GitHub](https://img.shields.io/github/license/pocketnetteam/pocketnet.api)
![contributors](https://img.shields.io/github/contributors/pocketnetteam/pocketnet.core)
![last_commit](https://img.shields.io/github/last-commit/pocketnetteam/pocketnet.core)
![release_date](https://img.shields.io/github/release-date/pocketnetteam/pocketnet.core)
![version](https://img.shields.io/github/v/release/pocketnetteam/pocketnet.core)
![download_latest](https://img.shields.io/github/downloads/pocketnetteam/pocketnet.core/latest/total)

# What is Pocketcoin?

[Pocketnet](https://pocketnet.app/about) is a decentralized social network based on the blockchain.
There is no central authority or corporation. Platform is run by equal
nodes on a blockchain with no centralized server. All revenue is split
between node operators and content creators. Node operators stake Pocketcoin
in order to mint blocks with rewards and transactions fees. Half of rewards
in each block go to content creators based on ratings their content gathers
from users. [Read more in the article.](https://pocketnet.app/docs/Pocketnet%20Whitepaper%20Draft%20v2.pdf)

# What should I know?
This software allows you to participate in the work of the blockchain network - [`https://pocketnet.app/help`](https://pocketnet.app/help?page=faq).

To start a node independently, you need basic skills of working with the operating system, understanding the principle of the blockchain network. A deeper level of personal computer proficiency is welcome.

# Usage
PocketnetCore is distributed in two ways: binary installer and build from source code.

Minimum system requirements:
- 4 core CPU
- 12GB RAM
- 15GB free disk space
- 10Mbps internet connection


# Installation
## Linux (Ubuntu, Debian, Mint, etc.)
Install package with root privilegies. To do this, open the terminal in the directory where you downloaded the installer and execute commands:
```shell
$ sudo dpkg -i pocketnetcore_*_linux_x64_setup.deb
```
## Windows
Run the `pocketnetcore_*_win_x64_setup.exe` and follow the instructions of the installer.\
When you first start, the pocketnetcore desktop utility will ask for the location of the blockchain data directory. Default for Windows `%APPDATA%/Pocketcoin`, for linux `~/.pocketcoin`.

## Docker
Make sure that enough resources are allocated in your docker settings for the node to work from the section https://github.com/pocketnetteam/pocketnet.core#usage \
You can start your node with a single command from Docker.
```shell
$ docker run -d \
    --name=pocketnet.main \
    -p 37070:37070 \
    -p 37071:37071 \
    -p 8087:8087 \
    -v /var/pocketnet/.data:/home/pocketcoin/.pocketcoin \
    pocketnetteam/pocketnet.core:latest
```
Control
```shell
$ docker ps --format '{{.ID}}\t{{.Names}}\t{{.Image}}'
ea7759a47250    pocketnet.main      pocketnetteam/pocketnet.core:latest
$
$ docker exec -it pocketnet.main /bin/sh
$
$ pocketcoin-cli --help
$ pocketcoin-tx --help
```

# First full synchronization
To quickly synchronize and minimize traffic costs, you can run an empty node with additional parameters:
- `-listen=0` - disable the visibility of your node so that other novice nodes can't connect to you to download the blockchain.
- `-blocksonly=1` - specifies the mode of operation without transaction relay. In this way, the node will load the blocks as a whole, ignoring individual transactions on the network.

**After full synchronization, it is strongly recommended to disable these settings for the full operation of the node.**

You can get the full list of parameters:
```shell
$ pocketcoind --help
```

# Initialize blockhain data with database checkpoint
1. Stop the node.
2. Download database archive:
    * With torrent:
        ```
        magnet:?xt=urn:btih:cf4838bb06263064f8d8505565e9998692ef760c&dn=pocketnet.checkpoint.1182423.tar.gz&tr=udp%3a%2f%2ftracker.openbittorrent.com%3a80%2fannounce
        ```
    * With direct link from pocketnet.app host:
        ```
        http://pocketnet.app/checkpoints/pocketnet.checkpoint.latest.tar.gz
        ```
4. There must be archive tar.gz with 4 directories:
    ```shell
    blocks\
    chainstate\
    indexes\
    pocketdb\
    ```
4. Clean out everything except **wallet.dat** file, **wallets/** directory and **pocketcoin.conf** config file in the blockchain working directory and unpack the archive:
    ```shell
    # for unix
    $ cd ~/.pocketcoin/
     
    # or for windows
    $ cd %APPDATA%\Pocketcoin\
    
    # or for macos
    $ cd ~/Library/Application\ Support/Pocketcoin/
     
    # delete exists DB
    $ rm -r ./blocks
    $ rm -r ./chainstate
    $ rm -r ./indexes
    $ rm -r ./pocketdb
    
    # unpack new checkpoint DB
    $ tar -xzvf pocketnet.checkpoint.*.tar.gz -C ./
    ```
5. Make sure the folders and files inside are not set to "read only"
6. Start the node.

**VERY IMPORTANT**: save the **wallet.dat** file or **wallets/** files before cleaning the directory. It is recommended to even save these files somewhere for backup. 


# Build from source code
See `doc/build-*.md` files for build instructions.


# Help
You can get help and useful information from different sources:
- https://pocketnet.app/help
- https://github.com/pocketnetteam/pocketnet.core/tree/master/doc/help
- https://github.com/pocketnetteam/pocketnet.core/blob/master/share/examples/pocketcoin.conf
- Contact section below

# License
Pocketnet Core is released under the terms of the Apache 2.0 license. See [COPYING](COPYING) for more
information or see https://opensource.org/licenses/Apache-2.0.

# Contacts
support@pocketnet.app - general questions

core@pocketnet.app - blockchain nodes

