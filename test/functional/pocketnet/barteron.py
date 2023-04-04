#!/usr/bin/env python3
# Copyright (c) 2023 The Pocketnet developers
# Distributed under the Apache 2.0 software license, see the accompanying
# https://www.apache.org/licenses/LICENSE-2.0
"""
A Barteron functional test
Launch this with command from 'test/functional/pocketnet' directory
"""

import sys
import pathlib

sys.path.insert(0, str(pathlib.Path(__file__).parent.parent))

# Imports should be in PEP8 ordering (std library first, then third party
# libraries then local imports).
from collections import defaultdict

# Avoid wildcard * imports
from test_framework.blocktools import create_block, create_coinbase
from test_framework.messages import CInv, MSG_BLOCK
from test_framework.test_framework import PocketcoinTestFramework
from test_framework.util import (
    assert_equal,
    get_rpc_proxy,
    rpc_url,
    assert_raises_rpc_error,
)

# Pocketnet framework
from framework.chain_builder import ChainBuilder
from framework.helpers import rollback_node
from framework.models import *


class BarteronTest(PocketcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 1
        self.extra_args = [["-debug=consensus","-api=1"]]

    def run_test(self):
        """Main test logic"""
        node = self.nodes[0]
        builder = ChainBuilder(node, self.log)
        pubGenTx = node.public().generatetransaction

        # ---------------------------------------------------------------------------------
        # Prepare chain & accounts
        builder.build_init()
        builder.register_accounts()

        # ---------------------------------------------------------------------------------
        self.log.info("Register Barteron account")

        bartAccount = BartAccountPayload()
        bartAccount.s1 = builder.accounts[0].Address
        bartAccount.p = Payload()
        bartAccount.p.s4 = [1,2,3,4,5]
        bartAccount.p.s5 = [1,2]
        pubGenTx(builder.accounts[0], bartAccount)
        node.stakeblock(1)
        
        # ---------------------------------------------------------------------------------
        self.log.info("Register Barteron account with incorrect lists")

        bartAccount = BartAccountPayload()
        bartAccount.s1 = builder.accounts[0].Address
        bartAccount.p = Payload()
        bartAccount.p.s4 = 'Incorrect string - not json list'
        bartAccount.p.s5 = 'Incorrect string - not json list'
        pubGenTx(builder.accounts[0], bartAccount)
        node.stakeblock(1)
        # ---------------------------------------------------------------------------------
        

if __name__ == "__main__":
    BarteronTest().main()
