Pocketnet Core
===============

Setup
---------------------
Pocketnet Core is the original Pocketcoin client and it builds the backbone of the network. It downloads and, by default, stores the entire history of Pocketcoin transactions (which is currently more than 100 GBs); depending on the speed of your computer and network connection, the synchronization process can take anywhere from a few hours to a day or more.

To download Pocketnet Core, visit [pocketnet.core github repo](https://github.com/pocketnetteam/pocketnet.core/releases/).

Running
---------------------
The following are some helpful notes on how to run Pocketnet Core on your native platform.

### Unix

Unpack the files into a directory and run:

- `bin/pocketcoin-qt` (GUI) or
- `bin/pocketcoind` (headless)

### Windows

Unpack the files into a directory, and then run pocketcoin-qt.exe.

### macOS (planned)

Drag Pocketnet Core to your applications folder, and then run Pocketnet Core.

### Need Help?

* Ask for help on [Bastyon](http://webchat.freenode.net?channels=pocketcoin).

Building
---------------------
The following are developer notes on how to build Pocketnet Core on your native platform. They are not complete guides, but include notes on the necessary libraries, compile flags, etc.

- [Dependencies](dependencies.md)
- [macOS Build Notes](build-osx.md)
- [Unix Build Notes](build-unix.md)
- [Windows Build Notes](build-windows.md)
- [OpenBSD Build Notes](build-openbsd.md)
- [NetBSD Build Notes](build-netbsd.md)
- [Gitian Building Guide](gitian-building.md)

Development
---------------------
The Pocketnet repo's [root README](/README.md) contains relevant information on the development process and automated testing.

- [Developer Notes](developer-notes.md)
- [Release Notes](release-notes.md)
- [Release Process](release-process.md)
- [Source Code Documentation (External Link)](https://dev.visucore.com/pocketcoin/doxygen/)
- [Translation Process](translation_process.md)
- [Translation Strings Policy](translation_strings_policy.md)
- [Travis CI](travis-ci.md)
- [Unauthenticated REST Interface](REST-interface.md)
- [Shared Libraries](shared-libraries.md)
- [BIPS](bips.md)
- [Dnsseed Policy](dnsseed-policy.md)
- [Benchmarking](benchmarking.md)

### Miscellaneous
- [Assets Attribution](assets-attribution.md)
- [Files](files.md)
- [Fuzz-testing](fuzzing.md)
- [I2P Support](i2p.md)
- [Reduce Traffic](reduce-traffic.md)
- [Tor Support](tor.md)
- [Init Scripts (systemd/upstart/openrc)](init.md)
- [ZMQ](zmq.md)
- [PSBT support](psbt.md)

License
---------------------
Distributed under the [MIT software license](/COPYING).
This product includes software developed by the OpenSSL Project for use in the [OpenSSL Toolkit](https://www.openssl.org/). This product includes
cryptographic software written by Eric Young ([eay@cryptsoft.com](mailto:eay@cryptsoft.com)), and UPnP software written by Thomas Bernard.
