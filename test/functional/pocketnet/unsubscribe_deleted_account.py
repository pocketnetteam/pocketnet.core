#!/usr/bin/env python3
# Copyright (c) 2017-2022 The Bitcoin Core developers
# Copyright (c) 2018-2023 The Pocketnet Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""
An Unsubscribe/Unblocking deleted account functional test
Launch this with command from 'test/functional/pocketnet' directory
"""

import sys
import pathlib

sys.path.insert(0, str(pathlib.Path(__file__).parent.parent))

# Imports should be in PEP8 ordering (std library first, then third party
# libraries then local imports).

# Avoid wildcard * imports
from test_framework.blocktools import create_block, create_coinbase
from test_framework.messages import CInv, MSG_BLOCK
from test_framework.test_framework import PocketcoinTestFramework
from test_framework.util import assert_equal, get_rpc_proxy, rpc_url, assert_raises_rpc_error

# Pocketnet framework
from framework.helpers import rollback_node, restoreTo
from framework.models import *

from framework.chain_builder import ChainBuilder

class UnsubscribeUnblockingDeletedAccountTest(PocketcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 1
        self.extra_args = [["-debug=consensus"]]

    def run_test(self):
        node = self.nodes[0]
        builder = ChainBuilder(node, self.log)
        builder.build_init(accounts_num=2, moderators_num=1)

        pubGenTx = node.public().generatetransaction
        accounts = builder.accounts
        builder.register_accounts()

        # ---------------------------------------------------------------------------------
        self.log.info("Subscribe to active account")

        pubGenTx(accounts[0], SubscribePayload(accounts[1].Address))
        pubGenTx(accounts[0], BlockingPayload(accounts[1].Address))
        node.stakeblock(1)

        assert node.public().getuserblockings(accounts[0].Address, 1)[0] == accounts[1].Address
        assert node.public().getusersubscribes(accounts[0].Address, 1)[0]['adddress'] == accounts[1].Address

        pubGenTx(accounts[1], AccountDeletePayload())
        node.stakeblock(1)

        pubGenTx(accounts[0], UnsubscribePayload(accounts[1].Address))
        pubGenTx(accounts[0], UnblockingPayload(accounts[1].Address))
        node.stakeblock(1)

        assert len(node.public().getuserblockings(accounts[0].Address, 1)) <= 0
        assert len(node.public().getusersubscribes(accounts[0].Address, 1)) <= 0


if __name__ == "__main__":
    UnsubscribeUnblockingDeletedAccountTest().main()
