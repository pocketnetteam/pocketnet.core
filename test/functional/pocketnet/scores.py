#!/usr/bin/env python3
# Copyright (c) 2023 The Pocketnet developers
# Distributed under the Apache 2.0 software license, see the accompanying
# https://www.apache.org/licenses/LICENSE-2.0
"""
This test checks the score calls.
Launch this from 'test/functional/pocketnet' directory or via 
test_runner.py
"""

import sys
import pathlib

sys.path.insert(0, str(pathlib.Path(__file__).parent.parent))

from framework.chain_builder import ChainBuilder
from framework.helpers import add_content, comment_post, like_comment, like_post
from framework.models import ContentPostPayload

from test_framework.test_framework import PocketcoinTestFramework


class ScoresTest(PocketcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 2
        self.extra_args = [["-debug=consensus"], ["-debug=consensus"]]

    def set_up(self, builder):
        self.log.info("SetUp - Generating posts and comments")
        add_content(builder.node, builder.accounts[0], ContentPostPayload(language="ru"))
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

    def test_getaddressscores(self, builder):
        self.log.info("Test 1 - Getting address scores")
        public_api = builder.node.public()
        account = builder.accounts[0]
        post_id = account.content[0]

        self.log.info("Check - address scores are empty for accounts without activity")
        scores = public_api.getaddressscores(account.Address, post_id)
        assert isinstance(scores, list)
        assert len(scores) == 0

        self.log.info("Check - address scores are not empty for user1 and user2")
        for account, score in zip([builder.accounts[1], builder.accounts[2]], [5, 4]):
            scores = public_api.getaddressscores(account.Address, post_id)
            assert isinstance(scores, list)
            assert len(scores) == 1
            assert scores[0]["address"] == account.Address
            assert scores[0]["posttxid"] == post_id
            assert scores[0]["value"] == score

    def test_getpostscores(self, builder):
        self.log.info("Test 2 - Getting post scores")
        public_api = builder.node.public()
        account = builder.accounts[0]
        post_id = account.content[0]

        scores = public_api.getpostscores(post_id)
        assert isinstance(scores, list)
        assert len(scores) == 2

        for score in scores:
            assert score["address"] in [builder.accounts[1].Address, builder.accounts[2].Address]
            assert score["name"] in ["user1", "user2"]
            assert score["posttxid"] == post_id
            assert score["value"] in ["4", "5"]

    def test_getpagescores(self, builder):
        self.log.info("Test 3 - Getting page scores")
        public_api = builder.node.public()
        account = builder.accounts[0]
        post_id = account.content[0]

        comments = [
            builder.accounts[1].comment[0],
            builder.accounts[2].comment[0],
            builder.accounts[3].comment[0],
            builder.accounts[1].comment[1],
        ]

        scores = public_api.getpagescores([post_id], account.Address, comments)
        assert isinstance(scores, list)
        assert len(scores) == 4

        for score in scores:
            assert score["cmntid"] in comments
            assert score["scoreDown"] == "0"
            if score["cmntid"] == comments[0]:
                assert score["scoreUp"] == "1"
                assert score["reputation"] == "1"
            else:
                assert score["scoreUp"] == "0"
                assert "reputation" not in score

    def test_getaccountraters(self, builder):
        self.log.info("Test 4 - Getting account raters")
        public_api = builder.node.public()
        account = builder.accounts[0]

        self.log.info("Check - account raters are empty for accounts without likes")
        scores = public_api.getaddressscores(builder.accounts[3].Address)
        assert isinstance(scores, list)
        assert len(scores) == 0

        self.log.info("Check - account raters are not empty for accounts with likes")
        accounts = [builder.accounts[0], builder.accounts[1]]
        rater_accounts = [builder.accounts[1], builder.moderators[0]]
        for account, rater in zip(accounts, rater_accounts):
            result = public_api.getaccountraters(account.Address)

            assert isinstance(result, list)
            assert len(result) == 1
            assert result[0]["address"] == rater.Address
            assert result[0]["name"] == rater.Name
            assert result[0]["ratingscnt"] == 1

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
        self.test_getaddressscores(builder)
        self.test_getpostscores(builder)
        self.test_getpagescores(builder)
        self.test_getaccountraters(builder)
        self.test_sync_nodes(builder)


if __name__ == "__main__":
    ScoresTest().main()
