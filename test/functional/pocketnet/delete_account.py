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

sys.path.insert(0, str(pathlib.Path(__file__).parent.parent))

# Avoid wildcard * imports
from test_framework.test_framework import PocketcoinTestFramework
from test_framework.util import assert_raises_rpc_error

# Pocketnet framework
from framework.chain_builder import ChainBuilder
from framework.helpers import register_accounts
from framework.models import *


class AccountDeleteTest(PocketcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        # TODO: run multiple and check the synchronization and conflicting transactions
        self.num_nodes = 1
        self.extra_args = [["-debug=consensus"]]

    def run_test(self):
        node = self.nodes[0]
        builder = ChainBuilder(node, self.log)
        builder.build_init()

        pubGenTx = node.public().generatetransaction
        accounts = builder.accounts

        # ---------------------------------------------------------------------------------

        self.log.info("Check delete not registered account")
        node.stakeblock(15)
        assert_raises_rpc_error(1, None, pubGenTx, accounts[0], AccountDeletePayload())

        # ---------------------------------------------------------------------------------

        builder.register_accounts()

        # ---------------------------------------------------------------------------------
        self.log.info("Prepare content")

        accounts[0].content.append(pubGenTx(accounts[0], ContentPostPayload()))
        accounts[0].content.append(pubGenTx(accounts[0], ContentVideoPayload()))
        accounts[0].content.append(pubGenTx(accounts[0], ContentArticlePayload()))
        accounts[0].content.append(pubGenTx(accounts[0], ContentStreamPayload()))
        accounts[0].content.append(pubGenTx(accounts[0], ContentAudioPayload()))
        node.stakeblock(1)

        accounts[0].comment.append(pubGenTx(accounts[0], CommentPayload(accounts[0].content[0])))
        node.stakeblock(1)

        # ---------------------------------------------------------------------------------
        self.log.info("Test 1 - delete & other txs in mempool")

        # Delete Account 0
        pubGenTx(accounts[0], AccountDeletePayload())

        # Create content allowed in one block with delete transaction
        accounts[0].content.append(pubGenTx(accounts[0], ContentPostPayload()))
        accounts[0].content.append(pubGenTx(accounts[0], ContentVideoPayload()))
        accounts[0].content.append(pubGenTx(accounts[0], ContentArticlePayload()))
        accounts[0].content.append(pubGenTx(accounts[0], ContentStreamPayload()))
        accounts[0].content.append(pubGenTx(accounts[0], ContentAudioPayload()))

        pubGenTx(accounts[0], BoostPayload(accounts[0].content[0]))

        accounts[0].comment.append(pubGenTx(accounts[0], CommentPayload(accounts[0].content[0])))
        accounts[0].comment.append(
            pubGenTx(
                accounts[0],
                CommentPayload(accounts[0].content[0], accounts[0].comment[0]),
            )
        )
        accounts[0].comment.append(
            pubGenTx(
                accounts[0],
                CommentPayload(
                    accounts[0].content[0],
                    accounts[0].comment[0],
                    accounts[0].comment[0],
                ),
            )
        )

        # Test general account transactions
        pubGenTx(accounts[0], AccountSettingPayload())
        assert_raises_rpc_error(
            ConsensusResult.ChangeInfoDoubleInMempool,
            None,
            pubGenTx,
            accounts[0],
            AccountPayload(accounts[0].Name),
        )
        assert_raises_rpc_error(
            ConsensusResult.ManyTransactions,
            None,
            pubGenTx,
            accounts[0],
            AccountDeletePayload(),
        )

        # Account0 subscribe and blockings another
        accounts[0].subscribes.append(accounts[1].Address)
        pubGenTx(accounts[0], SubscribePayload(accounts[1].Address))

        accounts[0].subscribes.append(accounts[2].Address)
        pubGenTx(accounts[0], SubscribePrivatePayload(accounts[2].Address))

        accounts[0].blockings.append(accounts[3].Address)
        pubGenTx(accounts[0], BlockingPayload(accounts[3].Address))

        # ------------------------------------------------
        # Prepare another accounts
        accounts[1].content.append(pubGenTx(accounts[1], ContentPostPayload()))

        accounts[1].comment.append(pubGenTx(accounts[1], CommentPayload(accounts[0].content[0])))
        accounts[1].comment.append(
            pubGenTx(
                accounts[1],
                CommentPayload(accounts[0].content[0], accounts[0].comment[0]),
            )
        )
        accounts[1].comment.append(
            pubGenTx(
                accounts[1],
                CommentPayload(
                    accounts[0].content[0],
                    accounts[0].comment[0],
                    accounts[0].comment[0],
                ),
            )
        )

        # Account1 subscribed Account0
        accounts[1].subscribes.append(accounts[0].Address)
        pubGenTx(accounts[1], SubscribePayload(accounts[0].Address))

        # Account2 subscribed private Account0
        accounts[2].subscribes.append(accounts[0].Address)
        pubGenTx(accounts[2], SubscribePrivatePayload(accounts[0].Address))

        # Account3 block Account0
        accounts[3].blockings.append(accounts[0].Address)
        pubGenTx(accounts[3], BlockingPayload(accounts[0].Address))

        node.stakeblock(15)

        # ---------------------------------------------------------------------------------
        self.log.info("Test 2 - all txs from deleted account")

        assert_raises_rpc_error(
            ConsensusResult.NotRegistered,
            None,
            pubGenTx,
            accounts[0],
            AccountDeletePayload(),
        )
        assert_raises_rpc_error(
            ConsensusResult.NotRegistered,
            None,
            pubGenTx,
            accounts[0],
            AccountSettingPayload(),
        )

        assert_raises_rpc_error(
            ConsensusResult.NotRegistered,
            None,
            pubGenTx,
            accounts[0],
            ContentDeletePayload(accounts[0].content[0]),
        )
        assert_raises_rpc_error(
            ConsensusResult.NotRegistered,
            None,
            pubGenTx,
            accounts[0],
            ContentPostPayload(),
        )
        assert_raises_rpc_error(
            ConsensusResult.NotRegistered,
            None,
            pubGenTx,
            accounts[0],
            ContentVideoPayload(),
        )
        assert_raises_rpc_error(
            ConsensusResult.NotRegistered,
            None,
            pubGenTx,
            accounts[0],
            ContentArticlePayload(),
        )
        assert_raises_rpc_error(
            ConsensusResult.NotRegistered,
            None,
            pubGenTx,
            accounts[0],
            ContentStreamPayload(),
        )
        assert_raises_rpc_error(
            ConsensusResult.NotRegistered,
            None,
            pubGenTx,
            accounts[0],
            ContentAudioPayload(),
        )

        assert_raises_rpc_error(
            ConsensusResult.NotRegistered,
            None,
            pubGenTx,
            accounts[0],
            BoostPayload(accounts[0].content[0]),
        )
        assert_raises_rpc_error(
            ConsensusResult.NotRegistered,
            None,
            pubGenTx,
            accounts[0],
            ComplainPayload(accounts[1].content[0]),
        )
        assert_raises_rpc_error(
            ConsensusResult.NotRegistered,
            None,
            pubGenTx,
            accounts[0],
            ModFlagPayload(accounts[1].content[0], accounts[1].Address),
        )

        assert_raises_rpc_error(
            ConsensusResult.NotRegistered,
            None,
            pubGenTx,
            accounts[0],
            CommentPayload(accounts[0].content[0]),
        )
        assert_raises_rpc_error(
            ConsensusResult.NotRegistered,
            None,
            pubGenTx,
            accounts[0],
            CommentEditPayload(accounts[0].content[0], accounts[0].comment[0]),
        )
        assert_raises_rpc_error(
            ConsensusResult.NotRegistered,
            None,
            pubGenTx,
            accounts[0],
            CommentDeletePayload(accounts[0].content[0], accounts[0].comment[0]),
        )

        assert_raises_rpc_error(
            ConsensusResult.NotRegistered,
            None,
            pubGenTx,
            accounts[0],
            SubscribePayload(accounts[4].Address),
        )
        assert_raises_rpc_error(
            ConsensusResult.NotRegistered,
            None,
            pubGenTx,
            accounts[0],
            SubscribePrivatePayload(accounts[4].Address),
        )
        assert_raises_rpc_error(
            ConsensusResult.NotRegistered,
            None,
            pubGenTx,
            accounts[0],
            UnsubscribePayload(accounts[1].Address),
        )
        assert_raises_rpc_error(
            ConsensusResult.NotRegistered,
            None,
            pubGenTx,
            accounts[0],
            BlockingPayload(accounts[1].Address)
        )
        assert_raises_rpc_error(
            ConsensusResult.NotRegistered,
            None,
            pubGenTx,
            accounts[0],
            UnblockingPayload(accounts[3].Address),
        )

        assert_raises_rpc_error(
            ConsensusResult.NotRegistered,
            None,
            pubGenTx,
            accounts[0],
            ScoreContentPayload(accounts[1].content[0], 5, accounts[1].Address),
        )
        assert_raises_rpc_error(
            ConsensusResult.NotRegistered,
            None,
            pubGenTx,
            accounts[0],
            ScoreCommentPayload(accounts[1].comment[0], 1, accounts[1].Address),
        )

        node.stakeblock(15)

        # ---------------------------------------------------------------------------------
        self.log.info("Test 3 - actions from another under deleted accounts")

        pubGenTx(accounts[1], BoostPayload(accounts[0].content[0]))
        pubGenTx(accounts[1], ComplainPayload(accounts[0].content[0]))
        assert_raises_rpc_error(
            ConsensusResult.NotRegistered,
            None,
            pubGenTx,
            accounts[1],
            ModFlagPayload(accounts[0].content[0], accounts[0].Address),
        )

        pubGenTx(accounts[1], CommentPayload(accounts[0].content[0]))
        pubGenTx(
            accounts[1],
            CommentEditPayload(accounts[0].content[0], accounts[1].comment[0]),
        )
        pubGenTx(
            accounts[1],
            CommentDeletePayload(accounts[0].content[0], accounts[1].comment[1], accounts[0].comment[0]),
        )

        assert_raises_rpc_error(
            ConsensusResult.NotRegistered,
            None,
            pubGenTx,
            accounts[4],
            SubscribePayload(accounts[0].Address),
        )
        assert_raises_rpc_error(
            ConsensusResult.NotRegistered,
            None,
            pubGenTx,
            accounts[5],
            SubscribePrivatePayload(accounts[0].Address),
        )
        # Now unsubscribe deleted accounts allowed
        pubGenTx(
            accounts[1],
            UnsubscribePayload(accounts[0].Address),
        )
        pubGenTx(
            accounts[2],
            UnsubscribePayload(accounts[0].Address),
        )

        assert_raises_rpc_error(
            ConsensusResult.NotRegistered,
            None,
            pubGenTx,
            accounts[1],
            BlockingPayload(accounts[0].Address),
        )

        # Now unblocking deleted accounts allowed
        pubGenTx(
            accounts[3],
            UnblockingPayload(accounts[0].Address),
        )

        pubGenTx(
            accounts[1],
            ScoreContentPayload(accounts[0].content[0], 5, accounts[0].Address),
        )
        pubGenTx(
            accounts[1],
            ScoreCommentPayload(accounts[0].comment[0], 1, accounts[0].Address),
        )

        node.stakeblock(1)

        # ---------------------------------------------------------------------------------
        self.log.info("Test 4 - restore account and again delete")

        pubGenTx(
            accounts[0],
            AccountPayload(accounts[0].Name),
        )

        node.stakeblock(1)

        pubGenTx(
            accounts[0],
            AccountDeletePayload(),
        )      

        node.stakeblock(1)
        

if __name__ == "__main__":
    AccountDeleteTest().main()
