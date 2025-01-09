// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include "pocketdb/models/base/Block.h"

namespace PocketTx
{

    Block::Block() : Base()
    {
    }

    Block::Block(const CBlock& block) : Base()
    {
        SetHash(block.GetHash().GetHex());
        SetTime(block.GetBlockTime());
        SetBits(block.nBits);
        SetNonce(block.nNonce);
        SetPrevHash(block.hashPrevBlock.GetHex());
        SetMerkleRoot(block.hashMerkleRoot.GetHex());
        SetVersion(block.nVersion);
        SetSignature(HexStr(block.vchBlockSig));
    }

    const optional<string>& Block::GetHash() const { return m_hash; }
    void Block::SetHash(string value) { m_hash = value; }
    bool Block::operator==(const string& hash) const { return m_hash == hash; }

    const optional<string>& Block::GetSignature() const { return m_signature; }
    void Block::SetSignature(string value) { m_signature = value; }

    const optional<int64_t>& Block::GetVersion() const { return m_version; }
    void Block::SetVersion(int64_t value) { m_version = value; }

    const optional<string>& Block::GetPrevHash() const { return m_prevhash; }
    void Block::SetPrevHash(string value) { m_prevhash = value; }

    const optional<string>& Block::GetMerkleRoot() const { return m_merkleroot; }
    void Block::SetMerkleRoot(string value) { m_merkleroot = value; }

    const optional<int64_t>& Block::GetTime() const { return m_time; }
    void Block::SetTime(int64_t value) { m_time = value; }

    const optional<int64_t>& Block::GetBits() const { return m_bits; }
    void Block::SetBits(int64_t value) { m_bits = value; }

    const optional<int64_t>& Block::GetNonce() const { return m_nonce; }
    void Block::SetNonce(int64_t value) { m_nonce = value; }

    const optional<vector<shared_ptr<Transaction>>>& Block::GetTransactions() const { return m_transactions; }
    void Block::SetTransactions(vector<shared_ptr<Transaction>> value) { m_transactions = value; }

} // namespace PocketTx