// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 Bitcoin developers
// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETCONSENSUS_COMPLAIN_HPP
#define POCKETCONSENSUS_COMPLAIN_HPP

#include "pocketdb/consensus/social/Base.hpp"
#include "pocketdb/models/dto/Complain.hpp"

namespace PocketConsensus
{
    /*******************************************************************************************************************
    *
    *  Complain consensus base class
    *
    *******************************************************************************************************************/
    class ComplainConsensus : public SocialBaseConsensus
    {
    protected:
    public:
        ComplainConsensus(int height) : SocialBaseConsensus(height) {}
        ComplainConsensus() : SocialBaseConsensus() {}

        tuple<bool, SocialConsensusResult> Validate(shared_ptr<Transaction> tx, PocketBlock& block)
        {
            return make_tuple(true, SocialConsensusResult_Success);
            // TODO (brangr): implement
            // std::string _txid = oitm["txid"].get_str();
            // std::string _address = oitm["address"].get_str();
            // std::string _post = oitm["posttxid"].get_str();
            // int64_t _time = oitm["time"].get_int64();

            // if (!CheckRegistration(oitm, _address, checkMempool, checkWithTime, height, blockVtx, result)) {
            //     return false;
            // }

            // // Check score to self
            // bool not_found = false;
            // reindexer::Item postItm;
            // if (g_pocketdb->SelectOne(
            //                 reindexer::Query("Posts")
            //                     .Where("txid", CondEq, _post)
            //                     .Where("block", CondLt, height),
            //                 postItm)
            //         .ok()) {
            //     // Score to self post
            //     if (postItm["address"].As<string>() == _address) {
            //         result = ANTIBOTRESULT::SelfComplain;
            //         return false;
            //     }
            // } else {
            //     // Post not found
            //     not_found = true;

            //     // Maybe in current block?
            //     if (blockVtx.Exists("Posts")) {
            //         for (auto& mtx : blockVtx.Data["Posts"]) {
            //             if (mtx["txid"].get_str() == _post) {
            //                 not_found = false;
            //                 break;
            //             }
            //         }
            //     }

            //     if (not_found) {
            //         result = ANTIBOTRESULT::NotFound;
            //         return false;
            //     }
            // }

            // // Check double score to post
            // reindexer::Item doubleComplainItm;
            // if (g_pocketdb->SelectOne(
            //                 reindexer::Query("Complains")
            //                     .Where("address", CondEq, _address)
            //                     .Where("posttxid", CondEq, _post)
            //                     .Where("block", CondLt, height),
            //                 doubleComplainItm)
            //         .ok()) {
            //     result = ANTIBOTRESULT::DoubleComplain;
            //     return false;
            // }

            // ABMODE mode;
            // int reputation = 0;
            // int64_t balance = 0;
            // getMode(_address, mode, reputation, balance, height);
            // int64_t minRep = GetActualLimit(Limit::threshold_reputation_complains, height);
            // if (reputation < minRep) {
            //     LogPrintf("%s - %s < %s\n", _address, reputation, minRep);
            //     result = ANTIBOTRESULT::LowReputation;
            //     return false;
            // }

            // reindexer::QueryResults complainsRes;
            // if (!g_pocketdb->DB()->Select(
            //                         reindexer::Query("Complains")
            //                             .Where("address", CondEq, _address)
            //                             .Where("time", CondGe, _time - 86400)
            //                             .Where("block", CondLt, height),
            //                         complainsRes)
            //         .ok()) {
            //     result = ANTIBOTRESULT::Failed;
            //     return false;
            // }

            // int complainCount = complainsRes.Count();

            // // Also check mempool
            // if (checkMempool) {
            //     reindexer::QueryResults res;
            //     if (g_pocketdb->Select(reindexer::Query("Mempool").Where("table", CondEq, "Complains").Not().Where("txid", CondEq, _txid), res).ok()) {
            //         for (auto& m : res) {
            //             reindexer::Item mItm = m.GetItem();
            //             std::string t_src = DecodeBase64(mItm["data"].As<string>());

            //             reindexer::Item t_itm = g_pocketdb->DB()->NewItem("Complains");
            //             if (t_itm.FromJSON(t_src).ok()) {
            //                 if (t_itm["address"].As<string>() == _address) {
            //                     if (!checkWithTime || t_itm["time"].As<int64_t>() <= _time)
            //                         complainCount += 1;

            //                     if (t_itm["posttxid"].As<string>() == _post) {
            //                         result = ANTIBOTRESULT::DoubleComplain;
            //                         return false;
            //                     }
            //                 }
            //             }
            //         }
            //     }
            // }

            // // Check block
            // if (blockVtx.Exists("Complains")) {
            //     for (auto& mtx : blockVtx.Data["Complains"]) {
            //         if (mtx["txid"].get_str() != _txid && mtx["address"].get_str() == _address) {
            //             if (!checkWithTime || mtx["time"].get_int64() <= _time)
            //                 complainCount += 1;

            //             if (mtx["posttxid"].get_str() == _post) {
            //                 result = ANTIBOTRESULT::DoubleComplain;
            //                 return false;
            //             }
            //         }
            //     }
            // }

            // int limit = getLimit(Complain, mode, height);
            // if (complainCount >= limit) {
            //     result = ANTIBOTRESULT::ComplainLimit;
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
    class ComplainConsensus_checkpoint_1 : public ComplainConsensus
    {
    protected:
        int CheckpointHeight() override { return 1; }
    public:
        ComplainConsensus_checkpoint_1(int height) : ComplainConsensus(height) {}
    };


    /*******************************************************************************************************************
    *
    *  Factory for select actual rules version
    *  Каждая новая перегрузка добавляет новый функционал, поддерживающийся с некоторым условием - например высота
    *
    *******************************************************************************************************************/
    class ComplainConsensusFactory
    {
    private:
        static inline const std::map<int, std::function<ComplainConsensus*(int height)>> m_rules =
        {
            {1, [](int height) { return new ComplainConsensus_checkpoint_1(height); }},
            {0, [](int height) { return new ComplainConsensus(height); }},
        };
    public:
        shared_ptr <ComplainConsensus> Instance(int height)
        {
            return shared_ptr<ComplainConsensus>(
                (--m_rules.upper_bound(height))->second(height)
            );
        }
    };
}

#endif // POCKETCONSENSUS_COMPLAIN_HPP