#!/usr/bin/env python3
# Copyright (c) 2023 The Pocketnet developers
# Distributed under the Apache 2.0 software license, see the accompanying
# https://www.apache.org/licenses/LICENSE-2.0
"""
This test checks the system node calls.
Launch this from 'test/functional/pocketnet' directory or via 
test_runner.py
"""

import sys
import pathlib

sys.path.insert(0, str(pathlib.Path(__file__).parent.parent))

from framework.chain_builder import ChainBuilder

from test_framework.test_framework import PocketcoinTestFramework


class SystemTest(PocketcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 2
        self.extra_args = [["-debug=consensus"], ["-debug=consensus"]]

    def test_getpeerinfo(self):
        self.log.info("Test 1 - Getting peer info")

        node0_result = self.nodes[0].public().getpeerinfo()
        node1_result = self.nodes[1].public().getpeerinfo()

        assert isinstance(node0_result, list)
        assert isinstance(node1_result, list)

        node0_info = node0_result[0]
        node1_info = node1_result[0]

        assert node0_info["inbound"] == True
        assert node1_info["inbound"] == False
        assert node0_info["lastrecv"] == node1_info["lastrecv"]
        assert node0_info["lastsend"] == node1_info["lastsend"]
        assert node0_info["services"] == node1_info["services"]
        assert node0_info["synced_blocks"] == -1
        assert node0_info["synced_headers"] == -1
        assert node1_info["synced_blocks"] > 1000
        assert node1_info["synced_headers"] > 1000

    def test_getnodeinfo(self):
        self.log.info("Test 2 - Getting node info")

        node0_info = self.nodes[0].public().getnodeinfo()
        node1_info = self.nodes[1].public().getnodeinfo()

        assert isinstance(node0_info, dict)
        assert isinstance(node1_info, dict)

        assert node0_info["chain"] == node1_info["chain"]
        assert node0_info["lastblock"] == node1_info["lastblock"]
        assert node0_info["ports"] != node1_info["ports"]
        assert node0_info["version"] == node1_info["version"]

        for port in ["api", "http", "https", "node", "rest", "wss"]:
            assert port in node0_info["ports"]
            assert port in node1_info["ports"]

    def test_gettime(self):
        self.log.info("Test 3 - Getting node time")

        node0_time = self.nodes[0].public().gettime()
        node1_time = self.nodes[1].public().gettime()

        assert isinstance(node0_time, dict)
        assert isinstance(node1_time, dict)
        assert node0_time["time"] == node1_time["time"]

    def test_getcoininfo(self):
        self.log.info("Test 4 - Getting coin info")

        node0_result = self.nodes[0].public().getcoininfo()
        node1_result = self.nodes[1].public().getcoininfo()

        assert isinstance(node0_result, dict)
        assert isinstance(node1_result, dict)

        assert node0_result["emission"] == node1_result["emission"]
        assert node0_result["height"] == node1_result["height"]

    def test_sync_nodes(self):
        self.log.info("Check - sync nodes")
        self.connect_nodes(1, 2)
        self.sync_all()

    def run_test(self):
        node = self.nodes[0]
        builder = ChainBuilder(node, self.log)
        builder.build_init(accounts_num=5, moderators_num=1)
        builder.register_accounts()

        self.test_sync_nodes()
        self.test_getpeerinfo()
        self.test_getnodeinfo()
        self.test_gettime()
        self.test_getcoininfo()


if __name__ == "__main__":
    SystemTest().main()
