#!/usr/bin/env python3

#
# Launch this with command from test/functional directory
# > python3 -i test-shell.py
#

from test_framework.test_shell import TestShell
from pocketnet.framework.models import *

test = TestShell().setup()
node = test.nodes[0]

pubGenTx = node.public().generatetransaction
nodeAddress = node.getnewaddress()
_ = node.generatetoaddress(1020, nodeAddress)
