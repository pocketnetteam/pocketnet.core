# Copyright (c) 2023 The Pocketnet developers
# Distributed under the Apache 2.0 software license, see the accompanying
# https://www.apache.org/licenses/LICENSE-2.0

import random

from framework.models import (
    Account,
    AccountPayload,
    BlockingPayload,
    BoostPayload,
    CommentPayload,
    ContentPostPayload,
    ContentVideoPayload,
    ContentArticlePayload,
    ContentStreamPayload,
    ContentAudioPayload,
    ScoreCommentPayload,
    ScoreContentPayload,
    SubscribePayload,
)


def generate_accounts(node, node_address, account_num, amount=10, is_moderator=False):
    accounts = []
    for i in range(account_num):
        acc = node.public().generateaddress()
        name = f"moderator{i}" if is_moderator else f"user{i}"
        accounts.append(Account(acc["address"], acc["privkey"], name))
        node.sendtoaddress(address=accounts[i].Address, amount=amount, destaddress=node_address)

    node.stakeblock(1)

    for account in accounts:
        address_info = node.public().getaddressinfo(address=account.Address)
        assert address_info["balance"] == amount

    return accounts


def add_content(node, account, payload):
    pub_gen_tx = node.public().generatetransaction
    account.content.append(pub_gen_tx(account, payload))


def generate_posts(node, account):
    add_content(node, account, ContentPostPayload())
    add_content(node, account, ContentVideoPayload())
    add_content(node, account, ContentArticlePayload())
    add_content(node, account, ContentStreamPayload())
    add_content(node, account, ContentAudioPayload())
    node.stakeblock(1)


def boost_post(node, account, post_id):
    pub_gen_tx = node.public().generatetransaction
    pub_gen_tx(account, BoostPayload(post_id))
    node.stakeblock(1)


def generate_comments(node, account1, account2):
    pub_gen_tx = node.public().generatetransaction
    for post_id in account1.content:
        comment_id = pub_gen_tx(account2, CommentPayload(post_id))
        account2.comment.append(comment_id)
        node.stakeblock(1)
        answer_id = pub_gen_tx(account1, CommentPayload(post_id, comment_id))
        account1.comment.append(answer_id)
        node.stakeblock(1)


def generate_likes(node, account1, account2):
    pub_gen_tx = node.public().generatetransaction
    for post_id in account1.content:
        score = random.randint(3, 5)
        pub_gen_tx(account2, ScoreContentPayload(post_id, score, account1.Address))
        node.stakeblock(1)

    for comment_id in account2.comment:
        # TODO: enable dislikes (need high reputation for that)
        # score = random.randint(0, 1)
        score = 1
        pub_gen_tx(account1, ScoreCommentPayload(comment_id, score, account2.Address))
        node.stakeblock(1)


def generate_subscription(node, account1, account2):
    pub_gen_tx = node.public().generatetransaction
    account1.subscribes.append(account2.Address)
    pub_gen_tx(account1, SubscribePayload(account2.Address))
    node.stakeblock(1)


def generate_account_blockings(node, account1, account2):
    pub_gen_tx = node.public().generatetransaction
    account1.blockings.append(account2.Address)
    pub_gen_tx(account1, BlockingPayload(account2.Address))
    node.stakeblock(1)


def register_accounts(node, accounts, out_count=50, confirmations=0):
    hashes = []
    pub_gen_tx = node.public().generatetransaction
    for account in accounts:
        payload = AccountPayload(account.Name, "image", "en", "about", "s", "b", "pubkey")
        _hash = pub_gen_tx(account, payload, out_count, confirmations)
        hashes.append(_hash)

    node.stakeblock(15)

    for i, account in enumerate(accounts):
        assert node.public().getuserprofile(account.Address)[0]["hash"] == hashes[i]


def rollback_node(node, blocks, logger=None):
    if logger is not None:
        logger.warning(f"Rolling back node, number of blocks: {blocks}")

    for _ in range(blocks):
        node.invalidateblock(node.getbestblockhash())
