def rollback_node(node, blocks, logger=None):
    if logger is not None:
        logger.warning(f"Rolling back node, number of blocks: {blocks}")

    for _ in range(blocks):
        node.invalidateblock(node.getbestblockhash())
