#!/usr/bin/env python3
# Copyright (c) 2023 The Pocketnet developers
# Distributed under the Apache 2.0 software license, see the accompanying
# https://www.apache.org/licenses/LICENSE-2.0
"""
This test covers the explorer functionality.
Launch this from 'test/functional/pocketnet' directory or via 
test_runner.py
"""

import sys
import pathlib

sys.path.insert(0, str(pathlib.Path(__file__).parent.parent))

from pocketnet.framework.chain_builder import ChainBuilder
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
from test_framework.util import assert_raises_rpc_error


class ExplorerTest(PocketcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 2
        self.extra_args = [["-debug=consensus"], ["-debug=consensus"]]

    def set_up(self, builder):
        self.log.info("SetUp - Preparing the test data for explorer")
        add_content(builder.node, builder.accounts[0], ContentPostPayload(language="ru"))
        like_post(builder.node, builder.accounts[0], builder.accounts[1], builder.accounts[0].content[0], 5)
        comment_post(builder.node, builder.accounts[1], builder.accounts[0].content[0])
        like_comment(builder.node, builder.accounts[0], builder.accounts[1], builder.accounts[1].comment[0])
        generate_subscription(builder.node, builder.accounts[1], builder.accounts[0])
        generate_subscription(builder.node, builder.accounts[2], builder.accounts[0])
        generate_subscription(builder.node, builder.accounts[0], builder.accounts[4])
        generate_account_blockings(builder.node, builder.accounts[0], builder.accounts[3])
        generate_account_blockings(builder.node, builder.accounts[4], builder.accounts[0])
        builder.node.stakeblock(10)

    def test_getstatistictransactions(self, builder):
        self.log.info("Test 1 - Get statistic transactions")
        public_api = builder.node.public()

        result = public_api.getstatistictransactions()
        assert isinstance(result, dict)
        assert len(result) == 1

        values = list(result.values())[0]
        assert values["1"] == 6
        assert values["100"] == 12
        assert values["200"] == 1
        assert values["204"] == 1
        assert values["300"] == 1
        assert values["301"] == 1
        assert values["302"] == 3

    def test_getstatisticbyhours(self, builder):
        self.log.info("Test 2 - Get statistic by hours")
        public_api = builder.node.public()

        result = public_api.getstatisticbyhours()
        assert isinstance(result, dict)
        assert result == {'17': {'1': 6, '100': 12}}

    def test_getstatisticbydays(self, builder):
        self.log.info("Test 3 - Get statistic by days")
        public_api = builder.node.public()

        result = public_api.getstatisticbydays()
        assert isinstance(result, dict)
        assert result == {}

    def test_getstatisticcontentbyhours(self, builder):
        self.log.info("Test 4 - Get statistic content by hours")
        public_api = builder.node.public()

        result = public_api.getstatisticcontentbyhours()
        assert isinstance(result, dict)
        assert result['17'] == 6
        assert result['18'] == 6

    def test_getstatisticcontentbydays(self, builder):
        self.log.info("Test 5 - Get statistic content by days")
        public_api = builder.node.public()

        result = public_api.getstatisticcontentbydays()
        assert isinstance(result, dict)
        assert result['0'] == 6

    def test_getaddressinfo(self, builder):
        self.log.info("Test 6 - Get address info")
        public_api = builder.node.public()
        account = builder.accounts[0]

        result = public_api.getaddressinfo(account.Address)
        assert isinstance(result, dict)
        assert "balance" in result
        assert "lastChange" in result

        assert_raises_rpc_error(-5, "Invalid address", public_api.getaddressinfo, "dummy_hash")

    def test_getblocks(self, builder):
        self.log.info("Test 7 - Get blocks")
        public_api = builder.node.public()

        last_blocks = public_api.getlastblocks()
        assert isinstance(last_blocks, list)
        assert len(last_blocks) == 10

        last_blocks = public_api.getlastblocks(20)
        assert len(last_blocks) == 20

        last_block = last_blocks[0]
        assert isinstance(last_block, dict)
        for key in ["hash", "height", "ntx", "time"]:
            assert key in last_block

        block = public_api.getcompactblock(last_block["hash"])
        assert isinstance(block, dict)
        for key in ["bits", "difficulty", "hash", "height", "merkleroot", "nTx", "nexthash", "prevhash", "time"]:
            assert key in block

        assert_raises_rpc_error(-5, "Block not found", public_api.getcompactblock, "dummy_hash")

    def test_searchbyhash(self, builder):
        self.log.info("Test 8 - Search for hash")
        public_api = builder.node.public()

        last_blocks = public_api.getlastblocks()
        last_block = last_blocks[0]

        hashes = [last_block["hash"], builder.accounts[0].Address, builder.accounts[0].content[0], "dummy_hash"]
        types = ["block", "address", "transaction", "notfound"]

        for hash_, type_ in zip(hashes, types):
            result = public_api.searchbyhash(hash_)
            assert isinstance(result, dict)
            assert result["type"] == type_

    def test_gettransactions(self, builder):
        self.log.info("Test 9 - Get transactions")
        public_api = builder.node.public()
        account = builder.accounts[0]

        self.log.info("Check - get single transaction")
        result = public_api.gettransactions(account.content[0])
        assert isinstance(result, list)
        assert len(result) == 1

        transaction = result[0]
        assert transaction["type"] == 200
        assert transaction["txid"] == account.content[0]
        self.check_transaction(transaction)

        self.log.info("Check - get multiple transactions")
        hashes = [account.content[0], builder.accounts[1].comment[0]]
        types = [200, 204]
        result = public_api.gettransactions(hashes)

        assert isinstance(result, list)
        assert len(result) == 2

        for transaction in result:
            assert transaction["type"] in types
            assert transaction["txid"] in hashes
            self.check_transaction(transaction)

        self.log.info("Check - get not existing transaction")
        assert_raises_rpc_error(-32603, None, public_api.gettransactions, "dummy_hash")


    def test_getaddresstransactions(self, builder):
        self.log.info("Test 10 - Get address transactions")
        public_api = builder.node.public()
        account = builder.accounts[0]

        self.log.info("Check - get address transactions")
        result = public_api.getaddresstransactions(account.Address)
        assert isinstance(result, list)
        assert len(result) == 7

        for transaction in result:
            self.check_transaction(transaction)

        self.log.info("Check - get not existing address transactions")
        result = public_api.getaddresstransactions("dummy_hash")
        assert isinstance(result, list)
        assert len(result) == 0

    def test_getblocktransactions(self, builder):
        self.log.info("Test 11 - Get block transactions")
        public_api = builder.node.public()

        last_blocks = public_api.getlastblocks(11)
        last_block = last_blocks[0]

        self.log.info("Check - stake block does not have transactions")
        result = public_api.getblocktransactions(last_block["hash"])
        assert isinstance(result, list)
        assert len(result) == 1
        self.check_transaction(result[0])

        self.log.info("Check - activity block has transactions")
        first_block = last_blocks[-1]
        result = public_api.getblocktransactions(first_block["hash"])
        assert isinstance(result, list)
        assert len(result) == 0

        self.log.info("Check - dummy hash does not have transactions")
        result = public_api.getblocktransactions("dummy_hash")
        assert isinstance(result, list)
        assert len(result) == 0

    def test_getbalancehistory(self, builder):
        self.log.info("Test 12 - Get balance history")
        public_api = builder.node.public()
        account = builder.accounts[0]

        result = public_api.getbalancehistory(account.Address)
        assert isinstance(result, list)
        assert len(result) == 7

        for item in result:
            assert isinstance(item, list)
            assert len(item) == 2

    def test_sync_nodes(self, builder):
        self.log.info("Check - sync nodes")
        self.connect_nodes(1, 2)
        self.sync_all()

    def check_transaction(self, transaction):
        for key in ["blockHash", "height", "nTime", "txid", "type", "vin", "vout"]:
            assert key in transaction

    def run_test(self):
        node = self.nodes[0]
        builder = ChainBuilder(node, self.log)
        builder.build_init(accounts_num=5, moderators_num=1)
        builder.register_accounts()
        builder.register_accounts()

        self.set_up(builder)
        self.test_getstatistictransactions(builder)
        self.test_getstatisticbyhours(builder)
        self.test_getstatisticbydays(builder)
        self.test_getstatisticcontentbyhours(builder)
        self.test_getstatisticcontentbydays(builder)
        self.test_getaddressinfo(builder)
        self.test_getblocks(builder)
        self.test_searchbyhash(builder)
        self.test_gettransactions(builder)
        self.test_getaddresstransactions(builder)
        self.test_getblocktransactions(builder)
        # self.test_getbalancehistory(builder) TODO Implement this method in node
        self.test_sync_nodes(builder)


if __name__ == "__main__":
    ExplorerTest().main()
