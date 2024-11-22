0.22.8 Release Notes
====================

Pocketnet Core version 0.22.8 is now available from:

  <https://github.com/pocketnetteam/pocketnet.core/releases/tag/0.22.8>

This release includes new features, various bug fixes and performance
improvements, as well as updated translations.

Please report bugs using the issue tracker at GitHub:

  <https://github.com/pocketnetteam/pocketnet.core/issues>

How to Upgrade
==============

If you are running an older version, shut it down. Wait until it has completely
shut down (which might take a few minutes in some cases), then run the
installer (on Windows) or just copy over `/Applications/Pocketcoin-Qt` (on Mac)
or just copy over `pocketcoind`/`pocketcoin-qt` (on Linux).

Upgrading directly from a version of Pocketnet Core that has reached its EOL is
possible, but it might take some time if the data directory needs to be migrated. Old
wallet versions of Pocketnet Core are generally supported.

Compatibility
=============

Pocketnet Core is supported and extensively tested on operating systems
using the Debian Linux, Ubuntu Linux 20+ (and clones like Mint Linux, etc.), macOS 10.15+ and
Windows 7 and newer. Pocketnet Core should also work on most other Unix-like systems but
is not as frequently tested on them.  It is not recommended to use Pocketnet Core on
unsupported systems.

Notable changes
===============

macOS 64-bit installer
-------------------------

New in 0.22.8 is the macOS 64-bit version of the client.
The minimum requirements are:
* A 64-bit-capable CPU (Intel or Apple silicon);
* macOS 10.15 or later.

General
-------
- QT version upgraded from 5.9.8 to 5.15.14.
- SQL error fixed in `getcomments` RPC method
- GitHub Actions building - build depends from repository
- Cmake build fixes
- Per-Peer Message Capture added (backported from bitcoin/bitcoin#19509)
- Version 3 compact blocks processing improoved. Compact blocks are used for efficient relay of blocks,
  either through High Bandwidth or Low Bandwidth nodes. See BIP 152 for full details (backported from bitcoin/bitcoin#20799)
- Log category added to logs (backported from bitcoin/bitcoin#24464)
- Torv2 removal followups (backported from bitcoin/bitcoin#22179)
- fix: sqlite logging (error description added)
- net: peer address logging

Low-level RPC changes
---------------------

- The `getstakinginfo` RPC method now returns an `stake-time` value from wallet(s) on startup.

0.22.8 change log
=================
Full Changelog: [0.22.7...0.22.8](https://github.com/pocketnetteam/pocketnet.core/compare/0.22.7...0.22.8)

Credits
=======

Thanks to everyone who directly contributed to this release:

- Andy Oknen
- HiHat

As well as to everyone that helped with translations on [Transifex](https://www.transifex.com/pocketnetteam/pocketnet-core/).
