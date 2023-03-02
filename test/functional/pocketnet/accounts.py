#!/usr/bin/env python3
# Copyright (c) 2023 The Pocketnet developers
# Distributed under the Apache 2.0 software license, see the accompanying
# https://www.apache.org/licenses/LICENSE-2.0
"""
This test covers the account interactions.
Launch this from 'test/functional/pocketnet' directory or via 
test_runner.py
"""

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
        self.test_sync_nodes(builder)


if __name__ == "__main__":
    AccountsTest().main()
