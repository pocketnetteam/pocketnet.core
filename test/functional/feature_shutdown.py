#!/usr/bin/env python3
# Copyright (c) 2018-2019 The Pocketcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test pocketcoind shutdown."""

from test_framework.test_framework import PocketcoinTestFramework
from test_framework.util import assert_equal, get_rpc_proxy
from threading import Thread

class ShutdownTest(PocketcoinTestFramework):

    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 1
        self.supports_cli = False

    def run_test(self):
        node = get_rpc_proxy(self.nodes[0].url, 1, timeout=600, coveragedir=self.nodes[0].coverage_dir)
        # Wait until the server connect init block
        self.wait_until(lambda: node.getblockcount() >= 0)
        # Wait 1 second after requesting shutdown but not before the `stop` call
        # finishes. This is to ensure event loop waits for current connections
        # to close.
        self.stop_node(0, wait=1000)

if __name__ == '__main__':
    ShutdownTest().main()