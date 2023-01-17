#!/usr/bin/env python3
# Copyright (c) 2017-2022 The Bitcoin Core developers
# Copyright (c) 2018-2023 The Pocketnet Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""
An ContentCollections functional test
Launch this with command from 'test/functional/pocketnet' directory
"""

import sys
import pathlib
sys.path.insert(0, str(pathlib.Path(__file__).parent.parent))

# Imports should be in PEP8 ordering (std library first, then third party
# libraries then local imports).
from collections import defaultdict

# Avoid wildcard * imports
from test_framework.blocktools import (create_block, create_coinbase)
from test_framework.messages import CInv, MSG_BLOCK
from test_framework.test_framework import PocketcoinTestFramework
from test_framework.util import (
    assert_equal,
    get_rpc_proxy,
    rpc_url,
    assert_raises_rpc_error
)

# Pocketnet framework
from framework.models import *

class ConteentCollectionsTest(PocketcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        # TODO : запустить несколько нод для проверки синхронизации и спорных транзакций
        self.num_nodes = 1
        self.extra_args = [["-debug=consensus"]]

    def run_test(self):
        """Main test logic"""
    node = self.nodes[0]
    pubGenTx = node.public().generatetransaction

    # ---------------------------------------------------------------------------------

    self.log.info("Generate general node address")
    nodeAddress = node.getnewaddress()

    self.log.info("Generate first coinbase 1020 blocks")
    node.generatetoaddress(1020, nodeAddress)

    self.log.info("Node balance: %s", node.public().getaddressinfo(nodeAddress))

    # ---------------------------------------------------------------------------------

    node.stakeblock(1)

if __name__ == '__main__':
    ConteentCollectionsTest().main()