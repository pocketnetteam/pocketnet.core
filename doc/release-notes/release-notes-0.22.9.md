0.22.9 Release Notes
====================

Pocketcoin Core version 0.22.9 is now available from:

  <https://github.com/pocketnetteam/pocketnet.core/releases/tag/0.22.9>

This is a new major version release, including new features, various bugfixes
and performance improvements, as well as updated translations.

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
is not as frequently tested on them. It is not recommended to use Pocketnet Core on
unsupported systems.

Notable changes
===============

Consensus
---------
- Enabling the moderation subsystem for Barteron transactions (PIP 109)
- Enabling Boosts for Barteron Offers (PIP 109)
- Allow mixing of content in collections (PIP 109)
- Fix count limit for complain transaction (PIP 109)
- Allow `.` symbol in App ID (PIP 109)
- Include moderation votes from completed juries in winners lottery (PIP 110)

General
-------
- Building fixes (openssl on macOS and GUI cmake build)


Low-level RPC changes
---------------------
- RPC method `getapps` extend next named arguments: `address` for filter by author address and `id` for filter by unique identificator.
- Fix `gettransactions` and `getrawtransaction` in public RPC - include payload data.
- Extend `getalljury` with pagination arguments.


0.22.9 change log
=================
Full Changelog: [0.22.8...0.22.9](https://github.com/pocketnetteam/pocketnet.core/compare/0.22.8...0.22.9)

Credits
=======

Thanks to everyone who directly contributed to this release:

- Andy Oknen

As well as to everyone that helped with translations on [Transifex](https://www.transifex.com/pocketnetteam/pocketnet-core/).