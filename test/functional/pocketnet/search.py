#!/usr/bin/env python3
# Copyright (c) 2023 The Pocketnet developers
# Distributed under the Apache 2.0 software license, see the accompanying
# https://www.apache.org/licenses/LICENSE-2.0
"""
This test checks the search functionality.
Launch this from 'test/functional/pocketnet' directory or via 
test_runner.py
"""

import sys
import pathlib

sys.path.insert(0, str(pathlib.Path(__file__).parent.parent))

from framework.chain_builder import ChainBuilder
from framework.helpers import add_content, comment_post, generate_subscription, like_comment, like_post
from framework.models import ContentPostPayload

from test_framework.test_framework import PocketcoinTestFramework


class SearchTest(PocketcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 2
        self.extra_args = [["-debug=consensus"], ["-debug=consensus"]]

    def set_up(self, builder):
        self.log.info("SetUp - Generating posts and comments")
        add_content(
            builder.node,
            builder.accounts[0],
            ContentPostPayload(
                caption="Sensation!", message="Very interesting text", url="https://fancy.domain.com", language="en"
            ),
        )
        account = builder.accounts[0]
        post_id = account.content[0]
        comment_post(builder.node, builder.accounts[1], post_id, "First user comment")
        comment_post(builder.node, builder.accounts[2], post_id, "Second user comment")
        comment_post(builder.node, builder.accounts[3], post_id, "Third user comment")
        comment_post(builder.node, builder.accounts[1], post_id, "First user second comment")
        comment_post(builder.node, builder.accounts[1], post_id, "First user third comment")
        like_comment(builder.node, builder.moderators[0], builder.accounts[1], builder.accounts[1].comment[0])
        like_post(builder.node, builder.accounts[0], builder.accounts[1], post_id, 5)
        like_post(builder.node, builder.accounts[0], builder.accounts[2], post_id, 4)
        generate_subscription(builder.node, builder.accounts[0], builder.accounts[1])
        generate_subscription(builder.node, builder.accounts[2], builder.accounts[0])

    def test_post_search(self):
        self.log.info("Test 1 - Search content")
        self.log.info("Check - Search for not existing keyword")
        result1 = self.nodes[0].public().search("fake", "posts")
        result2 = self.nodes[1].public().search("fake", "posts")

        assert result1 == result2
        assert result1["posts"]["data"] == []

        self.log.info("Check - Search for existing different keywords")
        result1 = self.nodes[0].public().search("interesting", "posts")
        result2 = self.nodes[1].public().search("sensation", "posts")
        assert result1 == result2

        data = result1["posts"]["data"]
        assert isinstance(data, list)
        assert len(data) == 1
        assert data[0]["c"] == "Sensation!"
        assert data[0]["m"] == "Very interesting text"

    def test_user_search(self, builder):
        self.log.info("Test 2 - Search users (search call)")

        self.log.info("Check - Search for not existing user")
        result = builder.node.public().search("fancy_user", "users")
        assert result == {"users": {"data": []}}

        self.log.info("Check - Search for concrete user")
        result = builder.node.public().search("user2", "users")
        data = result["users"]["data"]
        assert isinstance(data, list)
        assert len(data) == 1
        assert data[0]["address"] == builder.accounts[2].Address
        assert data[0]["name"] == "user2"

        self.log.info("Check - Search user by substring")
        result = builder.node.public().search("user", "users")
        data = result["users"]["data"]
        assert isinstance(data, list)
        assert len(data) == 5
        for user in data:
            assert user["name"] in ["user0", "user1", "user2", "user3", "user4"]

        result = builder.node.public().search("moder", "users")
        data = result["users"]["data"]
        assert isinstance(data, list)
        assert len(data) == 1
        assert data[0]["name"] == "moderator0"

    def test_searchlinks(self, builder):
        self.log.info("Test 3 - Search links")

        self.log.info("Check - Search for not existing url")
        result = builder.node.public().searchlinks(["https://fake.domain"])
        assert isinstance(result, list)
        assert len(result) == 0

        self.log.info("Check - Search for substring in url")
        result = builder.node.public().searchlinks(["fancy.domain.com"])
        assert isinstance(result, list)
        assert len(result) == 0

        self.log.info("Check - Search for exact url")
        result = builder.node.public().searchlinks(["https://fancy.domain.com"])
        assert isinstance(result, list)
        assert len(result) == 1
        assert result[0]["c"] == "Sensation!"
        assert result[0]["m"] == "Very interesting text"
        assert result[0]["u"] == "https://fancy.domain.com"

    def test_searchusers(self, builder):
        self.log.info("Test 4 - Search users (searchusers call)")

        self.log.info("Check - Search for not existing user")
        result = builder.node.public().searchusers("fancy_user")
        assert isinstance(result, list)
        assert result == []

        self.log.info("Check - Search for concrete user")
        result = builder.node.public().searchusers("user2")
        assert isinstance(result, list)
        assert len(result) == 1
        assert result[0]["address"] == builder.accounts[2].Address
        assert result[0]["name"] == "user2"

        self.log.info("Check - Search user by substring")
        result = builder.node.public().searchusers("user")
        assert isinstance(result, list)
        assert len(result) == 5
        for user in result:
            assert user["name"] in ["user0", "user1", "user2", "user3", "user4"]

        result = builder.node.public().searchusers("moder")
        assert isinstance(result, list)
        assert len(result) == 1
        assert result[0]["name"] == "moderator0"

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
        self.test_sync_nodes(builder)
        self.test_post_search()
        self.test_user_search(builder)
        self.test_searchlinks(builder)
        self.test_searchusers(builder)


if __name__ == "__main__":
    SearchTest().main()
