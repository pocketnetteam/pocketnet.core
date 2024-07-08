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
        
        app = AppPayload()
        app.s1 = accounts[0].Address
        app.p = Payload()
        app.p.s1 = "{\"n\":\"First app\",\"d\":\"First app description\",\"t\":[\"tag1\",\"tag2\"]}"
        app.p.s2 = "first_app"

        appTx0 = pubGenTx(accounts[0], app)

        # Double to mempool from one address
        assert_raises_rpc_error(ConsensusResult.ContentLimit, None, pubGenTx, accounts[0], app)

        # Double to mempool from another address with same payload
        assert_raises_rpc_error(ConsensusResult.FailedOpReturn, None, pubGenTx, accounts[1], app)

        node.stakeblock(1)

        # Another app with same id
        app.s1 = accounts[1].Address
        assert_raises_rpc_error(ConsensusResult.NicknameDouble, None, pubGenTx, accounts[1], app)

        # Double to blockchain from one address
        app.s1 = accounts[0].Address
        assert_raises_rpc_error(ConsensusResult.NicknameDouble, None, pubGenTx, accounts[0], app)

        # Double to blockchain from another address with same payload
        app.s1 = accounts[0].Address
        assert_raises_rpc_error(ConsensusResult.FailedOpReturn, None, pubGenTx, accounts[1], app)

        # Double to blockchain from another address with different payload
        app.s1 = accounts[1].Address
        assert_raises_rpc_error(ConsensusResult.NicknameDouble, None, pubGenTx, accounts[1], app)

        node.stakeblock(1)

        # ---------------------------------------------------------------------------------

        self.log.info("Check APIs")

        # Scores
        assert len(node.public().getapps({})) == 1
        assert_raises_rpc_error(ConsensusResult.SelfScore, None, pubGenTx, accounts[0], ScoreContentPayload(appTx0, 5, accounts[0].Address))

        scTx0 = pubGenTx(accounts[1], ScoreContentPayload(appTx0, 5, accounts[0].Address))
        assert_raises_rpc_error(ConsensusResult.DoubleScore, None, pubGenTx, accounts[1], ScoreContentPayload(appTx0, 5, accounts[0].Address))

        node.stakeblock(1)

        # Comments
        cmtTx0 = pubGenTx(accounts[0], CommentPayload(appTx0))
        cmtTx1 = pubGenTx(accounts[1], CommentPayload(appTx0))
        
        node.stakeblock(1)

        # Check rating and additional apis (scores, comments)

        assert node.public().getapps({})[0]['ad']['r'] == 2
        assert len(node.public().getappscores({"app": appTx0})) == 1
        assert len(node.public().getappcomments({"app": appTx0})) == 2



if __name__ == "__main__":
    ContentAppTest().main()
