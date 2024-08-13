0.22.6 Release Notes
====================

Pocketnet Core version 0.22.6 is now available from:

  <https://github.com/pocketnetteam/pocketnet.core/releases/tag/0.22.6>

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

General
------------

- Fixed application crashes due to the lack of exception handling when there is no transaction in the database.
- Fixed an error in the list of "starting" nodes, due to which the node could not find peers.
- Optional arguments for managing the sqlite cache are implemented.
- The list of "starting" nodes in the MAIN network has been expanded.

Barteron
-------

- Fixed the case dependence of the search.


0.22.6 change log
=================
Full Changelog: [0.22.5...0.22.6](https://github.com/pocketnetteam/pocketnet.core/compare/0.22.5...0.22.6)

Credits
=======

Thanks to everyone who directly contributed to this release:

- Andy Oknen
- Network_Support
- HiHat
