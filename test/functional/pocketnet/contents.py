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
from framework.helpers import add_content, boost_post, comment_post, like_post
from framework.models import (
    ContentArticlePayload,
    ContentAudioPayload,
    ContentPostPayload,
    ContentStreamPayload,
    ContentVideoPayload,
)
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

    def test_getcontent(self, builder):
        self.log.info("Test 1 - Getting content")
        public_api = builder.node.public()
        accounts = builder.accounts + builder.moderators

        self.log.info("Check - getcontent and getcontents return correct amount of data")
        contents = [public_api.getcontents(account.Address) for account in accounts]

        for account, content, content_len in zip(accounts, contents, [2, 3, 4, 0, 0]):
            assert len(content) == content_len
            ids = [c["txid"] for c in content]
            content_data = public_api.getcontent(ids, account.Address)
            assert len(content_data) == content_len

        self.log.info("Check - getcontent not for existing content IDs returns empty list")
        result = public_api.getcontent(["random_transaction_id"], accounts[0].Address)
        assert len(result) == 0

    def test_getcontent_detail_check(self, builder):
        self.log.info("Test 2 - Getting content for very custom post")
        public_api = builder.node.public()
        account = builder.moderators[0]

        post = ContentPostPayload(
            language="es",
            message="Hasta la vista",
            caption="Fancy Caption",
            url="https://random.xyz",
            tags=["society"],
            images=["fancy_image"],
        )
        add_content(builder.node, account, post)

        self.log.info("Check - getcontent returns correct data for custom post")
        contents = public_api.getcontents(account.Address)
        post_id = contents[0]["txid"]
        assert len(contents) == 1
        data = public_api.getcontent([post_id], account.Address)
        assert len(data) == 1

        post = data[0]
        assert post["c"] == "Fancy Caption"
        assert post["m"] == "Hasta la vista"
        assert post["comments"] == 0
        assert post["lastComment"] is None
        assert post["scoreCnt"] == "0"
        assert post["scoreSum"] == "0"
        assert post["i"] == ["fancy_image"]
        assert post["l"] == "es"
        assert post["t"] == ["society"]
        assert post["u"] == "https://random.xyz"
        assert post["txid"] == post_id
        assert isinstance(post["userprofile"], dict)
        assert post["userprofile"]["address"] == account.Address

        boost_post(builder.node, builder.accounts[0], post_id)
        boost_post(builder.node, account, post_id)

        like_post(builder.node, account, builder.accounts[0], post_id, 5)
        like_post(builder.node, account, builder.accounts[1], post_id, 3)

        comment_post(builder.node, builder.accounts[1], post_id, "first comment")
        comment_post(builder.node, builder.accounts[2], post_id, "second comment")

        self.log.info("Check - getcontent returns correct data after boost, like, comment")
        data = public_api.getcontent([post_id], account.Address)
        assert len(data) == 1

        post = data[0]
        assert post["comments"] == 2
        assert post["scoreCnt"] == "2"
        assert post["scoreSum"] == "8"
        assert isinstance(post["lastComment"], dict)
        assert post["lastComment"]["msg"] == "second comment"

    def test_getcontentsstatistic(self, builder):
        self.log.info("Test 3 - Getting contents statistic")
        public_api = builder.node.public()

        self.log.info("Check - getcontentsstatistic without parameters is empty")
        statistic = public_api.getcontentsstatistic()
        assert isinstance(statistic, list)
        assert len(statistic) == 0

        self.log.info("Check - getcontentsstatistic for account with activity")
        statistic = public_api.getcontentsstatistic([builder.moderators[0].Address])
        assert isinstance(statistic, list)
        assert len(statistic) == 1
        assert isinstance(statistic[0], dict)
        assert statistic[0]["address"] == builder.moderators[0].Address
        assert statistic[0]["countLikers"] == 2
        assert statistic[0]["scoreCnt"] == 2
        assert statistic[0]["scoreSum"] == 8

    def test_getrandomcontents(self, builder):
        self.log.info("Test 4 - Getting random content")
        public_api = builder.node.public()

        self.log.info("Check - Get random contents returns empty list")
        contents = public_api.getrandomcontents()
        assert isinstance(contents, list)
        assert len(contents) == 0

        self.log.info("Check - Random content returns content for language=es")
        contents = public_api.getrandomcontents("es")
        assert isinstance(contents, list)
        assert len(contents) == 1
        assert contents[0]["address"] == builder.moderators[0].Address
        assert contents[0]["m"] == "Hasta la vista"

    def test_getcontentactions(self, builder):
        self.log.info("Test 5 - Getting content actions")
        public_api = builder.node.public()

        self.log.info("Check - Get content actions for the post with activity")
        content_id = builder.moderators[0].content[0]
        actions = public_api.getcontentactions(content_id)
        assert isinstance(actions, dict)

        self.log.info("Check - Content boosts")
        boosts = actions["boosts"]
        assert isinstance(boosts, list)
        assert len(boosts) == 2
        for boost in boosts:
            assert boost["address"] in [builder.accounts[0].Address, builder.moderators[0].Address]
            assert boost["value"] == 1
            if boost["address"] == builder.moderators[0].Address:
                assert boost["reputation"] == "20"
            else:
                assert boost.get("reputation") is None

        self.log.info("Check - Content donations")
        donations = actions["donations"]
        assert isinstance(donations, list)
        assert len(donations) == 0

        self.log.info("Check - Content scores")
        scores = actions["scores"]
        assert isinstance(scores, list)
        assert len(scores) == 2
        for score in scores:
            assert score["address"] in [builder.accounts[0].Address, builder.accounts[1].Address]
            if score["address"] == builder.accounts[0].Address:
                assert score["value"] == 5
            else:
                assert score["value"] == 3

    def test_getevents(self, builder):
        self.log.info("Test 6 - Getting events")
        public_api = builder.node.public()

        self.log.info("Check - Getting events for empty account")
        events = public_api.getevents(builder.moderators[1].Address)
        assert isinstance(events, list)
        assert len(events) == 1
        assert isinstance(events[0], dict)
        for key in ["blockNum", "height", "inputs", "outputs", "txType", "type"]:
            assert key in events[0].keys()
        assert events[0]["txType"] == 1
        assert events[0]["type"] == "money"
        assert isinstance(events[0]["inputs"], list)
        assert isinstance(events[0]["outputs"], list)

        self.log.info("Check - Getting events for not empty account")
        events = public_api.getevents(builder.moderators[0].Address)
        assert isinstance(events, list)
        assert len(events) == 8

    def test_getactivities(self, builder):
        self.log.info("Test 7 - Getting activities")
        public_api = builder.node.public()

        self.log.info("Check - Getting activities for empty account")
        activities = public_api.getactivities(builder.moderators[1].Address)
        assert isinstance(activities, list)
        assert len(activities) == 0

        self.log.info("Check - Getting activities for not empty account")
        activities = public_api.getactivities(builder.moderators[0].Address)
        assert isinstance(activities, list)
        assert len(activities) == 1

    def test_getnotifications(self, builder):
        self.log.info("Test 8 - Getting notifications")
        public_api = builder.node.public()

        self.log.info("Check - Getting notifications from height 1")
        notifications = public_api.getnotifications(1)
        assert isinstance(notifications, dict)
        data = notifications.get("data")
        assert isinstance(data, list)
        assert len(data) == 1
        notifiers = notifications.get("notifiers")
        assert isinstance(notifiers, dict)
        assert len(notifiers.keys()) == 1

    def test_getnotificationssummary(self, builder):
        self.log.info("Test 9 - Getting notifications summary")
        public_api = builder.node.public()

        self.log.info("Check - Notifications summary for empty account")
        summary = public_api.getnotificationssummary([builder.moderators[1].Address])
        assert isinstance(summary, dict)
        assert len(summary) == 0

        self.log.info("Check - Notifications summary for account with activity")
        summary = public_api.getnotificationssummary([builder.moderators[0].Address])
        assert isinstance(summary, dict)
        assert len(summary) == 1
        data = summary.get(builder.moderators[0].Address)
        assert data == {"comment": 2, "contentscore": 2}

        self.log.info("Check - Notifications summary for all accounts")
        post_id = builder.accounts[1].content[0]
        comment_post(builder.node, builder.accounts[0], post_id)
        addresses = [acc.Address for acc in builder.accounts + builder.moderators]
        summary = public_api.getnotificationssummary(addresses)
        assert isinstance(summary, dict)
        assert len(summary) == 2
        data = summary.get(builder.moderators[0].Address)
        assert data == {"comment": 2, "contentscore": 2}
        data = summary.get(builder.accounts[1].Address)
        assert data == {"comment": 1}

    def run_test(self):
        node = self.nodes[0]
        builder = ChainBuilder(node, self.log)
        builder.build_init(accounts_num=3, moderators_num=2)
        builder.register_accounts()
        self.generate_posts(builder)

        self.test_getcontent(builder)
        self.test_getcontent_detail_check(builder)
        self.test_getcontentsstatistic(builder)
        self.test_getrandomcontents(builder)
        self.test_getcontentactions(builder)
        self.test_getevents(builder)
        self.test_getactivities(builder)
        self.test_getnotifications(builder)
        self.test_getnotificationssummary(builder)


if __name__ == "__main__":
    ContentTest().main()
