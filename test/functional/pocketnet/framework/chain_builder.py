# Copyright (c) 2023 The Pocketnet developers
# Distributed under the Apache 2.0 software license, see the accompanying
# https://www.apache.org/licenses/LICENSE-2.0

import logging

from framework.helpers import generate_accounts, register_accounts


class ChainBuilder:
    ACCOUNTS = 10
    FIRST_COINBASE_BLOCKS = 1020

    def __init__(self, node, logger=None):
        self._node = node
        self.log = logger or logging.getLogger("ChainBuilder")
        self.log.setLevel(logging.DEBUG)

    def build(self):
        self.build()
        self.register_accounts()
        self.generate_transfers()
        self.generate_posts()
        self.generate_comments()
        self.generate_likes()
        self.generate_subscriptions()
        self.generate_blacklists()

    def build_init(self):
        self.log.info("Generate general node address")
        node_address = self._node.getnewaddress()

        self.log.info(f"Generate first coinbase {self.FIRST_COINBASE_BLOCKS} blocks")
        self._node.generatetoaddress(self.FIRST_COINBASE_BLOCKS, node_address)

        info = self._node.public().getaddressinfo(node_address)
        self.log.info(f"Node balance: {info}")

        self.log.info(f"Generate {self.ACCOUNTS} account addresses")
        self._accounts = generate_accounts(self._node, node_address, self.ACCOUNTS)

        self.log.info(f"Generate {self.ACCOUNTS} moderator addresses")
        self._moders = generate_accounts(
            self._node, node_address, self.ACCOUNTS, is_moderator=True
        )

    def register_accounts(self):
        self.log.info("Register accounts")
        register_accounts(self._node, self.accounts)
        register_accounts(self._node, self.moderators)

    def generate_transfers(self):
        self.log.info("Generate payments between accounts")

    def generate_posts(self):
        self.log.info("Generate accounts posts")

    def generate_comments(self):
        self.log.info("Generate posts comments")

    def generate_likes(self):
        self.log.info("Generate posts and comments likes")

    def generate_subscriptions(self):
        self.log.info("Generate account subscriptions")

    def generate_blacklists(self):
        self.log.info("Generate accounts blacklists")

    def pub_gen_tx(self, *args, **kwargs):
        return self._node.public().generatetransaction(*args, **kwargs)

    @property
    def accounts(self):
        return self._accounts

    @property
    def moderators(self):
        return self._moders
