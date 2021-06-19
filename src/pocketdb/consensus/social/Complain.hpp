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
    public:
        ComplainConsensus(int height) : SocialBaseConsensus(height) {}
        ComplainConsensus() : SocialBaseConsensus() {}

    protected:

        tuple<bool, SocialConsensusResult> ValidateModel(shared_ptr <Transaction> tx) override
        {
            auto ptx = static_pointer_cast<Complain>(tx);

            // Check registration
            vector<string> addresses = {*ptx->GetAddress()};
            if (!PocketDb::ConsensusRepoInst.ExistsUserRegistrations(addresses))
                return {false, SocialConsensusResult_NotRegistered};

            // Authot or post must be exists
            auto postAddress = PocketDb::ConsensusRepoInst.GetPostAddress(*ptx->GetPostTxHash());
            if (postAddress == nullptr)
                return {false, SocialConsensusResult_NotFound};

            // TODO (brangr): такое чувство что это избыточная логика
            // Мы не проверяем наличие комплейнутого поста в мемпуле, соответственно
            // и в блоке искать бесполезно - нужно проверить наличие такой ситуации в живой БД
//                auto notFound = true;
//                for (const auto& otherTx : block)
//                {
//                    if (*otherTx->GetType() != CONTENT_POST)
//                    {
//                        continue;
//                    }
//
//                    //TODO question (brangr): if post in current block will be not valid, complain anyway will be valid?
//                    if (*otherTx->GetHash() == *ptx->GetPostTxHash())
//                    {
//                        notFound = false;
//                        break;
//                    }
//                }
//                if (notFound)
//                {
//                    return {false, SocialConsensusResult_NotFound};
//                }

            // Complain to self
            if (*postAddress == *ptx->GetAddress())
                return {false, SocialConsensusResult_SelfComplain};

            // Check double complain
            if (PocketDb::ConsensusRepoInst.ExistsComplain(*ptx->GetPostTxHash(), *ptx->GetAddress()))
                return {false, SocialConsensusResult_DoubleComplain};

            // Complains allow only with high reputation
            // int reputation = 0;
            // int64_t balance = 0;
            // getMode(_address, mode, reputation, balance, height);
            // int64_t minRep = GetActualLimit(Limit::threshold_reputation_complains, height);
            // if (reputation < minRep) {
            //     LogPrintf("%s - %s < %s\n", _address, reputation, minRep);
            //     result = ANTIBOTRESULT::LowReputation;
            //     return false;
            // }

            return Success;
        }

        tuple<bool, SocialConsensusResult> ValidateLimit(shared_ptr <Transaction> tx, const PocketBlock& block) override
        {
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
        }

        tuple<bool, SocialConsensusResult> ValidateLimit(shared_ptr <Transaction> tx) override
        {
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




            // int limit = getLimit(Complain, mode, height);
            // if (complainCount >= limit) {
            //     result = ANTIBOTRESULT::ComplainLimit;
            //     return false;
            // }
        }

        tuple<bool, SocialConsensusResult> CheckModel(shared_ptr <Transaction> tx) override
        {
            return Success;
        }
    };

    /*******************************************************************************************************************
    *
    *  Factory for select actual rules version
    *
    *******************************************************************************************************************/
    class ComplainConsensusFactory
    {
    private:
        static inline const std::map<int, std::function<ComplainConsensus*(int height)>> m_rules =
            {
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