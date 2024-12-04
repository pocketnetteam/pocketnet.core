#!/usr/bin/env python3
# Copyright (c) 2017-2022 The Bitcoin Core developers
# Copyright (c) 2018-2023 The Pocketnet Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""
An ContentCollections functional test
Launch this with command from 'test/functional/pocketnet' directory
"""

import sys
import pathlib

sys.path.insert(0, str(pathlib.Path(__file__).parent.parent))

# Imports should be in PEP8 ordering (std library first, then third party
# libraries then local imports).
from collections import defaultdict

# Avoid wildcard * imports
from test_framework.blocktools import create_block, create_coinbase
from test_framework.messages import CInv, MSG_BLOCK
from test_framework.test_framework import PocketcoinTestFramework
from test_framework.util import assert_equal, get_rpc_proxy, rpc_url, assert_raises_rpc_error

# Pocketnet framework
from framework.models import *


class ContentCollectionsTest(PocketcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        # TODO : запустить несколько нод для проверки синхронизации и спорных транзакций
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
        node.generatetoaddress(1020, nodeAddress)

        self.log.info("Node balance: %s", node.public().getaddressinfo(nodeAddress))

        # ---------------------------------------------------------------------------------

        self.log.info("Generate account addresses")
        nAddrs = 2
        accounts = []
        for i in range(nAddrs):
            acc = node.public().generateaddress()
            accounts.append(Account(acc["address"], acc["privkey"]))
            node.sendtoaddress(address=accounts[i].Address, amount=10, destaddress=nodeAddress)
        self.log.info("Account adddresses generated = " + str(len(accounts)))

        node.stakeblock(15)

        self.log.info("Check balance")
        for i in range(nAddrs):
            assert node.public().getaddressinfo(address=accounts[i].Address)["balance"] == 10

        # ---------------------------------------------------------------------------------

        # TODO (o1q):
        self.log.info("Check collections creation from not registered account")
        # assert_raises_rpc_error(1, None, pubGenTx, accounts[0], ContentCollectionsPayload())

        # ---------------------------------------------------------------------------------

        self.log.info("Register accounts")
        hashes = []
        for i in range(nAddrs):
            hashes.append(
                pubGenTx(accounts[i], AccountPayload(f"name{i}", "image", "en", "about", "s", "b", "pubkey"), 100)
            )

        node.stakeblock(15)

        for i in range(nAddrs):
            assert node.public().getuserprofile(accounts[i].Address)[0]["hash"] == hashes[i]

        # ---------------------------------------------------------------------------------
        self.log.info("Test 1 - Collections creation")

        contents_posts = []
        contents_videos = []
        contents_articles = []
        contents_streams = []
        contents_audios = []
        contents_collections = []

        contents_posts.append(pubGenTx(accounts[0], ContentPostPayload()))
        contents_videos.append(pubGenTx(accounts[0], ContentVideoPayload()))
        contents_articles.append(pubGenTx(accounts[0], ContentArticlePayload()))
        contents_streams.append(pubGenTx(accounts[0], ContentStreamPayload()))
        contents_audios.append(pubGenTx(accounts[0], ContentAudioPayload()))

        node.stakeblock(1)

        # creating collections
        contents_collections.append(pubGenTx(accounts[0], ContentCollectionsPayload("en", "caption", "image", 200, contents_posts, "")))
        contents_collections.append(pubGenTx(accounts[0], ContentCollectionsPayload("en", "caption", "image", 201, contents_videos, "")))
        contents_collections.append(pubGenTx(accounts[0], ContentCollectionsPayload("en", "caption", "image", 202, contents_articles, "")))
        contents_collections.append(pubGenTx(accounts[0], ContentCollectionsPayload("en", "caption", "image", 209, contents_streams, "")))
        contents_collections.append(pubGenTx(accounts[0], ContentCollectionsPayload("en", "caption", "image", 210, contents_audios, "")))
        node.stakeblock(1)

        # different content types
        pubGenTx(accounts[0], ContentCollectionsPayload("en", "caption", "image", 201, contents_posts, ""))
        pubGenTx(accounts[0], ContentCollectionsPayload("en", "caption", "image", 202, contents_posts, ""))
        pubGenTx(accounts[0], ContentCollectionsPayload("en", "caption", "image", 209, contents_posts, ""))
        pubGenTx(accounts[0], ContentCollectionsPayload("en", "caption", "image", 210, contents_posts, ""))
        pubGenTx(accounts[0], ContentCollectionsPayload("en", "caption", "image", 200, contents_videos, ""))
        pubGenTx(accounts[0], ContentCollectionsPayload("en", "caption", "image", 202, contents_videos, ""))
        pubGenTx(accounts[0], ContentCollectionsPayload("en", "caption", "image", 209, contents_videos, ""))
        pubGenTx(accounts[0], ContentCollectionsPayload("en", "caption", "image", 210, contents_videos, ""))
        pubGenTx(accounts[0], ContentCollectionsPayload("en", "caption", "image", 200, contents_articles, ""))
        pubGenTx(accounts[0], ContentCollectionsPayload("en", "caption", "image", 201, contents_articles, ""))
        pubGenTx(accounts[0], ContentCollectionsPayload("en", "caption", "image", 209, contents_articles, ""))
        pubGenTx(accounts[0], ContentCollectionsPayload("en", "caption", "image", 210, contents_articles, ""))
        pubGenTx(accounts[0], ContentCollectionsPayload("en", "caption", "image", 200, contents_streams, ""))
        pubGenTx(accounts[0], ContentCollectionsPayload("en", "caption", "image", 201, contents_streams, ""))
        pubGenTx(accounts[0], ContentCollectionsPayload("en", "caption", "image", 202, contents_streams, ""))
        pubGenTx(accounts[0], ContentCollectionsPayload("en", "caption", "image", 210, contents_streams, ""))
        pubGenTx(accounts[0], ContentCollectionsPayload("en", "caption", "image", 200, contents_audios, ""))
        pubGenTx(accounts[0], ContentCollectionsPayload("en", "caption", "image", 201, contents_audios, ""))
        pubGenTx(accounts[0], ContentCollectionsPayload("en", "caption", "image", 202, contents_audios, ""))
        pubGenTx(accounts[0], ContentCollectionsPayload("en", "caption", "image", 209, contents_audios, ""))

        contents_different = []
        contents_different.append(contents_posts[0])
        contents_different.append(contents_videos[0])
        contents_different.append(contents_articles[0])
        contents_different.append(contents_streams[0])
        contents_different.append(contents_audios[0])
        pubGenTx(accounts[0], ContentCollectionsPayload("en", "caption", "image", 200, contents_different, ""))

        node.stakeblock(1)
        # ---------------------------------------------------------------------------------
        self.log.info("Test 2 - Collection editing")

        # Do not allow change content type (from post)
        pubGenTx(accounts[0], ContentCollectionsPayload("en", "caption", "image", 201, contents_posts, contents_collections[0]))
        pubGenTx(accounts[0], ContentCollectionsPayload("en", "caption", "image", 202, contents_audios, contents_collections[1]))
        pubGenTx(accounts[0], ContentCollectionsPayload("en", "caption", "image", 209, contents_streams, contents_collections[2]))
        pubGenTx(accounts[0], ContentCollectionsPayload("en", "caption", "image", 210, contents_articles, contents_collections[3]))
        pubGenTx(accounts[0], ContentCollectionsPayload("en", "caption", "image", 200, contents_videos, contents_collections[4]))
        node.stakeblock(1)

        # Add additional content to each collection
        contents_posts.append(pubGenTx(accounts[0], ContentPostPayload()))
        contents_videos.append(pubGenTx(accounts[0], ContentVideoPayload()))
        contents_articles.append(pubGenTx(accounts[0], ContentArticlePayload()))
        contents_streams.append(pubGenTx(accounts[0], ContentStreamPayload()))
        contents_audios.append(pubGenTx(accounts[0], ContentAudioPayload()))
        node.stakeblock(1)

        # Do not allow edit foreign collection
        assert_raises_rpc_error(
            27,
            None,
            pubGenTx,
            accounts[1],
            ContentCollectionsPayload("en", "caption", "image", 200, contents_posts, contents_collections[0]),
        )

        # edit collection with posts
        pubGenTx(
            accounts[0],
            ContentCollectionsPayload("en", "caption", "image", 200, contents_posts, contents_collections[0]),
        )
        # edit collection with videos
        pubGenTx(
            accounts[0],
            ContentCollectionsPayload("en", "caption", "image", 201, contents_videos, contents_collections[1]),
        )
        # edit collection with articles
        pubGenTx(
            accounts[0],
            ContentCollectionsPayload("en", "caption", "image", 202, contents_articles, contents_collections[2]),
        )
        # edit collection with streams
        pubGenTx(
            accounts[0],
            ContentCollectionsPayload("en", "caption", "image", 209, contents_streams, contents_collections[3]),
        )
        # edit collection with audios
        pubGenTx(
            accounts[0],
            ContentCollectionsPayload("en", "caption", "image", 210, contents_audios, contents_collections[4]),
        )

        node.stakeblock(1)
        # ---------------------------------------------------------------------------------
        self.log.info("Test 3 - Collection actions")

        # Do now allow score
        assert_raises_rpc_error(12, None, pubGenTx, accounts[1], ScoreContentPayload(contents_collections[0], 1))
        # Do now allow comment
        assert_raises_rpc_error(12, None, pubGenTx, accounts[1], CommentPayload(contents_collections[0]))
        # Do now allow boost
        assert_raises_rpc_error(12, None, pubGenTx, accounts[1], BoostPayload(contents_collections[0]))

        node.stakeblock(1)
        # ---------------------------------------------------------------------------------
        self.log.info("Test 4 - Collection deleting")

        pubGenTx(accounts[0], ContentDeletePayload(contents_collections[0]))

        # Do not allow double deleting
        assert_raises_rpc_error(47, None, pubGenTx, accounts[0], ContentDeletePayload(contents_collections[0]))
        # Do not allow deleting foreign collection
        assert_raises_rpc_error(46, None, pubGenTx, accounts[1], ContentDeletePayload(contents_collections[1]))

        node.stakeblock(1)
        # ---------------------------------------------------------------------------------
        self.log.info("Test 5 - Collection limits")
        node.stakeblock(1)

        contents = []
        contents.append(pubGenTx(accounts[0], ContentPostPayload()))
        contents.append(pubGenTx(accounts[0], ContentPostPayload()))
        contents.append(pubGenTx(accounts[0], ContentPostPayload()))
        contents.append(pubGenTx(accounts[0], ContentPostPayload()))
        contents.append(pubGenTx(accounts[0], ContentPostPayload()))
        contents.append(pubGenTx(accounts[0], ContentPostPayload()))
        contents.append(pubGenTx(accounts[0], ContentPostPayload()))
        contents.append(pubGenTx(accounts[0], ContentPostPayload()))
        contents.append(pubGenTx(accounts[0], ContentPostPayload()))
        contents.append(pubGenTx(accounts[0], ContentPostPayload()))
        node.stakeblock(1)
        collection = pubGenTx(accounts[0], ContentCollectionsPayload("en", "caption", "image", 200, contents, ""))

        # maximum contents in collection
        contents.append(pubGenTx(accounts[0], ContentPostPayload()))
        assert_raises_rpc_error(
            11, None, pubGenTx, accounts[0], ContentCollectionsPayload("en", "caption", "image", 200, contents, "")
        )
        node.stakeblock(1)

        # maximum edits collection per day
        contents.pop(10)
        pubGenTx(accounts[0], ContentCollectionsPayload("en", "caption", "image", 200, contents, collection))
        node.stakeblock(1)
        pubGenTx(accounts[0], ContentCollectionsPayload("en", "caption", "image", 200, contents, collection))
        node.stakeblock(1)
        pubGenTx(accounts[0], ContentCollectionsPayload("en", "caption", "image", 200, contents, collection))
        node.stakeblock(1)
        pubGenTx(accounts[0], ContentCollectionsPayload("en", "caption", "image", 200, contents, collection))
        node.stakeblock(1)
        pubGenTx(accounts[0], ContentCollectionsPayload("en", "caption", "image", 200, contents, collection))
        node.stakeblock(1)
        assert_raises_rpc_error(
            26,
            None,
            pubGenTx,
            accounts[0],
            ContentCollectionsPayload("en", "caption", "image", 200, contents, collection),
        )
        node.stakeblock(100)
        pubGenTx(accounts[0], ContentCollectionsPayload("en", "caption", "image", 200, contents, collection))
        node.stakeblock(1)
        # ---------------------------------------------------------------------------------


if __name__ == "__main__":
    ContentCollectionsTest().main()
