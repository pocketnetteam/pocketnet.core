#!/usr/bin/env python3
# Copyright (c) 2017-2022 The Bitcoin Core developers
# Copyright (c) 2018-2023 The Pocketnet Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""
An AccountDelete functional test
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
from framework.models import *


class AccountDeleteTest(PocketcoinTestFramework):
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

        accounts = []
        for i in range(10):
            acc = node.public().generateaddress()
            accounts.append(Account(acc['address'], acc['privkey'], f'user{i}'))
            node.sendtoaddress(address=accounts[i].Address, amount=1, destaddress=nodeAddress)

        node.stakeblock(1)

        # ---------------------------------------------------------------------------------
        self.log.info("Generate moderator addresses")

        moders = []
        for i in range(10):
            acc = node.public().generateaddress()
            moders.append(Account(acc['address'], acc['privkey'], f'moderator{i}'))
            node.sendtoaddress(address=moders[i].Address, amount=1, destaddress=nodeAddress)

        node.stakeblock(1)

        # ---------------------------------------------------------------------------------
        self.log.info("Generate post for set moderator badges")

        # Create account
        fakeAcc = node.public().generateaddress()
        fakeAcc = Account(fakeAcc['address'], fakeAcc['privkey'], 'fake')
        node.sendtoaddress(address=fakeAcc.Address, amount=1, destaddress=nodeAddress)
        node.sendtoaddress(address=fakeAcc.Address, amount=1, destaddress=nodeAddress)
        node.stakeblock(1)
        fakeAccTx = pubGenTx(fakeAcc, AccountPayload(fakeAcc.Name,'image','en','about','s','b','pubkey'), 1, 0)
        node.stakeblock(1)
        fakePostTx = pubGenTx(fakeAcc, ContentPostPayload(), 1, 0)
        node.stakeblock(1)

        # ---------------------------------------------------------------------------------
        self.log.info("Register accounts")
        
        for acc in accounts:
            pubGenTx(acc, AccountPayload(acc.Name,'image','en','about','s','b','pubkey'), 1000, 0)

        for acc in moders:
            pubGenTx(acc, AccountPayload(acc.Name,'image','en','about','s','b','pubkey'), 1000, 0)

        node.stakeblock(10)

        # ---------------------------------------------------------------------------------
        self.log.info("Prepare moderators badge")

        # Create comments from moderators
        for acc in moders:
            acc.badgeComment = pubGenTx(acc, CommentPayload(fakePostTx))
        node.stakeblock(1)

        # Like another for set comment liker
        for i in range(len(moders)):
            accTo = moders[0 if i == len(moders)-1 else i+1]
            pubGenTx(moders[i], ScoreCommentPayload(accTo.badgeComment, 1, accTo.Address))
        node.stakeblock(10)

        # Check moderator badges
        for acc in moders:
            assert('moderator' in node.public().getuserstate(acc.Address)['badges'])

        # ---------------------------------------------------------------------------------
        self.log.info("Test 1 - вердикт положительный")

        # ---------------------------------------------------------------------------------
        self.log.info("Test 1 - вердикт отрицательный")

        # ---------------------------------------------------------------------------------
        self.log.info("Test 1 - не назначенный модератор не имеет права голоса")

        # ---------------------------------------------------------------------------------
        self.log.info("Test 1 - не модератор не имеет права голоса")
        
        

if __name__ == '__main__':
    AccountDeleteTest().main()
