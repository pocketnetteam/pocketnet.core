#!/usr/bin/env python3
# Copyright (c) 2023 The Pocketnet developers
# Distributed under the Apache 2.0 software license, see the accompanying
# https://www.apache.org/licenses/LICENSE-2.0
"""
An account deletion functional test
Launch this with command from 'test/functional/pocketnet' directory
"""

import sys
import pathlib
import time

sys.path.insert(0, str(pathlib.Path(__file__).parent.parent))

# Avoid wildcard * imports
from test_framework.test_framework import PocketcoinTestFramework
from test_framework.util import assert_raises_rpc_error

# Pocketnet framework
from framework.chain_builder import ChainBuilder
from framework.helpers import register_accounts
from framework.models import *


class ContentDeleteTest(PocketcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        # TODO: run multiple and check the synchronization and conflicting transactions
        self.num_nodes = 1
        self.extra_args = [["-debug=consensus"]]

    def run_test(self):
        node = self.nodes[0]
        builder = ChainBuilder(node, self.log)
        builder.build_init(1, 1)

        pubGenTx = node.public().generatetransaction
        accounts = builder.accounts
        builder.register_accounts()

        # ---------------------------------------------------------------------------------
        self.log.info("Two transactions in mempool not allowed")

        accounts[0].content.append(pubGenTx(accounts[0], ContentArticlePayload()))
        node.stakeblock(1)
        
        deletePost = ContentDeletePayload()
        deletePost.content_tx = accounts[0].content[0]
        pubGenTx(accounts[0], deletePost)

        editPost = ContentArticlePayload()
        editPost.txedit = accounts[0].content[0]
        assert_raises_rpc_error(
            ConsensusResult.DoubleContentEdit,
            None,
            pubGenTx,
            accounts[0],
            editPost
        )

        node.stakeblock(1)

        # ---------------------------------------------------------------------------------
        
if __name__ == "__main__":
    ContentDeleteTest().main()
