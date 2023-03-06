#!/usr/bin/env python3
# Copyright (c) 2023 The Pocketnet developers
# Distributed under the Apache 2.0 software license, see the accompanying
# https://www.apache.org/licenses/LICENSE-2.0
"""
This test uses chain_builder in order to generate some
activity on the chain.
Launch this from 'test/functional/pocketnet' directory
"""

import sys
import pathlib

sys.path.insert(0, str(pathlib.Path(__file__).parent.parent))

from framework.chain_builder import ChainBuilder

from test_framework.test_framework import PocketcoinTestFramework


class ChainBuilderFullActivityTest(PocketcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 1
        self.extra_args = [["-debug=consensus"]]

    def test_check_historical_feed(self, node):
        feed = node.public().gethistoricalfeed()
        contents = feed["contents"]
        assert len(contents) > 0
        self.log.info(f"Number of posts in historical feed: {len(contents)}")

    def run_test(self):
        node = self.nodes[0]
        builder = ChainBuilder(node, self.log)
        builder.build_full()

        info = node.public().getaddressinfo(builder.node_address)
        self.log.info(f"Node balance: {info}")

        self.test_check_historical_feed(node)


if __name__ == "__main__":
    ChainBuilderFullActivityTest().main()
