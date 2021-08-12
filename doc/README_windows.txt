Pocketcoin Core
=============


Intro
=============
Pocketcoin is a free open source peer-to-peer electronic cash system that is
completely decentralized, without the need for a central server or trusted
parties.  Users hold the crypto keys to their own money and transact directly
with each other, with the help of a P2P network to check for double-spending.


Launch daemon
=============
For launch Pocketnet Core Daemon  
To run the daemon, you must run the pocketcoind.exe (Default install C:\Program Files\PocketnetCore\daemon\pocketcoind.exe).
By default, the data directory is configured in %APPDATA%\Pocketcoin
To change the data directory, you must run pocketcoind.exe with parameter '-datadir=<full directory path>', example:
    C:\Program Files\PocketnetCore\daemon\pocketcoind.exe -datadir=C:\PocketcoinData

The configuration file is located in the data directory, for example: %APPDATA%\Pocketcoin\pocketcoin.conf

An example of a configuration file:

pocketcoin.conf
-----
port=37070
server=1

rpcallowip=0.0.0.0/0            # for access from anywhere
rpcallowip=127.0.0.1/24         # for access from only localhost
rpcallowip=192.168.0.0/0        # for access from local network

rpcport=37071
publicrpcport=38081
staticrpcport=80
rpcuser=<RPC_LOGIN>
rpcpassword=<RPC_PASSWORD>
-----

For more help use command:
    pocketcoind.exe --help


RPC client
=============
For use default RPC client:
    pocketcoin-cli.exe <command> ...

You can specify the server login and password:
    pocketcoin-cli.exe -rpcuser=<RPC_USER> -rpcpassword=<RPC_PASSWORD> ...

or use the path to the configuration file
    pocketcoin-cli.exe -datadir=<full directory path>

Example commands:
    pocketcoin-cli.exe getblockchaininfo
    pocketcoin-cli.exe getwalletinfo
    pocketcoin-cli.exe -rpcuser=user -rpcpassword=password getpeerinfo
    pocketcoin-cli.exe -datadir=C:\PocketcoinData getnetworkinfo

For more help use:
    pocketcoin-cli.exe --help









