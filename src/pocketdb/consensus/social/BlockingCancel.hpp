// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 Bitcoin developers
// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETCONSENSUS_BLOCKINGCANCEL_HPP
#define POCKETCONSENSUS_BLOCKINGCANCEL_HPP

#include "pocketdb/consensus/social/Base.hpp"
#include "pocketdb/models/dto/BlockingCancel.hpp"

namespace PocketConsensus
{
    /*******************************************************************************************************************
    *
    *  BlockingCancel consensus base class
    *
    *******************************************************************************************************************/
    class BlockingCancelConsensus : public SocialBaseConsensus
    {
    public:
        BlockingCancelConsensus(int height) : SocialBaseConsensus(height) {}

    protected:

        tuple<bool, SocialConsensusResult> ValidateModel(shared_ptr <Transaction> tx) override
        {
            auto ptx = static_pointer_cast<BlockingCancel>(tx);

            vector<string> addresses = {*ptx->GetAddress(), *ptx->GetAddressTo()};
            if (PocketDb::ConsensusRepoInst.ExistsUserRegistrations(addresses))
                return {false, SocialConsensusResult_NotRegistered};

            auto[existsBlocking, blockingType] = PocketDb::ConsensusRepoInst.GetLastBlockingType(
                *ptx->GetAddress(),
                *ptx->GetAddressTo());

            if (!existsBlocking || blockingType != ACTION_BLOCKING)
                return {false, SocialConsensusResult_InvalidBlocking};

            return Success;
        }

        tuple<bool, SocialConsensusResult> ValidateLimit(shared_ptr<Transaction> tx, const PocketBlock& block) override
        {
            // if (blockVtx.Exists("Blocking")) {
            //     for (auto& mtx : blockVtx.Data["Blocking"]) {
            //         if (mtx["txid"].get_str() != _txid && mtx["address"].get_str() == _address && mtx["address_to"].get_str() == _address_to) {
            //             result = ANTIBOTRESULT::ManyTransactions;
            //             return false;
            //         }
            //     }
            // }
        }

        tuple<bool, SocialConsensusResult> ValidateLimit(shared_ptr<Transaction> tx) override
        {
            //     reindexer::QueryResults res;
            //     if (g_pocketdb->Select(reindexer::Query("Mempool").Where("table", CondEq, "Blocking").Not().Where("txid", CondEq, _txid), res).ok()) {
            //         for (auto& m : res) {
            //             reindexer::Item mItm = m.GetItem();
            //             std::string t_src = DecodeBase64(mItm["data"].As<string>());

            //             reindexer::Item t_itm = g_pocketdb->DB()->NewItem("Blocking");
            //             if (t_itm.FromJSON(t_src).ok()) {
            //                 if (t_itm["address"].As<string>() == _address && t_itm["address_to"].As<string>() == _address_to) {
            //                     if (!checkWithTime || t_itm["time"].As<int64_t>() <= _time) {
            //                         result = ANTIBOTRESULT::ManyTransactions;
            //                         return false;
            //                     }
            //                 }
            //             }
            //         }
            //     }
        }

        tuple<bool, SocialConsensusResult> CheckModel(shared_ptr <Transaction> tx) override
        {
            auto ptx = static_pointer_cast<BlockingCancel>(tx);

            // Check required fields
            if (IsEmpty(ptx->GetAddress())) return {false, SocialConsensusResult_Failed};
            if (IsEmpty(ptx->GetAddressTo())) return {false, SocialConsensusResult_Failed};

            // Blocking self
            if (*ptx->GetAddress() == *ptx->GetAddressTo())
                return {false, SocialConsensusResult_SelfBlocking};

            return Success;
        }
    };

    /*******************************************************************************************************************
    *
    *  Factory for select actual rules version
    *
    *******************************************************************************************************************/
    class BlockingCancelConsensusFactory
    {
    private:
        static inline const std::map<int, std::function<BlockingCancelConsensus*(int height)>> m_rules =
            {
                {0, [](int height) { return new BlockingCancelConsensus(height); }},
            };
    public:
        shared_ptr <BlockingCancelConsensus> Instance(int height)
        {
            return shared_ptr<BlockingCancelConsensus>(
                (--m_rules.upper_bound(height))->second(height)
            );
        }
    };
}

#endif // POCKETCONSENSUS_BLOCKINGCANCEL_HPP