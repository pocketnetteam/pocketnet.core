0.22.5 Release Notes
====================

Pocketnet Core version 0.22.5 is now available from:

  <https://github.com/pocketnetteam/pocketnet.core/releases/tag/0.22.5>

This release includes new features, various bug fixes and performance
improvements, as well as updated translations.

Please report bugs using the issue tracker at GitHub:

  <https://github.com/pocketnetteam/pocketnet.core/issues>

How to Upgrade
==============

If you are running an older version, shut it down. Wait until it has completely
shut down (which might take a few minutes in some cases), then run the
installer (on Windows) or just copy over `pocketcoind`/`pocketcoin-qt` (on Linux).

Upgrading directly from a version of Pocketnet Core that has reached its EOL is
possible, but it might take some time if the data directory needs to be migrated. Old
wallet versions of Pocketnet Core are generally supported.

Compatibility
==============

Pocketnet Core is supported and extensively tested on operating systems
using the Debian Linux, Ubuntu Linux 20+ (and clones like Mint Linux, etc.) and
Windows 7 and newer.  Pocketnet Core should also work on most other Unix-like systems but
is not as frequently tested on them.  It is not recommended to use Pocketnet Core on
unsupported systems.

Notable changes
===============

P2P and network changes
-----------------------
- Added support for running Pocketnet Core as an
  [I2P (Invisible Internet Project)](https://en.wikipedia.org/wiki/I2P) service
  and connect to such services. See [doc/i2p.md](https://github.com/pocketnetteam/pocketnet.core/blob/0.22/doc/i2p.md) for details (backported from bitcoin/bitcoin#20685)
- This release removes support for Tor version 2 hidden services in favor of Tor
  v3 only, as the Tor network [dropped support for Tor
  v2](https://blog.torproject.org/v2-deprecation-timeline) with the release of
  Tor version 0.4.6.  Henceforth, Pocketnet Core ignores Tor v2 addresses; it
  neither rumors them over the network to other peers, nor stores them in memory
  or to `peers.dat` (backported from bitcoin/bitcoin#22050)
- Full support has been added for the CJDNS network. See the new option -cjdnsreachable and [doc/cjdns.md](https://github.com/pocketnetteam/pocketnet.core/blob/0.22/doc/cjdns.md) (backported from bitcoin/bitcoin#23077)

New and Updated RPCs
--------------------

- The `getpeerinfo` RPC returns two new boolean fields, `bip152_hb_to` and
  `bip152_hb_from`, that respectively indicate whether we selected a peer to be
  in compact blocks high-bandwidth mode or whether a peer selected us as a
  compact blocks high-bandwidth peer. High-bandwidth peers send new block
  announcements via a `cmpctblock` message rather than the usual inv/headers
  announcements. See BIP 152 for more details (backported from bitcoin/bitcoin#19776)

- `getpeerinfo` no longer returns the following fields: `addnode`, `banscore`,
  and `whitelisted`, which were previously deprecated in 0.21. Instead of
  `addnode`, the `connection_type` field returns manual. Instead of
  `whitelisted`, the `permissions` field indicates if the peer has special
  privileges. The `banscore` field has simply been removed (backported from bitcoin/bitcoin#20755)

- The `getnodeaddresses` RPC now returns a "network" field indicating the
  network type (ipv4, ipv6, onion, or i2p) for each address (backported from bitcoin/bitcoin#21594)

- `getnodeaddresses` now also accepts a "network" argument (ipv4, ipv6, onion,
  or i2p) to return only addresses of the specified network (backported from bitcoin/bitcoin#21843)

Tools and Utilities
-------------------

- A new CLI `-addrinfo` command returns the number of addresses known to the
  node per network type (including Tor v3 and I2P) and total. This can be
  useful to see if the node knows enough addresses in a network to use options
  like `-onlynet=<network>` or to upgrade to this release of Pocketnet Core 0.22.5
  that supports Tor v3 only (backported from bitcoin/bitcoin#21595)

0.22.5 change log
===============
Full Changelog: [0.22.4...0.22.5](https://github.com/pocketnetteam/pocketnet.core/compare/0.22.4...0.22.5)

Backports from bitcoin
----------------------
- bitcoin/bitcoin#15423 torcontrol: Query Tor for correct -onion configuration
- bitcoin/bitcoin#19776 net, rpc: expose high bandwidth mode state via getpeerinfo
- bitcoin/bitcoin#20685 Add I2P support using I2P SAM
- bitcoin/bitcoin#20755 [rpc] Remove deprecated fields from getpeerinfo
- bitcoin/bitcoin#20788 net: add RAII socket and use it instead of bare SOCKET
- bitcoin/bitcoin#21595 cli: create -addrinfo
- bitcoin/bitcoin#21843 p2p, rpc: enable GetAddr, GetAddresses, and getnodeaddresses by network
- bitcoin/bitcoin#22050 p2p: remove tor v2 support

Credits
=======

Thanks to everyone who directly contributed to this release:

- Andy Oknen
- Network_Support
- HiHat

As well as to everyone that helped with translations on [Transifex](https://www.transifex.com/pocketnetteam/pocketnet-core/).
