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

# Imports should be in PEP8 ordering (std library first, then third party
# libraries then local imports).
from collections import defaultdict

# Avoid wildcard * imports
from test_framework.blocktools import (create_block, create_coinbase)
from test_framework.messages import CInv, MSG_BLOCK
from test_framework.test_framework import PocketcoinTestFramework
from test_framework.util import (
    assert_equal,
    get_rpc_proxy,
    rpc_url,
    assert_raises_rpc_error
)

# Pocketnet framework
from framework.helpers import rollback_node
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
        node.generatetoaddress(1020, nodeAddress) # height : 1020

        self.log.info("Node balance: %s", node.public().getaddressinfo(nodeAddress))

        # ---------------------------------------------------------------------------------
        self.log.info("Generate account addresses")

        accounts = []
        for i in range(10):
            acc = node.public().generateaddress()
            accounts.append(Account(acc['address'], acc['privkey'], f'user{i}'))
            node.sendtoaddress(address=accounts[i].Address, amount=10, destaddress=nodeAddress)

        node.stakeblock(1) # height : 1021

        # ---------------------------------------------------------------------------------
        self.log.info("Generate post for set badges")

        # Create account
        fakeAcc = node.public().generateaddress()
        fakeAcc = Account(fakeAcc['address'], fakeAcc['privkey'], 'fake')
        node.sendtoaddress(address=fakeAcc.Address, amount=1, destaddress=nodeAddress)
        node.sendtoaddress(address=fakeAcc.Address, amount=1, destaddress=nodeAddress)
        node.stakeblock(1) # height : 1022
        fakeAccTx = pubGenTx(fakeAcc, AccountPayload(fakeAcc.Name,'image','en','about','s','b','pubkey'), 1, 0)
        node.stakeblock(1) # height : 1023
        fakePostTx = pubGenTx(fakeAcc, ContentPostPayload(), 1, 0)
        node.stakeblock(2) # height : 1025

        # ---------------------------------------------------------------------------------
        self.log.info("Register accounts")
        
        for acc in accounts:
            pubGenTx(acc, AccountPayload(acc.Name,'image','en','about','s','b','pubkey'), 1000, 0)

        node.stakeblock(20) # height : 1045

        for acc in accounts:
            assert('moderator' not in node.public().getuserstate(acc.Address)['badges'])

        # ---------------------------------------------------------------------------------
        self.log.info("Create comments from all acounts and like 1/2 accounts only")

        # Create comments from moderators
        for acc in accounts:
            acc.badgeComment = pubGenTx(acc, CommentPayload(fakePostTx))
        node.stakeblock(1) # height : 1046

        # Like another for set comment liker
        for i in range(len(accounts)):
            accTo = accounts[0 if i == len(accounts)-1 else i+1]
            accTo.moderator = False
            if i < len(accounts)/2:
                accTo.moderator = True
                pubGenTx(accounts[i], ScoreCommentPayload(accTo.badgeComment, 1, accTo.Address))
        
        # Stake for nearest Badge update period
        # /src/pocketdb/consensus/Base.h:326
        node.stakeblock(4) # height : 1050

        # Check moderator badges
        for acc in accounts:
            if acc.moderator:
                assert('moderator' in node.public().getuserstate(acc.Address)['badges'])
            else:
                assert('moderator' not in node.public().getuserstate(acc.Address)['badges'])

        # Rollback 1 block for remove all badges
        rollback_node(node, 1, self.log)  # height : 1049
        for acc in accounts:
            assert('moderator' not in node.public().getuserstate(acc.Address)['badges'])

        node.stakeblock(1) # height : 1050

        # ---------------------------------------------------------------------------------
        self.log.info("Stake chain to 1150 height for remove all badges")
        # /src/pocketdb/consensus/Base.h:326

        node.stakeblock(100); # height : 1150

        for acc in accounts:
            assert('moderator' not in node.public().getuserstate(acc.Address)['badges'])

        # ---------------------------------------------------------------------------------
        self.log.info("Rollback chain to 1149 height for restore 1/2 badges")

        rollback_node(node, 1, self.log)  # height : 1149

        # Recheck restored badges
        for acc in accounts:
            if acc.moderator:
                assert('moderator' in node.public().getuserstate(acc.Address)['badges'])
            else:
                assert('moderator' not in node.public().getuserstate(acc.Address)['badges'])


if __name__ == '__main__':
    ModerationJuryTest().main()
