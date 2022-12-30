#!/usr/bin/env python3
# Copyright (c) 2017-2022 The Bitcoin Core developers
# Copyright (c) 2018-2023 The Pocketnet Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""
An AccountDelete functional test
Launch this with command from 'test/functional/pocketnet' directory
"""

# Import from root directory
import sys
sys.path.insert(0, '../')

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
from pocketnet.framework.models import *


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
            accounts.append(node.public().generateaddress())
            node.sendtoaddress(address=accounts[i]['address'], amount=10, destaddress=nodeAddress)

        self.log.info("Stake 15 blocks and check balances")
        node.stakeblock(15)

        self.log.info("Check balance")
        for i in range(10):
            assert(node.public().getaddressinfo(address=accounts[i]['address'])['balance'] == 10)

        # ------------------------------------------------

        self.log.info("Register accounts")
        for i in range(10):
            acc = Account(f"test{i}",'image','en','about','s','b','pubkey')
            accounts[i]['txHash'] = node.public().generatetransaction(
                address=accounts[i]['address'],
                privkeys=[ accounts[i]['privkey'] ],
                outcount=10,
                type=acc.TxType,
                payload=acc.Serialize(),
                confirmations=10
            )

        self.log.info("Stake 10 block and check profiles")
        node.stakeblock(15)

        for i in range(10):
            assert(node.public().getuserprofile(accounts[i]['address'])[0]['hash'] == accounts[i]['txHash'])
        
        # ------------------------------------------------
        self.log.info("Create content from 0 account for check actions with content after delete account")
        post = ContentPost('en','very long message','caption','url',['tag1','tag2'],['image1','image2'])
        node.public().generatetransaction(
            address=accounts[0]['address'],
            privkeys=[ accounts[0]['privkey'] ],
            outcount=1,
            type=post.TxType,
            payload=post.Serialize(),
            confirmations=10
        )

        # ------------------------------------------------
        self.log.info("Delete 0 account")
        accDel = AccountDelete()
        node.public().generatetransaction(
            address=accounts[0]['address'],
            privkeys=[ accounts[0]['privkey'] ],
            outcount=1,
            type=accDel.TxType,
            payload=accDel.Serialize(),
            confirmations=10
        )

        # Change account info after delete account not allowed
        acc = Account(f"test{0}",'image','en','about','s','b','pubkey')
        assert_raises_rpc_error(
            61, "Failed SocialConsensusHelper::Validate with result 61",
            node.public().generatetransaction,
            address=accounts[0]['address'],
            privkeys=[ accounts[0]['privkey'] ],
            outcount=10,
            type=acc.TxType,
            payload=acc.Serialize(),
            confirmations=10
        )

        # TODO : любые другие транзакции не должны допускаться от удаленного аккаунта
        # ------------------------------------------------

        # TODO : лайки комментарии бусты репосты донаты не должны проходить для удаленного аккаунта - проверить с мемпулом


if __name__ == '__main__':
    AccountDeleteTest().main()
