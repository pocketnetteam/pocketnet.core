#!/usr/bin/env python3
# Copyright (c) 2017-2022 The Bitcoin Core developers
# Copyright (c) 2018-2023 The Pocketnet Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""
An ContentApp functional test
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
from test_framework.util import assert_equal, get_rpc_proxy, rpc_url, assert_raises_rpc_error

# Pocketnet framework
from framework.models import *


class ContentAppTest(PocketcoinTestFramework):
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

        self.log.info("Generate account addresses")
        nAddrs = 2
        accounts = []
        for i in range(nAddrs):
            acc = node.public().generateaddress()
            accounts.append(Account(acc["address"], acc["privkey"]))
            node.sendtoaddress(address=accounts[i].Address, amount=10, destaddress=nodeAddress)
        self.log.info("Account adddresses generated = " + str(len(accounts)))

        node.stakeblock(15)

        self.log.info("Check balance")
        for i in range(nAddrs):
            assert node.public().getaddressinfo(address=accounts[i].Address)["balance"] == 10

        # ---------------------------------------------------------------------------------
        
        self.log.info("Register accounts")
        hashes = []
        for i in range(nAddrs):
            hashes.append(
                pubGenTx(accounts[i], AccountPayload(f"name{i}", "image", "en", "about", "s", "b", "pubkey"), 50)
            )

        node.stakeblock(15)

        for i in range(nAddrs):
            assert node.public().getuserprofile(accounts[i].Address)[0]["hash"] == hashes[i]

        # ---------------------------------------------------------------------------------

        self.log.info("Create first app transaction")
        
        firstApp = ContentAppPayload()
        firstApp.caption = "First app"
        firstApp.message = "First app description"
        firstApp.url = "example.pocketnet.app"

        firstAppTx = pubGenTx(accounts[0], firstApp)

        node.stakeblock(1)

        # ---------------------------------------------------------------------------------


if __name__ == "__main__":
    ContentAppTest().main()
