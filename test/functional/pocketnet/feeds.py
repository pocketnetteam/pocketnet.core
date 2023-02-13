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
from framework.helpers import add_content, generate_subscription
from framework.models import (
    ContentPostPayload,
    ContentVideoPayload,
    ContentArticlePayload,
    ContentStreamPayload,
    ContentAudioPayload,
)

from test_framework.test_framework import PocketcoinTestFramework


DEFAULT_FEED_PARAMS = [0, "", 20, "", [], [], [], [], [], ""]


class FeedTest(PocketcoinTestFramework):
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

    def test_historical_feed(self, builder):
        self.log.info("Test 1 - Historical feed")
        feed = builder.node.public().gethistoricalfeed()
        contents = feed["contents"]
        self.log.info(f"Check - Number of posts in historical feed: {len(contents)}")
        assert len(contents) == 9

        for content_type, content_len in zip([None, "post", "video", "article", "audio", "stream"], [9, 9, 2, 1, 1, 1]):
            params = [0, "", 20, "", [], [content_type] if content_type else [], [], [], []]
            feed = builder.node.public().gethistoricalfeed(*params)
            contents = feed["contents"]
            self.log.info(f"Check - Number of posts of type {content_type} in historical feed: {len(contents)}")
            assert len(contents) == content_len

    def test_profile_feed(self, builder):
        self.log.info("Test 2 - Profile feed")
        accounts = builder.accounts + builder.moderators
        for account, content_len in zip(accounts, [2, 3, 4, 0]):
            params = DEFAULT_FEED_PARAMS.copy() + [account.Address, "", "", "desc"]
            feed = builder.node.public().getprofilefeed(*params)
            contents = feed["contents"]
            self.log.info(f"Check - Number of posts in profile feed: {len(contents)}")
            assert len(contents) == content_len

    def test_subscribes_feed_no_subscriptions(self, builder):
        self.log.info("Test 3 - Subscribes feed: No subscriptions")
        accounts = builder.accounts + builder.moderators
        for account in accounts:
            params = DEFAULT_FEED_PARAMS.copy() + [account.Address]
            feed = builder.node.public().getsubscribesfeed(*params)
            contents = feed["contents"]
            self.log.info(f"Check - Number of posts in subscribes feed: {len(contents)}")
            assert len(contents) == 0

    def test_subscribes_feed_with_subscriptions(self, builder):
        self.log.info("Test 4 - Subscribes feed: With subscriptions")
        accounts = builder.accounts + builder.moderators

        self.log.info("Account 1 subscribes to accounts 2 and 3")
        generate_subscription(builder.node, accounts[0], accounts[1])
        generate_subscription(builder.node, accounts[0], accounts[2])

        self.log.info("Account 2 subscribes to account 3 and moderator")
        generate_subscription(builder.node, accounts[1], accounts[2])
        generate_subscription(builder.node, accounts[1], accounts[3])

        for account, content_len, post_authors in zip(
            accounts, [7, 4, 0, 0], [[accounts[1].Address, accounts[2].Address], [accounts[2].Address], [], []]
        ):
            params = DEFAULT_FEED_PARAMS.copy() + [account.Address]
            feed = builder.node.public().getsubscribesfeed(*params)
            contents = feed["contents"]
            self.log.info(f"Check - Number of posts for account {account.Address} in subscribes feed: {len(contents)}")
            assert len(contents) == content_len
            for item in contents:
                assert item["address"] in post_authors

    def test_boostfeed(self, builder):
        self.log.info("Test 5 - Boosted feed")
        self.log.warning("Not implemented yet")

    def test_topfeed(self, builder):
        self.log.info("Test 6 - Top feed")
        self.log.warning("Not implemented yet")

    def test_mostcommentedfeed(self, builder):
        self.log.info("Test 5 - Most commented feed")
        self.log.warning("Not implemented yet")

    def run_test(self):
        node = self.nodes[0]
        builder = ChainBuilder(node, self.log)
        builder.build_init(accounts_num=3, moderators_num=1)
        builder.register_accounts()
        self.generate_posts(builder)

        self.test_historical_feed(builder)
        self.test_profile_feed(builder)
        self.test_subscribes_feed_no_subscriptions(builder)
        self.test_subscribes_feed_with_subscriptions(builder)
        self.test_boostfeed(builder)
        self.test_topfeed(builder)
        self.test_mostcommentedfeed(builder)


if __name__ == "__main__":
    FeedTest().main()
