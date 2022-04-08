// Copyright (c) 2019 The Pocketcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "consensus/validation.h"
#include <test/util/mining.h>

#include <chainparams.h>
#include <consensus/merkle.h>
#include <key_io.h>
#include <miner.h>
#include <node/context.h>
#include <pow.h>
#include <script/standard.h>
#include <util/check.h>
#include <validation.h>
#include <pocketdb/services/Serializer.h>

#include "pocketdb/services/Serializer.h"

CTxIn generatetoaddress(const NodeContext& node, const std::string& address)
{
    const auto dest = DecodeDestination(address);
    assert(IsValidDestination(dest));
    const auto coinbase_script = GetScriptForDestination(dest);

    return MineBlock(node, coinbase_script);
}

CTxIn MineBlock(const NodeContext& node, const CScript& coinbase_scriptPubKey)
{
    auto block = PrepareBlock(node, coinbase_scriptPubKey);

    // TODO : is height correct?
    while (!CheckProofOfWork(block->GetHash(), block->nBits, Params().GetConsensus(), 0)) {
        ++block->nNonce;
        assert(block->nNonce);
    }

    BlockValidationState state;
    auto[deserializeOk, pocketBlock] = PocketServices::Serializer::DeserializeBlock(*block);
    auto pocketBlockRef = std::make_shared<PocketBlock>(pocketBlock);
    bool processed{Assert(node.chainman)->ProcessNewBlock(state, Params(), block, pocketBlockRef, true, nullptr)};
    assert(processed);

    return CTxIn{block->vtx[0]->GetHash(), 0};
}

std::shared_ptr<CBlock> PrepareBlock(const NodeContext& node, const CScript& coinbase_scriptPubKey)
{
    auto block = std::make_shared<CBlock>(
        BlockAssembler{*Assert(node.mempool), Params()}
            .CreateNewBlock(coinbase_scriptPubKey)
            ->block);

    LOCK(cs_main);
    block->nTime = ::ChainActive().Tip()->GetMedianTimePast() + 1;
    block->hashMerkleRoot = BlockMerkleRoot(*block);

    return block;
}
