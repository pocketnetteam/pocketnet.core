#!/usr/bin/env python3
# Copyright (c) 2023 The Pocketnet developers
# Distributed under the Apache 2.0 software license, see the accompanying
# https://www.apache.org/licenses/LICENSE-2.0
"""
This test creates the content with comments and then checks the comments correctness.
Launch this from 'test/functional/pocketnet' directory or via 
test_runner.py
"""

import sys
import pathlib

sys.path.insert(0, str(pathlib.Path(__file__).parent.parent))

from framework.chain_builder import ChainBuilder
from framework.helpers import add_content, comment_post, like_comment
from framework.models import ContentPostPayload

from test_framework.test_framework import PocketcoinTestFramework


class CommentsTest(PocketcoinTestFramework):
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

    def test_getcomments(self, builder):
        self.log.info("Test 1 - Getting comments")
        public_api = builder.node.public()
        account = builder.accounts[0]
        post_id = account.content[0]

        comments = public_api.getcomments(post_id, "", account.Address)
        comment_with_likes = comments[0]
        assert comment_with_likes["reputation"] == "1"
        assert comment_with_likes["scoreUp"] == "1"

        self.check_comments(builder, builder.node)

    def test_getlastcomments(self, builder):
        self.log.info("Test 2 - Getting last comments")
        public_api = builder.node.public()
        account = builder.accounts[0]
        post_id = account.content[0]

        last_comments = public_api.getlastcomments("7", "", "ru")

        assert isinstance(last_comments, list)
        assert len(last_comments) == 1
        assert last_comments[0]["msg"] == "First user comment"
        assert last_comments[0]["postid"] == post_id
        assert last_comments[0]["address"] == builder.accounts[1].Address

        like_comment(builder.node, builder.moderators[0], builder.accounts[3], builder.accounts[3].comment[0])

        self.check_lastcomments(builder.node)

    def test_sync_nodes(self, builder):
        self.log.info("Test 3 - Getting comments & last comments after syncing 2 nodes")
        self.connect_nodes(1, 2)
        self.sync_all()

        node2 = self.nodes[1]

        self.log.info("Check - Getting comments from node 2")
        self.check_comments(builder, node2)

        self.log.info("Check - Getting last comments from node 2")
        self.check_lastcomments(node2)

    def check_comments(self, builder, node):
        public_api = node.public()
        account = builder.accounts[0]
        post_id = account.content[0]
        comments = public_api.getcomments(post_id, "", account.Address)
        assert len(comments) == 5
        accounts = [
            builder.accounts[1],
            builder.accounts[2],
            builder.accounts[3],
            builder.accounts[1],
            builder.accounts[1],
        ]
        texts = [
            "First user comment",
            "Second user comment",
            "Third user comment",
            "First user second comment",
            "First user third comment",
        ]

        for comment, account, text in zip(comments, accounts, texts):
            assert comment["address"] == account.Address
            assert comment["msg"] == text

    def check_lastcomments(self, node):
        public_api = node.public()
        last_comments = public_api.getlastcomments("7", "", "ru")
        assert len(last_comments) == 2
        msgs = [comment["msg"] for comment in last_comments]
        assert "First user comment" in msgs
        assert "Third user comment" in msgs

    def run_test(self):
        node = self.nodes[0]
        builder = ChainBuilder(node, self.log)
        builder.build_init(accounts_num=5, moderators_num=1)
        builder.register_accounts()

        self.set_up(builder)
        self.test_getcomments(builder)
        self.test_getlastcomments(builder)
        self.test_sync_nodes(builder)


if __name__ == "__main__":
    CommentsTest().main()
