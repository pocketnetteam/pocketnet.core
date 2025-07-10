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
        self.log.info("Generate post for set badges")

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
            assert "moderator" not in node.public().getuserstate(acc.Address)["badges"]

        # ---------------------------------------------------------------------------------
        self.log.info("Create comments from all acounts and like 1/2 accounts only")

        # Create comments from moderators
        for acc in accounts:
            acc.badgeComment = pubGenTx(acc, CommentPayload(fakePostTx))
        node.stakeblock(1)  # height : 1046

        # Like another for set comment liker
        for i in range(len(accounts)):
            accTo = accounts[0 if i == len(accounts) - 1 else i + 1]
            accTo.moderator = False
            if i < len(accounts) / 2:
                accTo.moderator = True
                pubGenTx(
                    accounts[i],
                    ScoreCommentPayload(accTo.badgeComment, 1, accTo.Address),
                )

        # Stake for nearest Badge update period
        # /src/pocketdb/consensus/Base.h:326
        node.stakeblock(4)  # height : 1050

        # Check moderator badges
        for acc in accounts:
            if acc.moderator:
                assert "moderator" in node.public().getuserstate(acc.Address)["badges"]
            else:
                assert "moderator" not in node.public().getuserstate(acc.Address)["badges"]

        # Rollback 1 block for remove all badges
        rollback_node(node, 1, self.log)  # height : 1049
        for acc in accounts:
            assert "moderator" not in node.public().getuserstate(acc.Address)["badges"]

        node.stakeblock(1)  # height : 1050

        # ---------------------------------------------------------------------------------
        self.log.info("Stake chain to 1150 height for remove all badges")
        # /src/pocketdb/consensus/Base.h:326

        node.stakeblock(100)
        # height : 1150

        for acc in accounts:
            assert "moderator" not in node.public().getuserstate(acc.Address)["badges"]

        # ---------------------------------------------------------------------------------
        self.log.info("Rollback chain to 1149 height for restore 1/2 badges")

        rollback_node(node, 1, self.log)  # height : 1149

        # Recheck restored badges
        for acc in accounts:
            if acc.moderator:
                assert "moderator" in node.public().getuserstate(acc.Address)["badges"]
            else:
                assert "moderator" not in node.public().getuserstate(acc.Address)["badges"]

        # ---------------------------------------------------------------------------------
        self.lof.info("Set up Badge 'Verificated' from developer and not developer")

        # RegTest developers
        regDevs = [
            Account("", "", f"dev1"),
            Account("", "", f"dev2"),
            Account("", "", f"dev3"),
        ]

        # Try create badge from developer without bastyon account - fail
        pubGenTx(dev[0], MoneyTransaction(f"a:b:verificated {accounts[1].Address}".encode('utf-8').hex()), fee=500)
        node.stakeblock(1)
        assert not any(b['badge'] == 'verificated' for b in node.public().getbadgehistory(accounts[1].Address, "verificated"))
        assert not any(b == 'verificated' for b in node.public().getuserprofile(accounts[1].Address)[0]['badges'])

        # Try create Badge 'Verificated' from non-developer account - fail
        pubGenTx(accounts[0], MoneyTransaction(f"a:b:verificated {accounts[1].Address}".encode('utf-8').hex()), fee=500)
        node.stakeblock(1)
        assert not any(b['badge'] == 'verificated' for b in node.public().getbadgehistory(accounts[1].Address, "verificated"))
        assert not any(b == 'verificated' for b in node.public().getuserprofile(accounts[1].Address)[0]['badges'])

        # Try create Badge 'Verificated' from non-developer account and without bastyon registration - fail
        emptyAcc = node.public().generateaddress()
        emptyAcc = Account(emptyAcc["address"], emptyAcc["privkey"], "fake")
        node.sendtoaddress(address=emptyAcc.Address, amount=1, destaddress=nodeAddress)
        node.sendtoaddress(address=emptyAcc.Address, amount=1, destaddress=nodeAddress)
        node.stakeblock(1)

        pubGenTx(emptyAcc, MoneyTransaction(f"a:b:verificated {accounts[1].Address}".encode('utf-8').hex()), fee=500)
        node.stakeblock(1)
        assert not any(b['badge'] == 'verificated' for b in node.public().getbadgehistory(accounts[1].Address, "verificated"))
        assert not any(b == 'verificated' for b in node.public().getuserprofile(accounts[1].Address)[0]['badges'])
        
        # Register bastyon accounts for devs
        for dev in regDevs:
            node.sendtoaddress(address=dev.Address, amount=1, destaddress=nodeAddress)
            node.sendtoaddress(address=dev.Address, amount=1, destaddress=nodeAddress)
            node.stakeblock(1)
            pubGenTx(
                dev,
                AccountPayload(dev.Name, "image", "en", "about", "s", "b", "pubkey"),
                1,
                0,
            )
        node.stakeblock(1)

        # Try create Badge from deveveloper with bastyon account - success
        pubGenTx(dev[0], MoneyTransaction(f"a:b:verificated {accounts[1].Address}".encode('utf-8').hex()), fee=500)
        node.stakeblock(1)
        assert len([b for b in node.public().getbadgehistory(accounts[1].Address, "verificated") if b['badge'] == 'verificated')] == 1
        assert any(b == 'verificated' for b in node.public().getuserprofile(accounts[1].Address)[0]['badges'])

        # Try create Badge from deveveloper with bastyon account twice - faile
        pubGenTx(dev[0], MoneyTransaction(f"a:b:verificated {accounts[1].Address}".encode('utf-8').hex()), fee=500)
        node.stakeblock(1)
        assert len([b for b in node.public().getbadgehistory(accounts[1].Address, "verificated") if b['badge'] == 'verificated')] == 1
        assert any(b == 'verificated' for b in node.public().getuserprofile(accounts[1].Address)[0]['badges'])

        # Try cancel Badge from developer - success
        pubGenTx(dev[0], MoneyTransaction(f"a:u:verificated {accounts[1].Address}".encode('utf-8').hex()), fee=500)
        node.stakeblock(1)
        assert len([b for b in node.public().getbadgehistory(accounts[1].Address, "verificated") if b['badge'] == 'verificated')] == 2
        assert not any(b == 'verificated' for b in node.public().getuserprofile(accounts[1].Address)[0]['badges'])

        # Try cancel Badge from developer twice - fail
        pubGenTx(dev[0], MoneyTransaction(f"a:u:verificated {accounts[1].Address}".encode('utf-8').hex()), fee=500)
        node.stakeblock(1)
        assert len([b for b in node.public().getbadgehistory(accounts[1].Address, "verificated") if b['badge'] == 'verificated')] == 2
        assert not any(b == 'verificated' for b in node.public().getuserprofile(accounts[1].Address)[0]['badges'])

        # Setup badge 'verificated' for next usage - success
        pubGenTx(dev[0], MoneyTransaction(f"a:b:verificated {accounts[1].Address}".encode('utf-8').hex()), fee=500)
        node.stakeblock(1)
        assert len([b for b in node.public().getbadgehistory(accounts[1].Address, "verificated") if b['badge'] == 'verificated']) == 3
        assert any(b == 'verificated' for b in node.public().getuserprofile(accounts[1].Address)[0]['badges'])

        # ---------------------------------------------------------------------------------
        self.lof.info("Set up Badge 'Validator' from developer and not developer")

        # Try create badge from developer - success
        pubGenTx(dev[0], MoneyTransaction(f"a:b:validator {accounts[2].Address}".encode('utf-8').hex()), fee=500)
        node.stakeblock(1)
        assert len([b for b in node.public().getbadgehistory(accounts[2].Address, "validator") in b['badge'] == 'validator']) == 1
        assert any(b == 'validator' for b in node.public().getuserprofile(accounts[2].Address)[0]['badges'])

        # Try cancel badge from developer - success
        pubGenTx(dev[0], MoneyTransaction(f"a:u:validator {accounts[2].Address}".encode('utf-8').hex()), fee=500)
        node.stakeblock(1)
        assert any(b['badge'] == 'validator' and b['cancel'] == 1 for b in node.public().getbadgehistory(accounts[2].Address, "validator"))
        assert not any(b == 'validator' for b in node.public().getuserprofile(accounts[2].Address)[0]['badges'])

        # Try create badge from account without 'Verificated' badge - fail
        pubGenTx(accounts[0], MoneyTransaction(f"a:b:validator {accounts[3].Address}".encode('utf-8').hex()), fee=500)
        node.stakeblock(1)
        assert len([b for b in node.public().getbadgehistory(accounts[3].Address, "validator") if b['badge'] == 'validator']) == 0
        assert not any(b == 'validator' for b in node.public().getuserprofile(accounts[3].Address)[0]['badges'])

        # Try create badge from account with 'Verificated' badge - success
        pubGenTx(accounts[1], MoneyTransaction(f"a:b:validator {accounts[3].Address}".encode('utf-8').hex()), fee=500)
        node.stakeblock(1)
        assert len([b for b in node.public().getbadgehistory(accounts[3].Address, "validator") if b['badge'] == 'validator']) == 1
        assert any(b == 'validator' for b in node.public().getuserprofile(accounts[3].Address)[0]['badges'])

        # Try cancel badge from account without 'Verificated' badge - fail
        pubGenTx(accounts[0], MoneyTransaction(f"a:u:validator {accounts[3].Address}".encode('utf-8').hex()), fee=500)
        node.stakeblock(1)
        assert len([b for b in node.public().getbadgehistory(accounts[3].Address, "validator") if b['badge'] == 'validator']) == 1
        assert any(b == 'validator' for b in node.public().getuserprofile(accounts[3].Address)[0]['badges'])

        # Try cancel badge from account with 'Verificated' badge - success
        pubGenTx(accounts[1], MoneyTransaction(f"a:u:validator {accounts[3].Address}".encode('utf-8').hex()), fee=500)
        node.stakeblock(1)
        assert len([b for b in node.public().getbadgehistory(accounts[3].Address, "validator") if b['badge'] == 'validator']) == 2
        assert not any(b == 'validator' for b in node.public().getuserprofile(accounts[3].Address)[0]['badges'])

        # Set up badge 'Verificated' for next usage - success
        pubGenTx(accounts[1], MoneyTransaction(f"a:u:validator {accounts[3].Address}".encode('utf-8').hex()), fee=500)
        node.stakeblock(1)
        assert len([b for b in node.public().getbadgehistory(accounts[3].Address, "validator") if b['badge'] == 'validator']) == 3
        assert any(b == 'validator' for b in node.public().getuserprofile(accounts[3].Address)[0]['badges'])

        # ---------------------------------------------------------------------------------
        self.lof.info("Set up Badge 'Verificated_ZN' from developer and not developer")

        # Try create badge from developer - success
        pubGenTx(dev[0], MoneyTransaction(f"a:b:verificated_zn {accounts[4].Address}".encode('utf-8').hex()), fee=500)
        node.stakeblock(1)
        assert len([b for b in node.public().getbadgehistory(accounts[4].Address, "verificated_zn") if b['badge'] == 'verificated_zn']) == 1
        assert any(b == 'verificated_zn' for b in node.public().getuserprofile(accounts[4].Address)[0]['badges'])

        # Try cancel badge from developer - success
        pubGenTx(dev[0], MoneyTransaction(f"a:u:verificated_zn {accounts[4].Address}".encode('utf-8').hex()), fee=500)
        node.stakeblock(1)
        assert len([b for b in node.public().getbadgehistory(accounts[4].Address, "verificated_zn") if b['badge'] == 'verificated_zn']) == 2
        assert not any(b == 'verificated_zn' for b in node.public().getuserprofile(accounts[4].Address)[0]['badges'])

        # Try create badge from account with 'Verificated' badge - fail
        pubGenTx(accounts[1], MoneyTransaction(f"a:b:verificated_zn {accounts[4].Address}".encode('utf-8').hex()), fee=500)
        node.stakeblock(1)
        assert len([b for b in node.public().getbadgehistory(accounts[4].Address, "verificated_zn") if b['badge'] == 'verificated_zn']) == 2
        assert not any(b == 'verificated_zn' for b in node.public().getuserprofile(accounts[4].Address)[0]['badges'])

        # Try create badge from account with 'Validator' badge - success
        pubGenTx(accounts[3], MoneyTransaction(f"a:b:verificated_zn {accounts[4].Address}".encode('utf-8').hex()), fee=500)
        node.stakeblock(1)
        assert len([b for b in node.public().getbadgehistory(accounts[4].Address, "verificated_zn") if b['badge'] == 'verificated_zn']) == 3
        assert any(b == 'verificated_zn' for b in node.public().getuserprofile(accounts[4].Address)[0]['badges'])

        # Try cancel badge from account without 'Validator' badge - fail
        pubGenTx(accounts[1], MoneyTransaction(f"a:u:verificated_zn {accounts[4].Address}".encode('utf-8').hex()), fee=500)
        node.stakeblock(1)
        assert len([b for b in node.public().getbadgehistory(accounts[4].Address, "verificated_zn") if b['badge'] == 'verificated_zn']) == 3
        assert any(b == 'verificated_zn' for b in node.public().getuserprofile(accounts[4].Address)[0]['badges'])

        # Try cancel badge from account with 'Validator' badge - success
        pubGenTx(accounts[3], MoneyTransaction(f"a:u:verificated_zn {accounts[4].Address}".encode('utf-8').hex()), fee=500)
        node.stakeblock(1)
        assert len([b for b in node.public().getbadgehistory(accounts[4].Address, "verificated_zn") if b['badge'] == 'verificated_zn']) == 4
        assert not any(b == 'verificated_zn' for b in node.public().getuserprofile(accounts[4].Address)[0]['badges'])



if __name__ == "__main__":
    ModerationJuryTest().main()
