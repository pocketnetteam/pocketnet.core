#!/usr/bin/env python3
# Copyright (c) 2017-2022 The Bitcoin Core developers
# Copyright (c) 2018-2023 The Pocketnet Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""
An Auto Abandon Orphan Stakes functional test
Launch this with command from 'test/functional/pocketnet' directory
"""

import sys
import pathlib

sys.path.insert(0, str(pathlib.Path(__file__).parent.parent))

# Imports should be in PEP8 ordering (std library first, then third party
# libraries then local imports).
from test_framework.test_framework import PocketcoinTestFramework
from test_framework.util import assert_equal

# Pocketnet framework
from framework.helpers import rollback_node, restoreTo
from framework.models import *

from framework.chain_builder import ChainBuilder

DEFAULT_FEED_PARAMS = [0, "", 20, "", [], [], [], [], [], ""]

class AbandonOrphanStakesTest(PocketcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 1
        self.extra_args = [["-debug=consensus", "-debug=wallet"]]

    def run_test(self):
        node = self.nodes[0]
        builder = ChainBuilder(node, self.log)
        builder.build_init(accounts_num=0, moderators_num=0)

        # ---------------------------------------------------------------------------------
        self.log.info("General check stake, invalidate and reconsider")

        # Start balance
        b0 = node.getwalletinfo()['ttl_balance']

        # Generate new stake
        stake_id = node.stakeblock(1)
        test_block_hash = node.getbestblockhash()

        # New stake is not abandoned
        stake_0 = [s for s in node.listtransactions("*", 2000)['txs'] if s['txid'] == stake_id and s['vout'] == 0][0]
        assert_equal(stake_0['abandoned'], False)

        # Balance after new stake
        b1 = node.getwalletinfo()['ttl_balance']
        assert(b1 != b0)

        # Invalidate stake for testing auto abandon
        node.invalidateblock(test_block_hash)

        # Stake is abandoned
        stake_1 = [s for s in node.listtransactions("*", 2000)['txs'] if s['txid'] == stake_id and s['vout'] == 0][0]
        assert_equal(stake_1['abandoned'], True)

        # Balance after invalidation should be the same as before stake
        b2 = node.getwalletinfo()['ttl_balance']
        assert_equal(b2, b0)

        # Reconsider stake for testing auto rollback abandon status and restore balance
        node.reconsiderblock(test_block_hash)

        # Stake is not abandoned
        stake_2 = [s for s in node.listtransactions("*", 2000)['txs'] if s['txid'] == stake_id and s['vout'] == 0][0]
        assert_equal(stake_2['abandoned'], False)

        # Balance after rollback should be the same as after stake
        b3 = node.getwalletinfo()['ttl_balance']
        assert_equal(b3, b1)

if __name__ == "__main__":
    AbandonOrphanStakesTest().main()
