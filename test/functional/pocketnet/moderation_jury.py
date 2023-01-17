#!/usr/bin/env python3
# Copyright (c) 2017-2022 The Bitcoin Core developers
# Copyright (c) 2018-2023 The Pocketnet Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""
An Moderation Jury functional test
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


class ModerationJuryTest(PocketcoinTestFramework):
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
            node.sendtoaddress(address=accounts[i].Address, amount=10, destaddress=nodeAddress)

        node.stakeblock(1)

        # ---------------------------------------------------------------------------------
        self.log.info("Generate moderator addresses")

        moders = []
        for i in range(10):
            acc = node.public().generateaddress()
            moders.append(Account(acc['address'], acc['privkey'], f'moderator{i}'))
            node.sendtoaddress(address=moders[i].Address, amount=10, destaddress=nodeAddress)

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

        node.stakeblock(20)

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
        node.stakeblock(5)

        # Check moderator badges
        for acc in moders:
            assert('moderator' in node.public().getuserstate(acc.Address)['badges'])

        # ---------------------------------------------------------------------------------
        self.log.info("Test 1 - flags not allowed & jury did not created")

        # Post for flags
        jury1 = {
            'post': pubGenTx(accounts[0], ContentPostPayload()),
            'account': accounts[0]
        }

        # Content in mempool not accepted flags
        assert_raises_rpc_error(12, None, pubGenTx, moders[0], ModFlagPayload(jury1['post'], jury1['account'].Address, 1))
        node.stakeblock(1)

        # We need 2 flags in 10 blocks for create jury
        # /src/pocketdb/consensus/Base.h:674 moderation_jury_flag_count
        # /src/pocketdb/consensus/Base.h:679 moderation_jury_flag_depth

        lastFlagTx = pubGenTx(moders[0], ModFlagPayload(jury1['post'], jury1['account'].Address, 1))
        node.stakeblock(10)
        lastFlagTx = pubGenTx(moders[1], ModFlagPayload(jury1['post'], jury1['account'].Address, 1))
        node.stakeblock(1)
        assert('id' not in node.public().getjury(lastFlagTx))
        assert(len(node.public().getjurymoderators(lastFlagTx)) == 0)

        # ---------------------------------------------------------------------------------
        self.log.info("Test 2 - jury created and moderators assigned")

        lastFlagTx = pubGenTx(moders[3], ModFlagPayload(jury1['post'], jury1['account'].Address, 1))
        node.stakeblock(1)
        jury1['data'] = node.public().getjury(lastFlagTx)
        assert('id' in jury1['data'] and jury1['data']['id'] == lastFlagTx)
        jury1['mods'] = node.public().getjurymoderators(lastFlagTx)
        assert(len(jury1['mods']) == 4)

        # ---------------------------------------------------------------------------------
        self.log.info("Test 3 - not assigned moderator dont allowed vote")

        assigned = []
        notAssigned = []
        for mod in moders:
            if mod.Address not in jury1['mods']:
                notAssigned.append(mod)
            else:
                assigned.append(mod)

        assert(len(assigned) == len(jury1['mods']))

        # After create jury votes allowed after delay in 10 blocks
        # /src/pocketdb/consensus/moderation/Vote.hpp:51
        for mod in notAssigned:
            assert_raises_rpc_error(44, None, pubGenTx, mod, ModVotePayload(jury1['data']['id'], 1))

        node.stakeblock(10)

        # Not assigned moderators not allowed vote
        # /src/pocketdb/consensus/moderation/Vote.hpp:55
        for mod in notAssigned:
            assert_raises_rpc_error(44, None, pubGenTx, mod, ModVotePayload(jury1['data']['id'], 1))

        # ---------------------------------------------------------------------------------
        self.log.info("Test 4 - all moderators vote positive")

        # One vote does not pass verdict
        # /src/pocketdb/consensus/Base.h:689 moderation_jury_vote_count
        pubGenTx(assigned[0], ModVotePayload(jury1['data']['id'], 1))
        node.stakeblock(1)
        assert('verdict' not in node.public().getjury(jury1['data']['id']))

        # Second vote pass verdict
        pubGenTx(assigned[1], ModVotePayload(jury1['data']['id'], 1))
        node.stakeblock(1)
        assert('verdict' in node.public().getjury(jury1['data']['id']))
        assert(node.public().getjury(jury1['data']['id'])['verdict'] == 1)

        # Check ban
        assert(node.public().getbans(jury1['account'].Address)[0]['juryId'] == jury1['data']['id'])
        assert(node.public().getbans(jury1['account'].Address)[0]['reason'] == 1)
        assert(node.public().getbans(jury1['account'].Address)[0]['ending'] == 43200)

        # ---------------------------------------------------------------------------------
        self.log.info("Test 5 - banned account can not create pocketnet transactions")
        assert_raises_rpc_error(68, None, pubGenTx, jury1['account'], AccountPayload(jury1['account'].Name))
        assert_raises_rpc_error(68, None, pubGenTx, jury1['account'], ContentPostPayload())

if __name__ == '__main__':
    ModerationJuryTest().main()
