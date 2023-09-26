#!/usr/bin/env python3
# Copyright (c) 2017-2022 The Bitcoin Core developers
# Copyright (c) 2018-2023 The Pocketnet Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""
An ActionBlocking functional test
Launch this with command from 'test/functional/pocketnet' directory
"""

import sys
import pathlib

sys.path.insert(0, str(pathlib.Path(__file__).parent.parent))

# Imports should be in PEP8 ordering (std library first, then third party
# libraries then local imports).

# Avoid wildcard * imports
from test_framework.blocktools import create_block, create_coinbase
from test_framework.messages import CInv, MSG_BLOCK
from test_framework.test_framework import PocketcoinTestFramework
from test_framework.util import assert_equal, get_rpc_proxy, rpc_url, assert_raises_rpc_error

# Pocketnet framework
from framework.helpers import rollback_node, restoreTo
from framework.models import *


class ActionBlockingTest(PocketcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        # TODO : запустить несколько нод для проверки синхронизации и спорных транзакций
        self.num_nodes = 1
        self.extra_args = [["-debug=consensus"]]

    def run_test(self):
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
        self.log.info("Test 1 - Blocking creation")

        self.log.info("Check - Simple blocking")
        accounts[0].blockings.append(pubGenTx(accounts[0], BlockingPayload(accounts[1].Address)))

        self.log.info("Check - Do not allow double blocking in mempool")
        assert_raises_rpc_error(
            28,
            None,
            pubGenTx,
            accounts[0],
            BlockingPayload(accounts[1].Address),
        )
        node.stakeblock(1)

        self.log.info("Check - Do not allow double blocking")
        assert_raises_rpc_error(
            23,
            None,
            pubGenTx,
            accounts[0],
            BlockingPayload(accounts[1].Address),
        )
        node.stakeblock(1)

        self.log.info("Check - Simple unblocking")
        pubGenTx(accounts[0], UnblockingPayload(accounts[1].Address))
        accounts[0].blockings.pop(0)
        node.stakeblock(1)

        # (0) 
        # self.log.info("Check - Blocking after unblocking")
        # accounts[0].blockings.append(pubGenTx(accounts[0], BlockingPayload(accounts[1].Address)))
        node.stakeblock(1) # height > 1054

        assert len(node.public().getuserblockings(accounts[0].Address)) == 0

        # ---------------------------------------------------------------------------------
        # проверка что после отката мультиблокс таблица заполнена правило при наличии 2х и более транзакций
        # бана/разбана за период отката

        # (1)
        self.log.info("x -> y false")
        accounts[0].blockings.append(pubGenTx(accounts[0], BlockingPayload(accounts[1].Address)))
        node.stakeblock(1) # height > 1055

        # (2)
        self.log.info("x -> y true")
        accounts[0].blockings.append(pubGenTx(accounts[0], UnblockingPayload(accounts[1].Address)))
        node.stakeblock(1) # height > 1056

        assert len(node.public().getuserblockings(accounts[0].Address)) == 0

        # (2) -> (1) -> (0)
        h = restoreTo(node, 1054, self.log) # height > 1054
        assert 1054 == h
        
        assert len(node.public().getuserblockings(accounts[0].Address)) == 0

        # ---------------------------------------------------------------------------------

        # proveryaem korrektnost otkata esli uzer bil zaregistrirovan za period otkata
        



if __name__ == "__main__":
    ActionBlockingTest().main()
