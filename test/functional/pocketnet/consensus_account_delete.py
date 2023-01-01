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

        # ------------------------------------------------

        self.log.info("Generate general node address")
        nodeAddress = node.getnewaddress()

        self.log.info("Generate first coinbase 1020 blocks")
        node.generatetoaddress(1020, nodeAddress)

        self.log.info("Node balance: %s", node.public().getaddressinfo(nodeAddress))

        # ------------------------------------------------

        self.log.info("Generate account addresses")
        accounts = []
        for i in range(10):
            acc = node.public().generateaddress()
            accounts.append(Account(acc['address'], acc['privkey']))
            node.sendtoaddress(address=accounts[i].Address, amount=10, destaddress=nodeAddress)

        self.log.info("Stake 15 blocks and check balances")
        node.stakeblock(15)

        self.log.info("Check balance")
        for i in range(10):
            assert(node.public().getaddressinfo(address=accounts[i].Address)['balance'] == 10)

        # ------------------------------------------------

        self.log.info("Register accounts")
        hashes = []
        for i in range(10):
            hashes.append(node.public().generatetransaction(accounts[i], AccountPayload(accounts[i].Address,'image','en','about','s','b','pubkey'), 10))

        self.log.info("Stake 10 block and check profiles")
        node.stakeblock(15)

        for i in range(10):
            assert(node.public().getuserprofile(accounts[i].Address)[0]['hash'] == hashes[i])
        
        # ------------------------------------------------
        self.log.info("Create content from 0 account for check actions with content after delete account")
        node.public().generatetransaction(accounts[0], ContentPostPayload('en','very long message','caption','url',['tag1','tag2'],['image1','image2']))

        # ------------------------------------------------
        self.log.info("Delete 0 account")
        node.public().generatetransaction(accounts[0], AccountDeletePayload())

        self.log.info("Any actions after delete account not allowed")
        test_actions_from_deleted_account(account[0])

        # TODO : actions with content from deleted account (delete tx in mempool)

        # ------------------------------------------------
        self.log.info("Stake 1 block for receive deleted account to blockchain from mempool")
        node.stakeblock(1)

        self.log.info("Again test actions from deleted account")
        test_actions_from_deleted_account(account[0])

        # ------------------------------------------------

        # TODO : actions with content from deleted account (delete tx already in blockchain)




    def test_actions_from_deleted_account(account):
        node = self.nodes[0]

        assert_raises_rpc_error(61, "Failed SocialConsensusHelper::Validate with result 61", node.public().generatetransaction,
            account, AccountPayload(account.Address,'image','en','about','s','b','pubkey'))

        assert_raises_rpc_error(61, "Failed SocialConsensusHelper::Validate with result 61", node.public().generatetransaction,
            account, AccountDeletePayload())

        assert_raises_rpc_error(61, "Failed SocialConsensusHelper::Validate with result 61", node.public().generatetransaction,
            account, AccountSettingPayload(...)) # TODO

        # -----------------------------------------------------------------
        
        assert_raises_rpc_error(61, "Failed SocialConsensusHelper::Validate with result 61", node.public().generatetransaction,
            account, ContentPostPayload('en','very long message','caption','url',['tag1','tag2'],['image1','image2']))

        assert_raises_rpc_error(61, "Failed SocialConsensusHelper::Validate with result 61", node.public().generatetransaction,
            account, ContentPostEditPayload('en','very long message','caption','url',['tag1','tag2'],['image1','image2']))

        assert_raises_rpc_error(61, "Failed SocialConsensusHelper::Validate with result 61", node.public().generatetransaction,
            account, ContentVideoPayload('en','very long message','caption','url',['tag1','tag2'],['image1','image2']))

        assert_raises_rpc_error(61, "Failed SocialConsensusHelper::Validate with result 61", node.public().generatetransaction,
            account, ContentArticlePayload('en','very long message','caption','url',['tag1','tag2'],['image1','image2']))

        assert_raises_rpc_error(61, "Failed SocialConsensusHelper::Validate with result 61", node.public().generatetransaction,
            account, ContentAudioPayload('en','very long message','caption','url',['tag1','tag2'],['image1','image2']))

        assert_raises_rpc_error(61, "Failed SocialConsensusHelper::Validate with result 61", node.public().generatetransaction,
            account, ContentStreamPayload('en','very long message','caption','url',['tag1','tag2'],['image1','image2']))

        assert_raises_rpc_error(61, "Failed SocialConsensusHelper::Validate with result 61", node.public().generatetransaction,
            account, ContentDeletePayload('en','very long message','caption','url',['tag1','tag2'],['image1','image2']))

        assert_raises_rpc_error(61, "Failed SocialConsensusHelper::Validate with result 61", node.public().generatetransaction,
            account, ContentCommentPayload(...)) # TODO

        assert_raises_rpc_error(61, "Failed SocialConsensusHelper::Validate with result 61", node.public().generatetransaction,
            account, ContentCommentDeletePayload(...)) # TODO

        # -----------------------------------------------------------------

        assert_raises_rpc_error(61, "Failed SocialConsensusHelper::Validate with result 61", node.public().generatetransaction,
            account, ActionScorePostPayload(...)) # TODO

        assert_raises_rpc_error(61, "Failed SocialConsensusHelper::Validate with result 61", node.public().generatetransaction,
            account, ActionScoreCommentPayload(...)) # TODO

        assert_raises_rpc_error(61, "Failed SocialConsensusHelper::Validate with result 61", node.public().generatetransaction,
            account, ActionSubscribePayload(...)) # TODO

        assert_raises_rpc_error(61, "Failed SocialConsensusHelper::Validate with result 61", node.public().generatetransaction,
            account, ActionSubscrivePrivatePayload(...)) # TODO

        assert_raises_rpc_error(61, "Failed SocialConsensusHelper::Validate with result 61", node.public().generatetransaction,
            account, ActionUnsubscribePayload(...)) # TODO

        assert_raises_rpc_error(61, "Failed SocialConsensusHelper::Validate with result 61", node.public().generatetransaction,
            account, ActionBlockingPayload(...)) # TODO

        assert_raises_rpc_error(61, "Failed SocialConsensusHelper::Validate with result 61", node.public().generatetransaction,
            account, ActionUnblockingPayload(...)) # TODO

        assert_raises_rpc_error(61, "Failed SocialConsensusHelper::Validate with result 61", node.public().generatetransaction,
            account, ActionBoostPayload(...)) # TODO

    def test_actions_under_deleted_account(account):
        pass

if __name__ == '__main__':
    AccountDeleteTest().main()
