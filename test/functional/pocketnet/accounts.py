#!/usr/bin/env python3
# Copyright (c) 2023 The Pocketnet developers
# Distributed under the Apache 2.0 software license, see the accompanying
# https://www.apache.org/licenses/LICENSE-2.0
"""
This test covers the account interactions.
Launch this from 'test/functional/pocketnet' directory or via 
test_runner.py
"""

import json
import sys
import pathlib

sys.path.insert(0, str(pathlib.Path(__file__).parent.parent))

from framework.chain_builder import ChainBuilder
from framework.helpers import (
    add_content,
    comment_post,
    generate_account_blockings,
    generate_subscription,
    like_comment,
    like_post,
    set_account_setting,
)
from framework.models import ContentPostPayload

from test_framework.test_framework import PocketcoinTestFramework


class AccountsTest(PocketcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 2
        self.extra_args = [["-debug=consensus"], ["-debug=consensus"]]

    def set_up(self, builder):
        add_content(builder.node, builder.accounts[0], ContentPostPayload(language="ru"))
        like_post(builder.node, builder.accounts[0], builder.accounts[1], builder.accounts[0].content[0], 5)
        comment_post(builder.node, builder.accounts[1], builder.accounts[0].content[0])
        like_comment(builder.node, builder.accounts[0], builder.accounts[1], builder.accounts[1].comment[0])
        generate_subscription(builder.node, builder.accounts[1], builder.accounts[0])
        generate_subscription(builder.node, builder.accounts[2], builder.accounts[0])
        generate_subscription(builder.node, builder.accounts[0], builder.accounts[4])
        generate_account_blockings(builder.node, builder.accounts[0], builder.accounts[3])
        generate_account_blockings(builder.node, builder.accounts[4], builder.accounts[0])

    def test_getuseraddress(self, builder):
        self.log.info("Test 1 - Getting users addresses by their names")
        public_api = builder.node.public()
        for account in builder.accounts + builder.moderators:
            result = public_api.getuseraddress(account.Name)
            assert isinstance(result, list)
            assert len(result) == 1
            assert isinstance(result[0], dict)
            assert result[0]["address"] == account.Address
            assert result[0]["name"] == account.Name

    def test_getuserprofile(self, builder):
        self.log.info("Test 2 - Getting users profiles")
        public_api = builder.node.public()

        self.log.info("Check - No profile returned if address is not provided")
        profile = public_api.getuserprofile()
        assert isinstance(profile, list)
        assert len(profile) == 0

        self.log.info("Check - User profiles returned if addresses provided")
        profile = public_api.getuserprofile(builder.accounts[0].Address)
        assert isinstance(profile, list)
        assert len(profile) == 1
        assert profile[0]["name"] == builder.accounts[0].Name
        assert profile[0]["blocking"] == [builder.accounts[3].Address]
        assert profile[0]["blockings_count"] == 1
        assert profile[0]["content"] == {"200": 1}
        assert profile[0]["likers_count"] == 1
        assert profile[0]["postcnt"] == 1
        assert profile[0]["subscribers"] == [builder.accounts[1].Address, builder.accounts[2].Address]
        assert profile[0]["subscribers_count"] == 2
        assert profile[0]["subscribes"] == [{"adddress": builder.accounts[4].Address, "private": "false"}]
        assert profile[0]["subscribes_count"] == 1

    def test_getaddressregistration(self, builder):
        self.log.info("Test 3 - Getting address registration")
        public_api = builder.node.public()
        addresses = [account.Address for account in builder.accounts]
        registration = public_api.getaddressregistration(addresses)

        assert isinstance(registration, list)
        assert len(registration) == 5

        for reg in registration:
            assert isinstance(reg, dict)
            assert "time" in reg
            assert "txid" in reg
            assert reg["address"] in addresses

    def test_getuserstate(self, builder):
        self.log.info("Test 4 - Getting user state")
        public_api = builder.node.public()
        for account in builder.accounts:
            state = public_api.getuserstate(account.Address)
            assert isinstance(state, dict)
            assert state["address"] == account.Address

        self.log.info("Check - Account 1 state detail check")
        state = public_api.getuserstate(builder.accounts[0].Address)
        assert state["article_spent"] == 0
        assert state["audio_spent"] == 0
        assert state["comment_score_spent"] == 1
        assert state["comment_spent"] == 0
        assert state["complain_spent"] == 0
        assert state["likers"] == 1
        assert state["mod_flag_spent"] == 0
        assert state["post_spent"] == 1
        assert state["reputation"] == 2
        assert state["score_spent"] == 0
        assert state["stream_spent"] == 0
        assert state["trial"] == False
        assert state["video_spent"] == 0
        assert state["badges"] == ["shark"]
        assert state["mode"] == 1

    def test_txunspent(self, builder):
        self.log.info("Test 5 - Getting txunspent")
        public_api = builder.node.public()
        addresses = [account.Address for account in builder.accounts]
        unspent = public_api.txunspent(addresses)

        assert isinstance(unspent, list)
        for item in unspent:
            assert isinstance(item, dict)
            assert item["address"] in addresses
            assert "amount" in item
            assert "amountSat" in item
            assert "coinbase" in item
            assert "confirmations" in item
            assert "height" in item
            assert "pockettx" in item
            assert "scriptPubKey" in item
            assert "txid" in item
            assert "vout" in item

    def test_getaddressid(self, builder):
        self.log.info("Test 6 - Getting address id")
        public_api = builder.node.public()

        for account in builder.accounts:
            result = public_api.getaddressid(account.Address)
            assert isinstance(result, dict)
            assert result["address"] == account.Address
            id_ = result["id"]
            result_by_id = public_api.getaddressid(id_)
            assert isinstance(id_, int)
            assert result == result_by_id

    def test_getaccountsetting(self, builder):
        self.log.info("Test 7 - Getting account setting")
        public_api = builder.node.public()
        account = builder.accounts[0]

        account_setting = public_api.getaccountsetting(account.Address)
        assert isinstance(account_setting, str)
        assert account_setting == ""

        settings = {"foo": "bar", "test": True, "count": 5}
        set_account_setting(builder.node, account, settings)

        account_setting = public_api.getaccountsetting(account.Address)
        assert isinstance(account_setting, str)
        as_dict = json.loads(account_setting)
        assert as_dict == settings

    def test_getuserstatistic(self, builder):
        self.log.info("Test 8 - Getting user statistic")
        public_api = builder.node.public()
        account = builder.accounts[0]

        self.log.info("Check - Account without commentators and referals")
        statistic = public_api.getuserstatistic(account.Address)
        assert isinstance(statistic, list)
        assert len(statistic) == 1
        assert statistic[0]["address"] == account.Address
        assert statistic[0]["commentators"] == 0
        assert statistic[0]["histreferals"] == 0

    def test_getusersubscribes(self, builder):
        self.log.info("Test 9 - Getting user subscriptions")
        public_api = builder.node.public()

        accounts = [builder.accounts[0], builder.accounts[1], builder.accounts[2]]
        subscribes = [builder.accounts[4], builder.accounts[0], builder.accounts[0]]

        for account, subscribed_to in zip(accounts, subscribes):
            subs = public_api.getusersubscribes(account.Address)
            assert isinstance(subs, list)
            assert isinstance(subs[0], dict)
            assert subs[0]["adddress"] == subscribed_to.Address
            assert subs[0]["private"] == 0
            assert "height" in subs[0]
            assert "reputation" in subs[0]

    def test_getusersubscribers(self, builder):
        self.log.info("Test 10 - Getting user subscribers")
        public_api = builder.node.public()

        self.log.info("Check - Account with multiple subscribers")
        subscribers = public_api.getusersubscribers(builder.accounts[0].Address)
        expected = [builder.accounts[1].Address, builder.accounts[2].Address]
        assert isinstance(subscribers, list)
        assert len(subscribers) == 2
        for subscriber in subscribers:
            assert isinstance(subscriber, dict)
            assert subscriber["address"] in expected
            assert subscriber["private"] == 0
            assert "height" in subscriber
            assert "reputation" in subscriber
        
        self.log.info("Check - Account with single subscriber")
        subscribers = public_api.getusersubscribers(builder.accounts[4].Address)
        assert isinstance(subscribers, list)
        assert len(subscribers) == 1
        assert isinstance(subscribers[0], dict)
        assert subscribers[0]["address"] == builder.accounts[0].Address

    def test_getuserblockings(self, builder):
        self.log.info("Test 11 - Getting user blockings")
        public_api = builder.node.public()

        accounts = [builder.accounts[0], builder.accounts[4]]
        blockings = [builder.accounts[3], builder.accounts[0]]

        for account, blocking in zip(accounts, blockings):
            result = public_api.getuserblockings(account.Address)

            blocking_id = public_api.getaddressid(blocking.Address)
            assert isinstance(result, list)
            assert len(result) == 1
            assert result[0] == blocking_id["id"]

    def test_getuserblockers(self, builder):
        self.log.info("Test 12 - Getting user blockers")
        public_api = builder.node.public()

        accounts = [builder.accounts[3], builder.accounts[0]]
        blockers = [builder.accounts[0], builder.accounts[4]]

        for account, blocker in zip(accounts, blockers):
            result = public_api.getuserblockers(account.Address)

            blocker_id = public_api.getaddressid(blocker.Address)
            assert isinstance(result, list)
            assert len(result) == 1
            assert result[0] == blocker_id["id"]

    def test_gettopaccounts(self, builder):
        self.log.info("Test 13 - Getting top accounts")
        public_api = builder.node.public()

        self.log.info("Check - No top users so far")
        result = public_api.gettopaccounts()
        assert isinstance(result, list)
        assert len(result) == 0

    def test_sync_nodes(self, builder):
        self.log.info("Check - sync nodes")
        self.connect_nodes(1, 2)
        self.sync_all()

    def run_test(self):
        node = self.nodes[0]
        builder = ChainBuilder(node, self.log)
        builder.build_init(accounts_num=5, moderators_num=1)
        builder.register_accounts()

        self.set_up(builder)
        self.test_getuseraddress(builder)
        self.test_getuserprofile(builder)
        self.test_getaddressregistration(builder)
        self.test_getuserstate(builder)
        self.test_txunspent(builder)
        self.test_getaddressid(builder)
        self.test_getaccountsetting(builder)
        self.test_getuserstatistic(builder)
        self.test_getusersubscribes(builder)
        self.test_getusersubscribers(builder)
        self.test_getuserblockings(builder)
        self.test_getuserblockers(builder)
        self.test_gettopaccounts(builder)
        self.test_sync_nodes(builder)


if __name__ == "__main__":
    AccountsTest().main()
