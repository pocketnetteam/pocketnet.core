# Copyright (c) 2023 The Pocketnet developers
# Distributed under the Apache 2.0 software license, see the accompanying
# https://www.apache.org/licenses/LICENSE-2.0

from framework.models import Account, AccountPayload


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

    for account in accounts:
        address_info = node.public().getaddressinfo(address=account.Address)
        assert address_info["balance"] == amount

    return accounts


def register_accounts(node, accounts, out_count=50):
    hashes = []
    pub_gen_tx = node.public().generatetransaction
    for account in accounts:
        payload = AccountPayload(
            account.Name, "image", "en", "about", "s", "b", "pubkey"
        )
        _hash = pub_gen_tx(account, payload, out_count)
        hashes.append(_hash)

    node.stakeblock(15)

    for i, account in enumerate(accounts):
        assert node.public().getuserprofile(account.Address)[0]["hash"] == hashes[i]


def rollback_node(node, blocks, logger=None):
    if logger is not None:
        logger.warning(f"Rolling back node, number of blocks: {blocks}")

    for _ in range(blocks):
        node.invalidateblock(node.getbestblockhash())
