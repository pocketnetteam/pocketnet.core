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
        BlockingCancelConsensus() : SocialBaseConsensus() {}

        tuple<bool, SocialConsensusResult> Validate(shared_ptr<Transaction> tx, PocketBlock& block) override
        {
            return Validate(static_pointer_cast<BlockingCancel>(tx), block);
        }

        tuple<bool, SocialConsensusResult> Check(shared_ptr<Transaction> tx) override
        {
            return Check(static_pointer_cast<BlockingCancel>(tx));
        }
    protected:
        tuple<bool, SocialConsensusResult> Validate(shared_ptr<BlockingCancel> tx, PocketBlock& block)
        {
            vector<string> addresses = {*tx->GetAddress(), *tx->GetAddressTo()};
            if (auto[ok, result] = PocketDb::ConsensusRepoInst.ExistsUserRegistrations(addresses, *tx->GetHeight()); ok)
            {
                if (!result)
                {
                    return make_tuple(false, SocialConsensusResult_NotRegistered);
                }
            }

            // //-----------------------
            // // Also check mempool
            // if (checkMempool) {
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
            // }

            // // Check block
            // if (blockVtx.Exists("Blocking")) {
            //     for (auto& mtx : blockVtx.Data["Blocking"]) {
            //         if (mtx["txid"].get_str() != _txid && mtx["address"].get_str() == _address && mtx["address_to"].get_str() == _address_to) {
            //             result = ANTIBOTRESULT::ManyTransactions;
            //             return false;
            //         }
            //     }
            // }

            auto[ok, existsBlocking, blockingType] = PocketDb::ConsensusRepoInst.GetLastBlockingType(*tx->GetAddress(), *tx->GetAddressTo(), *tx->GetHeight());
            if (ok && existsBlocking && blockingType == ACTION_BLOCKING)
            {
                return make_tuple(false, SocialConsensusResult_DoubleBlocking);
            }

            return make_tuple(true, SocialConsensusResult_Success);
        }

        tuple<bool, SocialConsensusResult> Check(shared_ptr<BlockingCancel> tx)
        {
            // Blocking self
            if (*tx->GetAddress() == *tx->GetAddressTo())
            {
                return make_tuple(false, SocialConsensusResult_SelfBlocking);
            }

            return make_tuple(true, SocialConsensusResult_Success);
        }
    };

    /*******************************************************************************************************************
    *
    *  Consensus checkpoint at 1 block
    *
    *******************************************************************************************************************/
    class BlockingCancelConsensus_checkpoint_1 : public BlockingCancelConsensus
    {
    protected:
        int CheckpointHeight() override { return 1; }
    public:
        BlockingCancelConsensus_checkpoint_1(int height) : BlockingCancelConsensus(height) {}
    };


    /*******************************************************************************************************************
    *
    *  Factory for select actual rules version
    *  Каждая новая перегрузка добавляет новый функционал, поддерживающийся с некоторым условием - например высота
    *
    *******************************************************************************************************************/
    class BlockingCancelConsensusFactory
    {
    private:
        static inline const std::map<int, std::function<BlockingCancelConsensus*(int height)>> m_rules =
        {
            {1, [](int height) { return new BlockingCancelConsensus_checkpoint_1(height); }},
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