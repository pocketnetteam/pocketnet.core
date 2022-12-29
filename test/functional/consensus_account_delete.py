#!/usr/bin/env python3
# Copyright (c) 2017-2022 The Bitcoin Core developers
# Copyright (c) 2018-2023 The Pocketnet Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""An AccountDelete functional test"""

from test_framework.test_framework import PocketcoinTestFramework
from test_framework.util import assert_equal, get_rpc_proxy
from threading import Thread


class AccountDeleteTest(PocketcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 1
        self.extra_args = [[], ["-debug=consensus"], []]

    def run_test(self):
        """Main test logic"""

        # Generating a block on one of the nodes will get us out of IBD
        blocks = [int(self.nodes[0].generate(nblocks=1020)[0], 16)]

        self.tip = int(self.nodes[0].getbestblockhash(), 16)
        self.block_time = self.nodes[0].getblock(self.nodes[0].getbestblockhash())['time'] + 1

        height = self.nodes[0].getblockcount()


if __name__ == '__main__':
    AccountDeleteTest().main()
