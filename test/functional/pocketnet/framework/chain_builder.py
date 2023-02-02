# Copyright (c) 2023 The Pocketnet developers
# Distributed under the Apache 2.0 software license, see the accompanying
# https://www.apache.org/licenses/LICENSE-2.0

import logging
import random

from framework.helpers import (
    boost_post,
    generate_account_blockings,
    generate_accounts,
    generate_comments,
    generate_likes,
    generate_posts,
    generate_subscription,
    register_accounts,
)


class ChainBuilder:
    ACCOUNT_NUM = 10
    FIRST_COINBASE_BLOCKS = 1020

    def __init__(self, node, logger=None):
        self._node = node
        self.log = logger or logging.getLogger("ChainBuilder")
        self.log.setLevel(logging.DEBUG)

    def build_full(self):
        self.build_init()
        self.register_accounts()
        # self.generate_transfers()
        self.generate_posts()
        self.boost_posts()
        self.generate_comments()
        self.generate_likes()
        self.generate_subscriptions()
        self.generate_accounts_blockings()
        self.log.info("Done generating blockchain activities.")

    def build_init(self):
        self.log.info("Generate general node address")
        self._node_address = self._node.getnewaddress()

        self.log.info(f"Generate first coinbase {self.FIRST_COINBASE_BLOCKS} blocks")
        self._node.generatetoaddress(self.FIRST_COINBASE_BLOCKS, self.node_address)

        info = self._node.public().getaddressinfo(self.node_address)
        self.log.info(f"Node balance: {info}")

        self.log.info(f"Generate {self.ACCOUNT_NUM} account addresses")
        self._accounts = generate_accounts(self._node, self.node_address, self.ACCOUNT_NUM)

        self.log.info(f"Generate {self.ACCOUNT_NUM} moderator addresses")
        self._moders = generate_accounts(self._node, self.node_address, self.ACCOUNT_NUM, is_moderator=True)

    def register_accounts(self):
        self.log.info("Register accounts")
        register_accounts(self._node, self.accounts)
        register_accounts(self._node, self.moderators)

    def generate_transfers(self):
        self.log.info("Generate accounts transfers")
        raise NotImplementedError

    def generate_posts(self):
        self.log.info("Generate accounts posts")
        for account in self.accounts:
            generate_posts(self._node, account)

    def boost_posts(self):
        self.log.info("Boost random posts of first 5 accounts")
        for account in self.accounts[:5]:
            try:
                post_id = random.choice(account.content)
                boost_post(self._node, account, post_id)
            except IndexError:
                self.log.info("No content found for boosting")

    def generate_comments(self):
        self.log.info("Generate accounts posts comments")
        for acc1, acc2 in zip(self.accounts, self.accounts[::-1]):
            generate_comments(self._node, acc1, acc2)

    def generate_likes(self):
        self.log.info("Generate posts and comments likes")
        mixed = random.sample(self.accounts, len(self.accounts))
        # 3 accounts likes and dislikes another 7
        for acc1 in mixed[:3]:
            for acc2 in mixed[3:]:
                generate_likes(self._node, acc1, acc2)

    def generate_subscriptions(self):
        self.log.info("Generate account subscriptions")
        mixed = random.sample(self.accounts, 5)
        # 2 accounts subscribe to other 3
        for acc1 in mixed[:2]:
            for acc2 in mixed[2:]:
                generate_subscription(self._node, acc1, acc2)

    def generate_accounts_blockings(self):
        self.log.info("Generate accounts blacklists")
        mixed = random.sample(self.accounts, 5)
        # 2 accounts block other 3
        for acc1 in mixed[:2]:
            for acc2 in mixed[2:]:
                generate_account_blockings(self._node, acc1, acc2)

    @property
    def accounts(self):
        return self._accounts

    @property
    def moderators(self):
        return self._moders

    @property
    def node_address(self):
        return self._node_address
