#!/usr/bin/env python3
# Copyright (c) 2023 The Pocketnet developers
# Distributed under the Apache 2.0 software license, see the accompanying
# https://www.apache.org/licenses/LICENSE-2.0
"""
A Badges additional table functional test
Launch this with command from 'test/functional/pocketnet' directory
"""

import sys
import pathlib

sys.path.insert(0, str(pathlib.Path(__file__).parent.parent))

# Avoid wildcard * imports
from test_framework.test_framework import PocketcoinTestFramework

# Pocketnet framework
from framework.helpers import generate_accounts, rollback_node
from framework.models import *


class ModerationJuryTest(PocketcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
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
        node.generatetoaddress(1020, nodeAddress)  # height : 1020

        self.log.info("Node balance: %s", node.public().getaddressinfo(nodeAddress))

        # ---------------------------------------------------------------------------------
        self.log.info("Generate account addresses")
        accounts = generate_accounts(node, nodeAddress, account_num=10)  # height : 1021

        # ---------------------------------------------------------------------------------
        self.log.info("Generate post for set scores")

        # Create account
        fakeAcc = node.public().generateaddress()
        fakeAcc = Account(fakeAcc["address"], fakeAcc["privkey"], "fake")
        node.sendtoaddress(address=fakeAcc.Address, amount=1, destaddress=nodeAddress)
        node.sendtoaddress(address=fakeAcc.Address, amount=1, destaddress=nodeAddress)
        node.stakeblock(1)  # height : 1022
        fakeAccTx = pubGenTx(
            fakeAcc,
            AccountPayload(fakeAcc.Name, "image", "en", "about", "s", "b", "pubkey"),
            1,
            0,
        )
        node.stakeblock(1)  # height : 1023
        fakePostTx = pubGenTx(fakeAcc, ContentPostPayload(), 1, 0)
        node.stakeblock(2)  # height : 1025

        # ---------------------------------------------------------------------------------
        self.log.info("Register accounts")

        for acc in accounts:
            pubGenTx(
                acc,
                AccountPayload(acc.Name, "image", "en", "about", "s", "b", "pubkey"),
                1000,
                0,
            )

        node.stakeblock(20)  # height : 1045

        for acc in accounts:
            assert "shark" in node.public().getuserstate(acc.Address)["badges"]

        self.log.info("Set score (5) to fakePost for check lottery payment")
        for acc in accounts:
            pubGenTx(
                acc,
                ScoreContentPayload(fakePostTx, 5, fakeAcc.Address),
            )

        node.stakeblock(2)  # height : 1047

        bestBlock = node.getblockchaininfo()["bestblockhash"]
        lastCoinstakeTx = node.getblock(bestBlock)["tx"][1]
        assert node.public().gettransactions(lastCoinstakeTx)[0]["vout"][0]["scriptPubKey"]["hex"] == "c0"

        # ---------------------------------------------------------------------------------
        self.log.info("Create comments from all acounts and like all accounts")

        # Create comments from moderators
        for acc in accounts:
            acc.badgeComment = pubGenTx(acc, CommentPayload(fakePostTx))
        node.stakeblock(1)  # height : 1048

        # Like another for set comment liker
        for i in range(len(accounts)):
            accTo = accounts[0 if i == len(accounts) - 1 else i + 1]
            pubGenTx(
                accounts[i],
                ScoreCommentPayload(accTo.badgeComment, 1, accTo.Address),
            )

        # Stake for nearest Badge update period
        # /src/pocketdb/consensus/Base.h:326
        node.stakeblock(52)  # height : 1100

        # Check moderator badges
        for acc in accounts:
            assert "shark" in node.public().getuserstate(acc.Address)["badges"]

        self.log.info("Generate another post for set scores")
        fakePostTx = pubGenTx(fakeAcc, ContentPostPayload(), 1, 0)
        node.stakeblock(1)  # height : 1101

        self.log.info("Set score (4) to fakePost for check lottery payment")
        for acc in accounts:
            pubGenTx(
                acc,
                ScoreContentPayload(fakePostTx, 4, fakeAcc.Address),
            )

        node.stakeblock(2)  # height : 1103

        bestBlock = node.getblockchaininfo()["bestblockhash"]
        lastCoinstakeTx = node.getblock(bestBlock)["tx"][1]
        assert node.public().gettransactions(lastCoinstakeTx)[0]["vout"][0]["scriptPubKey"]["hex"] == "c0"


if __name__ == "__main__":
    ModerationJuryTest().main()
