// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETTX_BLOCK_H
#define POCKETTX_BLOCK_H

#include <string>
#include <univalue/include/univalue.h>
#include <utility>
#include <util/strencodings.h>
#include <crypto/sha256.h>
#include <primitives/block.h>
#include <primitives/transaction.h>

#include "pocketdb/models/base/Payload.h"
#include "pocketdb/models/base/Transaction.h"
#include "pocketdb/util/Empty.h"

#include <optional>

namespace PocketTx
{
    using namespace std;
    using namespace PocketUtil;

    class Block : public Base
    {
    public:
        Block();
        Block(const CBlock& block);

        const optional<string>& GetHash() const;
        void SetHash(string value);
        bool operator==(const string& hash) const;

        const optional<string>& GetSignature() const;
        void SetSignature(string value);

        const optional<int64_t>& GetVersion() const;
        void SetVersion(int64_t value);

        const optional<string>& GetPrevHash() const;
        void SetPrevHash(string value);

        const optional<string>& GetMerkleRoot() const;
        void SetMerkleRoot(string value);

        const optional<int64_t>& GetTime() const;
        void SetTime(int64_t value);

        const optional<int64_t>& GetBits() const;
        void SetBits(int64_t value);

        const optional<int64_t>& GetNonce() const;
        void SetNonce(int64_t value);

        const optional<vector<shared_ptr<Transaction>>>& GetTransactions() const;
        void SetTransactions(vector<shared_ptr<Transaction>> value);

    protected:
        optional<vector<shared_ptr<Transaction>>> m_transactions;
        optional<string> m_hash = nullopt;
        optional<string> m_signature = nullopt;
        optional<int64_t> m_version = nullopt;
        optional<string> m_prevhash = nullopt;
        optional<string> m_merkleroot = nullopt;
        optional<int64_t> m_time = nullopt;
        optional<int64_t> m_bits = nullopt;
        optional<int64_t> m_nonce = nullopt;
    };

} // namespace PocketTx

#endif // POCKETTX_BLOCK_H