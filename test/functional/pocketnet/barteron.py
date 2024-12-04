#!/usr/bin/env python3
# Copyright (c) 2023 The Pocketnet developers
# Distributed under the Apache 2.0 software license, see the accompanying
# https://www.apache.org/licenses/LICENSE-2.0
"""
A Barteron functional test
Launch this with command from 'test/functional/pocketnet' directory
"""

import sys
import json
import pathlib

sys.path.insert(0, str(pathlib.Path(__file__).parent.parent))

# Imports should be in PEP8 ordering (std library first, then third party
# libraries then local imports).
from collections import defaultdict

# Avoid wildcard * imports
from test_framework.blocktools import create_block, create_coinbase
from test_framework.messages import CInv, MSG_BLOCK
from test_framework.test_framework import PocketcoinTestFramework
from test_framework.util import (
    assert_equal,
    get_rpc_proxy,
    rpc_url,
    assert_raises_rpc_error,
)

# Pocketnet framework
from framework.chain_builder import ChainBuilder
from framework.helpers import rollback_node
from framework.models import *

import random, string
def randomword(length):
   letters = string.ascii_lowercase
   return ''.join(random.choice(letters) for i in range(length))

class BarteronTest(PocketcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 1
        self.extra_args = [["-debug=consensus","-api=1"]]

    def run_test(self):
        """Main test logic"""
        node = self.nodes[0]
        builder = ChainBuilder(node, self.log)
        pubGenTx = node.public().generatetransaction

        # ---------------------------------------------------------------------------------
        # Prepare chain & accounts
        builder.build_init(accounts_num=3, moderators_num=1)
        node.stakeblock(10)

        # ---------------------------------------------------------------------------------
        self.log.info("Try register Barteron account without Bastyon account")

        bartAccount = BartAccountPayload()
        bartAccount.s1 = builder.accounts[0].Address
        assert_raises_rpc_error(ConsensusResult.NotRegistered, None, pubGenTx, builder.accounts[0], bartAccount)

        # ---------------------------------------------------------------------------------
        # Register accounts
        builder.register_accounts()

        # ---------------------------------------------------------------------------------
        self.log.info("Register Barteron accounts")

        brtAccounts = []
        for i, account in enumerate(builder.accounts):
            bartAccount = BartAccountPayload()
            bartAccount.s1 = account.Address
            bartAccount.p = Payload()
            bartAccount.p.s4 = json.dumps({ "a": [ random.randint(0, 100) ], "test": "HOI" })
            brtAccounts.append({ "tx": pubGenTx(account, bartAccount), "acc": account })
        node.stakeblock(5)

        for i, account in enumerate(builder.accounts):
            assert json.loads(node.public().getbarteronaccounts([account.Address])[0]['p']['s4'])['test'] == "HOI"
        
        # ---------------------------------------------------------------------------------
        self.log.info("Check accounts edit")
        _bartAccount = BartAccountPayload()
        _bartAccount.s1 = builder.accounts[0].Address
        _bartAccount.p = Payload()
        _bartAccount.p.s4 = json.dumps({ "a": [ random.randint(0, 100) ], "test": "HOI" })
        pubGenTx(builder.accounts[0], _bartAccount)
        node.stakeblock(1)
        
        # ---------------------------------------------------------------------------------
        self.log.info("Register Barteron offers")

        brtOffers = []
        lang = ['en','ru','gb']
        for i, account in enumerate(builder.accounts):
            for ii in range(10):
                bartOffer = BartOfferPayload()
                bartOffer.s1 = account.Address
                bartOffer.p = Payload()
                bartOffer.p.s1 = lang[random.randint(0, 2)]
                bartOffer.p.s2 = f'Custom caption with random ({random.randint(0, 100)}) number'
                bartOffer.p.s3 = f'Custom description with random ({random.randint(0, 100)}) number'
                bartOffer.p.s5 = ['http://image.url.1','http://image.url.2']
                bartOffer.p.s6 = randomword(random.randint(0, 10))
                bartOffer.p.i1 = random.randint(0, 1000)
                bartOffer.p.s4 = json.dumps({ "t": random.randint(0, 100), "a": [ random.randint(0, 100), random.randint(0, 100), random.randint(0, 100) ], "test": "HOI" })
                brtOffers.append({ "tx": pubGenTx(account, bartOffer), "acc": account })
                node.stakeblock(1)
        
        for i, account in enumerate(builder.accounts):
            assert json.loads(node.public().getbarteronoffersbyaddress(account.Address)[0]['p']['s4'])['test'] == "HOI"

        # ---------------------------------------------------------------------------------
        self.log.info("Moderation checks")

        assert_raises_rpc_error(ConsensusResult.SelfFlag, None, pubGenTx, builder.accounts[0], ModFlagPayload(brtAccounts[0]["tx"], brtAccounts[0]["acc"].Address))
        pubGenTx(builder.accounts[1], ModFlagPayload(brtAccounts[0]["tx"], brtAccounts[0]["acc"].Address))

        assert_raises_rpc_error(ConsensusResult.SelfFlag, None, pubGenTx, builder.accounts[0], ModFlagPayload(brtOffers[0]["tx"], brtOffers[0]["acc"].Address))
        assert_raises_rpc_error(ConsensusResult.Duplicate, None, pubGenTx, builder.accounts[1], ModFlagPayload(brtOffers[0]["tx"], brtOffers[0]["acc"].Address))
        pubGenTx(builder.accounts[2], ModFlagPayload(brtOffers[0]["tx"], brtOffers[0]["acc"].Address))

        # ---------------------------------------------------------------------------------
        self.log.info("Check offers feed")

        feed = node.public().getbarteronfeed({})
        # TODO - check?

        # ---------------------------------------------------------------------------------
        # todo - find deals
        self.log.info("Check offers deals")
        for i, offer in enumerate(feed):
            pass
        

if __name__ == "__main__":
    BarteronTest().main()
