# Copyright (c) 2023 The Pocketnet developers
# Distributed under the Apache 2.0 software license, see the accompanying
# https://www.apache.org/licenses/LICENSE-2.0

import logging

from framework.helpers import generate_accounts


class ChainBuilder:
    ACCOUNTS = 10
    FIRST_COINBASE_BLOCKS = 1020

    def __init__(self, node, logger=None):
        self._node = node
        self.log = logger or logging.getLogger("ChainBuilder")
        self.log.setLevel(logging.DEBUG)

    def build(self):
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

    def transaction(self, *args, **kwargs):
        return self._node().generatetransaction(*args, **kwargs)

    @property
    def accounts(self):
        return self._accounts

    @property
    def moderators(self):
        return self._moders
