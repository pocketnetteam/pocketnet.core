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
from framework.helpers import add_content, comment_post
from framework.models import ContentPostPayload

from test_framework.test_framework import PocketcoinTestFramework


class CommentsTest(PocketcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 1
        self.extra_args = [["-debug=consensus"]]

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

    def test_getcomments(self, builder):
        self.log.info("Test 1 - Getting comments")
        public_api = builder.node.public()
        account = builder.accounts[0]
        post_id = account.content[0]

        comments = public_api.getcomments(post_id, "", account.Address)

        from pprint import pprint
        pprint(comments)

    def test_getlastcomments(self, builder):
        self.log.info("Test 2 - Getting last comments")
        public_api = builder.node.public()
        account = builder.accounts[0]
        post_id = account.content[0]

        last_comments = public_api.getlastcomments("7", "", "ru")

        # for account, comments_count in zip(builder.accounts, [0, 3, 1, 1]):
        #     last_comments = public_api.getlastcomments([post_id], account.Address)
            # assert len(last_comments) == comments_count

        from pprint import pprint
        pprint(account.Address)
        pprint(last_comments)

    def run_test(self):
        node = self.nodes[0]
        builder = ChainBuilder(node, self.log)
        builder.build_init(accounts_num=5, moderators_num=1)
        builder.register_accounts()
        
        self.set_up(builder)
        self.test_getcomments(builder)
        # self.test_getlastcomments(builder)


if __name__ == "__main__":
    CommentsTest().main()
