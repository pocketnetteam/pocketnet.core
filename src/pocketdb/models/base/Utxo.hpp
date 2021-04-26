// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 Bitcoin developers
// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETTX_UTXO_H
#define POCKETTX_UTXO_H

#include <string>
#include <univalue.h>
#include <utility>
#include <utilstrencodings.h>
#include <crypto/sha256.h>
#include "pocketdb/models/base/Base.hpp"

namespace PocketTx
{
    class Utxo : public Base
    {
    public:

        Utxo() = default;

        shared_ptr<string> GetTxId() const { return m_txId; }
        void SetTxId(string value) { m_txId = make_shared<string>(value); }

        shared_ptr<int64_t> GetBlock() const { return m_block; }
        void SetBlock(int64_t value) { m_block = make_shared<int64_t>(value); }

        shared_ptr<int64_t> GetTxOut() const { return m_block; }
        void SetTxOut(int64_t value) { m_txOut = make_shared<int64_t>(value); }

        shared_ptr<int64_t> GetTxTime() const { return m_txTime; }
        void SetTxTime(int64_t value) { m_txTime = make_shared<int64_t>(value); }

        shared_ptr<string> GetAddress() const { return m_address; }
        void SetAddress(string value) { m_address = make_shared<string>(value); }

        shared_ptr<int64_t> GetBlockSpent() const { return m_block_spent; }
        void SetBlockSpent(int64_t value) { m_block_spent = make_shared<int64_t>(value); }

        shared_ptr<int64_t> GetAmount() const { return m_amount; }
        void SetAmount(int64_t value) { m_amount = make_shared<int64_t>(value); }

    protected:
        shared_ptr<string> m_txId = nullptr;
        shared_ptr<int64_t> m_block = nullptr;
        shared_ptr<int64_t> m_txOut = nullptr;
        shared_ptr<int64_t> m_txTime = nullptr;
        shared_ptr<string> m_address = nullptr;

        shared_ptr<int64_t> m_amount = nullptr;
        shared_ptr<int64_t> m_block_spent = nullptr;

    private:

    };

} // namespace PocketTx

#endif // POCKETTX_UTXO_H