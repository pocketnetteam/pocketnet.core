macOS Build Instructions and Notes
====================================
The commands in this guide should be executed in a Terminal application.
The built-in one is located in `/Applications/Utilities/Terminal.app`.

Preparation
-----------
Install the macOS command line tools:

`xcode-select --install`

When the popup appears, click `Install`.

Then install [Homebrew](https://brew.sh).

Dependencies
----------------------

    brew install automake berkeley-db4 libtool boost miniupnpc openssl pkg-config protobuf python qt libevent qrencode

See [dependencies.md](dependencies.md) for a complete overview.

If you want to build the disk image with `make deploy` (.dmg / optional), you need RSVG

    brew install librsvg

Berkeley DB
-----------
It is recommended to use Berkeley DB 4.8. If you have to build it yourself,
you can use [the installation script included in contrib/](/contrib/install_db4.sh)
like so

```shell
CFLAGS=-Wno-error=implicit-function-declaration ./contrib/install_db4.sh .
```

from the root of the repository.

**Note**: You only need Berkeley DB if the wallet is enabled (see the section *Disable-Wallet mode* below).

OpenSSL
-------
For compiler to find `openssl` library you have to set the following environment variables

    export LDFLAGS="-L/usr/local/opt/openssl/lib"
    export CPPFLAGS="-I/usr/local/opt/openssl/include"

**Note**: This step is required if you see `configure: error: libcrypto missing` while building the Pocketcoin Core.

Build Pocketcoin Core
------------------------

1. Clone the Pocketcoin Core source code and cd into `pocketcoin`

        git clone https://github.com/pocketcoin/pocketcoin
        cd pocketcoin

2.  Build Pocketcoin Core:

    Configure and build the headless Pocketcoin Core binaries as well as the GUI (if Qt is found).

    You can disable the GUI build by passing `--without-gui` to configure.

        ./autogen.sh
        ./configure
        make

3.  It is recommended to build and run the unit tests:

        make check

4.  You can also create a .dmg that contains the .app bundle (optional):

        make deploy

Disable-wallet mode
--------------------
When the intention is to run only a P2P node without a wallet, Pocketcoin Core may be compiled in
disable-wallet mode with:

    ./configure --disable-wallet

In this case there is no dependency on Berkeley DB 4.8.

Mining is also possible in disable-wallet mode using the `getblocktemplate` RPC call.

Running
-------

Pocketcoin Core is now available at `./src/pocketcoind`

Before running, it's recommended that you create an RPC configuration file.

    echo -e "rpcuser=pocketcoinrpc\nrpcpassword=$(xxd -l 16 -p /dev/urandom)" > "/Users/${USER}/Library/Application Support/Pocketcoin/pocketcoin.conf"

    chmod 600 "/Users/${USER}/Library/Application Support/Pocketcoin/pocketcoin.conf"

The first time you run pocketcoind, it will start downloading the blockchain. This process could take several hours.

You can monitor the download process by looking at the debug.log file:

    tail -f $HOME/Library/Application\ Support/Pocketcoin/debug.log

Other commands:
-------

    ./src/pocketcoind -daemon # Starts the pocketcoin daemon.
    ./src/pocketcoin-cli --help # Outputs a list of command-line options.
    ./src/pocketcoin-cli help # Outputs a list of RPC commands when the daemon is running.

Notes
-----

* Tested on OS X 10.10 Yosemite through macOS 10.13 High Sierra on 64-bit Intel processors only.

* Building with downloaded Qt binaries is not officially supported. See the notes in [#7714](https://github.com/pocketcoin/pocketcoin/issues/7714)
