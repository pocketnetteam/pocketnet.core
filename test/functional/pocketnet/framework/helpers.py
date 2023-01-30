# Copyright (c) 2023 The Pocketnet developers
# Distributed under the Apache 2.0 software license, see the accompanying
# https://www.apache.org/licenses/LICENSE-2.0

from framework.models import Account


def generate_accounts(node, node_address, account_num, amount=10, is_moderator=False):
    accounts = []
    for i in range(account_num):
        acc = node.public().generateaddress()
        name = f"moderator{i}" if is_moderator else f"user{i}"
        accounts.append(Account(acc["address"], acc["privkey"], name))
        node.sendtoaddress(
            address=accounts[i].Address, amount=amount, destaddress=node_address
        )

    node.stakeblock(1)

    for i in range(account_num):
        assert (
            node.public().getaddressinfo(address=accounts[i].Address)["balance"]
            == amount
        )

    return accounts


def rollback_node(node, blocks, logger=None):
    if logger is not None:
        logger.warning(f"Rolling back node, number of blocks: {blocks}")

    for _ in range(blocks):
        node.invalidateblock(node.getbestblockhash())
