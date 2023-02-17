#!/usr/bin/env python3
# Copyright (c) 2023 The Pocketnet developers
# Distributed under the Apache 2.0 software license, see the accompanying
# https://www.apache.org/licenses/LICENSE-2.0
"""
This test creates the content and then checks the feed correctness.
Launch this from 'test/functional/pocketnet' directory or via 
test_runner.py
"""

import sys
import pathlib

sys.path.insert(0, str(pathlib.Path(__file__).parent.parent))

from framework.chain_builder import ChainBuilder
from framework.helpers import add_content
from framework.models import ContentArticlePayload, ContentAudioPayload, ContentPostPayload, ContentStreamPayload, ContentVideoPayload
from test_framework.test_framework import PocketcoinTestFramework


class ContentTest(PocketcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 1
        self.extra_args = [["-debug=consensus"]]

    def generate_posts(self, builder):
        # Account 1: Post + Article
        add_content(builder.node, builder.accounts[0], ContentPostPayload())
        add_content(builder.node, builder.accounts[0], ContentArticlePayload())
        # Account 2: Post + 2 Videos
        add_content(builder.node, builder.accounts[1], ContentPostPayload())
        add_content(builder.node, builder.accounts[1], ContentVideoPayload())
        add_content(builder.node, builder.accounts[1], ContentVideoPayload())
        # Account 3: 2 Posts + Stream + Audio
        add_content(builder.node, builder.accounts[2], ContentPostPayload())
        add_content(builder.node, builder.accounts[2], ContentPostPayload())
        add_content(builder.node, builder.accounts[2], ContentStreamPayload())
        add_content(builder.node, builder.accounts[2], ContentAudioPayload())
        builder.node.stakeblock(1)

    def run_test(self):
        node = self.nodes[0]
        builder = ChainBuilder(node, self.log)
        builder.build_init(accounts_num=3, moderators_num=1)
        builder.register_accounts()
        self.generate_posts()

        self.test_getcontent(builder)
        self.test_getcontentsstatistic(builder)
        self.test_getcontents(builder)
        self.test_getrandomcontents(builder)
        self.test_getcontentactions(builder)
        self.test_getevents(builder)
        self.test_getactivities(builder)
        self.test_getnotifications(builder)
        self.test_getnotificationssummary(builder)


if __name__ == "__main__":
    ContentTest().main()
