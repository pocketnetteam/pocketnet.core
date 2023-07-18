#!/usr/bin/env python3
# Copyright (c) 2023 The Pocketnet developers
# Distributed under the Apache 2.0 software license, see the accompanying
# https://www.apache.org/licenses/LICENSE-2.0
"""
A Badges additional table functional test
Launch this with command from 'test/functional/pocketnet' directory
"""

import sys
import pathlib

sys.path.insert(0, str(pathlib.Path(__file__).parent.parent))

# Avoid wildcard * imports
from test_framework.test_framework import PocketcoinTestFramework

# Pocketnet framework
from framework.helpers import generate_accounts, rollback_node
from framework.models import *


class ModerationJuryTest(PocketcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
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
        node.generatetoaddress(1020, nodeAddress)  # height : 1020

        nodeBalance = node.public().getaddressinfo(nodeAddress)['balance']

        # ---------------------------------------------------------------------------------
        self.log.info("Generate account addresses")
        accounts = generate_accounts(node, nodeAddress, account_num=10)  # height : 1021

        # ---------------------------------------------------------------------------------
        self.log.info("Send money to accounts and check balances")

        for acc in accounts:
            node.sendtoaddress(address=acc.Address, amount=10, destaddress=nodeAddress)
        
        node.stakeblock(1)

        assert node.public().getaddressinfo(nodeAddress) == ...

        # TODO (aok) : implement after explorer api ready


        rollback_node(node, 1, self.log)  # height : 1149


if __name__ == "__main__":
    ModerationJuryTest().main()
