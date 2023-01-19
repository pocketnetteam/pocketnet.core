#!/usr/bin/env python3
# Copyright (c) 2018-2023 The Pocketnet developers
# Distributed under the Apache 2.0 software license, see the accompanying
# https://www.apache.org/licenses/LICENSE-2.0
"""
A Moderation Jury no ban functional test
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


class ModerationJuryNegativeTest(PocketcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        # TODO: run multiple and check the synchronization and conflicting transactions
        self.num_nodes = 1
        self.extra_args = [["-debug=consensus"]]

    def create_jury(self, account, moders):
        node = self.nodes[0]
        pubGenTx = node.public().generatetransaction

        # Post for flags
        jury = {
            'post': pubGenTx(account, ContentPostPayload()),
            'account': account
        }

        node.stakeblock(1)

        # We need 2 flags in 10 blocks for create jury
        # /src/pocketdb/consensus/Base.h:674 moderation_jury_flag_count
        # /src/pocketdb/consensus/Base.h:679 moderation_jury_flag_depth

        lastFlagTx = pubGenTx(moders[0], ModFlagPayload(jury['post'], jury['account'].Address, 1))
        node.stakeblock(10)
        lastFlagTx = pubGenTx(moders[1], ModFlagPayload(jury['post'], jury['account'].Address, 1))
        node.stakeblock(1)
        assert('id' not in node.public().getjury(lastFlagTx))
        assert(len(node.public().getjurymoderators(lastFlagTx)) == 0)

        # ---------------------------------------------------------------------------------

        lastFlagTx = pubGenTx(moders[3], ModFlagPayload(jury['post'], jury['account'].Address, 1))
        node.stakeblock(1)
        jury['data'] = node.public().getjury(lastFlagTx)
        assert('id' in jury['data'] and jury['data']['id'] == lastFlagTx)
        jury['mods'] = node.public().getjurymoderators(lastFlagTx)
        assert(len(jury['mods']) == 4)

        return jury

    def check_can_post(self, account, message="Not blocked"):
        self.log.info(f"Check - account is able to create new post: '{message}'")

        node = self.nodes[0]
        pubGenTx = node.public().generatetransaction

        pubGenTx(account, AccountPayload(account.Name))
        node.stakeblock(1)
        pubGenTx(account, ContentPostPayload(message=message))
        node.stakeblock(1)
        posts = node.public().getcontents(account.Address)
        assert(node.public().getcontent(posts[0]['txid'])[0]['m'] == message)

    def assert_verdict(self, jury, expected):
        self.log.info(f"Check - jury verdict: {'yes' if expected else 'no'}")

        node = self.nodes[0]
        assert('verdict' in node.public().getjury(jury['data']['id']))
        assert(node.public().getjury(jury['data']['id'])['verdict'] == expected)

    def assert_no_verdict(self, jury):
        self.log.info("Check - there is no verdict for the jury")

        node = self.nodes[0]
        assert('verdict' not in node.public().getjury(jury['data']['id']))

    def assert_ban(self, account, expected):
        self.log.info(f"Check - account is banned: {'yes' if expected else 'no'}")

        node = self.nodes[0]
        bans = node.public().getbans(account.Address)
        assert(len(bans) == expected)

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
        self.log.info("Test 1 - first moderator voted yes, second moderator voted no")

        jury1 = self.create_jury(accounts[0], moders)

        assigned = [moder for moder in moders if moder.Address in jury1['mods']]
        node.stakeblock(10)

        # One vote does not pass verdict
        # /src/pocketdb/consensus/Base.h:689 moderation_jury_vote_count
        pubGenTx(assigned[0], ModVotePayload(jury1['data']['id'], 1))
        node.stakeblock(1)
        assert('verdict' not in node.public().getjury(jury1['data']['id']))

        # Second vote is negative
        pubGenTx(assigned[1], ModVotePayload(jury1['data']['id'], 0))
        node.stakeblock(1)

        self.assert_verdict(jury1, expected=0)
        self.assert_ban(jury1['account'], expected=0)
        self.check_can_post(jury1['account'], "Not banned on second vote")

        # ---------------------------------------------------------------------------------
        self.log.info("Test 2 - first moderator voted no")

        jury2 = self.create_jury(accounts[1], moders)

        assigned = [moder for moder in moders if moder.Address in jury2['mods']]
        node.stakeblock(10)

        pubGenTx(assigned[0], ModVotePayload(jury2['data']['id'], 0))
        node.stakeblock(1)

        self.assert_verdict(jury2, expected=0)
        self.assert_ban(jury2['account'], expected=0)
        self.check_can_post(jury2['account'], "Not banned on first vote")

        # ---------------------------------------------------------------------------------
        self.log.info("Test 3 - Rollback blocks for remove verdict and jury")

        jury3 = self.create_jury(accounts[2], moders)

        assigned = [moder for moder in moders if moder.Address in jury3['mods']]
        node.stakeblock(10)

        # both moderators vote yes
        pubGenTx(assigned[0], ModVotePayload(jury3['data']['id'], 1))
        node.stakeblock(1)
        pubGenTx(assigned[1], ModVotePayload(jury3['data']['id'], 1))
        node.stakeblock(1)

        self.assert_verdict(jury3, expected=1)
        self.assert_ban(jury3['account'], expected=1)

        rollback_node(node, 2, self.log)

        self.assert_no_verdict(jury3)
        self.assert_ban(jury3['account'], expected=0)


if __name__ == '__main__':
    ModerationJuryNegativeTest().main()

