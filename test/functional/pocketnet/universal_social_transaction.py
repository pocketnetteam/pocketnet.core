#!/usr/bin/env python3
# Copyright (c) 2025 The Pocketnet developers
# Distributed under the Apache 2.0 software license, see the accompanying
# https://www.apache.org/licenses/LICENSE-2.0
"""
A Universal Social Transaction functional test
Launch this with command from 'test/functional/pocketnet' directory
"""

import sys
import json
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

import random, string
def randomword(length):
   letters = string.ascii_lowercase
   return ''.join(random.choice(letters) for i in range(length))

class USTTest(PocketcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 1

    def hexType(self, type):
        return '6a0e' + type.encode('utf-8').hex()

    def run_test(self):
        """Main test logic"""
        node = self.nodes[0]
        builder = ChainBuilder(node, self.log)
        pubGenTx = node.public().generatetransaction

        # ---------------------------------------------------------------------------------
        # Prepare chain & accounts
        builder.build_init(accounts_num=3, moderators_num=0)
        builder.register_accounts()
        node.stakeblock(10)

        acc = builder.accounts[0]

        # ---------------------------------------------------------------------------------
        self.log.info("Try send UST with custom identifier")
        _type = "test01"

        ust = UniversalSocialTransactionPayload(
            tx_type=_type.encode('utf-8').hex(),
            s1="some value",
            i1=42
        )

        assert_raises_rpc_error(ConsensusResult.BadTransaction, None, pubGenTx, acc, ust, fee=1)
        assert_raises_rpc_error(ConsensusResult.FailedOpReturn, None, pubGenTx, acc, ust, fee=500)

        # Social transactions must have address in s1
        ust.s1 = acc.Address

        pubGenTx(acc, ust, fee=500)
        node.stakeblock(1)
        
        # node.public().ust_list({
        #     'address': acc.Address,
        #     'types': [ '', '' ]
        # })

        txs = node.public().getfromtotransactions(acc.Address, acc.Address, '', self.hexType(_type))
        assert len(txs) == 1







if __name__ == "__main__":
    USTTest().main()
