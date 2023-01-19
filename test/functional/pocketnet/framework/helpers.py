# Copyright (c) 2023 The Pocketnet developers
# Distributed under the Apache 2.0 software license, see the accompanying
# https://www.apache.org/licenses/LICENSE-2.0


def rollback_node(node, blocks, logger=None):
    if logger is not None:
        logger.warning(f"Rolling back node, number of blocks: {blocks}")

    for _ in range(blocks):
        node.invalidateblock(node.getbestblockhash())

