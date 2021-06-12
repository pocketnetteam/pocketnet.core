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
    protected:
    public:
        BlockingCancelConsensus(int height) : SocialBaseConsensus(height) {}
        BlockingCancelConsensus() : SocialBaseConsensus() {}

        tuple<bool, SocialConsensusResult> Validate(shared_ptr<Transaction> tx, PocketBlock& block)
        {
            return make_tuple(true, SocialConsensusResult_Success);
            // TODO (brangr): implement
            // std::string _txid = oitm["txid"].get_str();
            // std::string _address = oitm["address"].get_str();
            // std::string _address_to = oitm["address_to"].get_str();
            // bool _unblocking = oitm["unblocking"].get_bool();
            // int64_t _time = oitm["time"].get_int64();

            // if (!CheckRegistration(oitm, _address, checkMempool, checkWithTime, height, blockVtx, result)) {
            //     return false;
            // }

            // if (!CheckRegistration(oitm, _address_to, checkMempool, checkWithTime, height, blockVtx, result)) {
            //     return false;
            // }

            // if (_address == _address_to) {
            //     result = ANTIBOTRESULT::SelfBlocking;
            //     return false;
            // }

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

            // reindexer::Item sItm;
            // Error err = g_pocketdb->SelectOne(
            //     reindexer::Query("BlockingView")
            //         .Where("address", CondEq, _address)
            //         .Where("address_to", CondEq, _address_to)
            //         .Where("block", CondLt, height),
            //     sItm);

            // if (_unblocking && !err.ok()) {
            //     result = ANTIBOTRESULT::InvalidBlocking;
            //     return false;
            // }

            // if (!_unblocking && err.ok()) {
            //     result = ANTIBOTRESULT::DoubleBlocking;
            //     return false;
            // }

            // return true;
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