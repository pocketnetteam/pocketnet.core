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

        tuple<bool, SocialConsensusResult> Validate(shared_ptr<Transaction> tx, PocketBlock& block) override
        {
            if (auto[ok, result] = SocialBaseConsensus::Validate(tx, block); !ok)
                return make_tuple(false, result);

            if (auto[ok, result] = Validate(static_pointer_cast<Complain>(tx), block); !ok)
                return make_tuple(false, result);
                
            return make_tuple(true, SocialConsensusResult_Success);
        }

        tuple<bool, SocialConsensusResult> Check(shared_ptr<Transaction> tx) override
        {
            if (auto[ok, result] = SocialBaseConsensus::Check(tx); !ok)
                return make_tuple(false, result);

            if (auto[ok, result] = Check(static_pointer_cast<Complain>(tx)); !ok)
                return make_tuple(false, result);
                
            return make_tuple(true, SocialConsensusResult_Success);
        }

    protected:

        virtual tuple<bool, SocialConsensusResult> Validate(shared_ptr<Complain> tx, PocketBlock& block)
        {
            vector<string> addresses = {*tx->GetAddress()};
            if (!PocketDb::ConsensusRepoInst.ExistsUserRegistrations(addresses))
                return make_tuple(false, SocialConsensusResult_NotRegistered);

            auto postAddress = PocketDb::ConsensusRepoInst.GetPostAddress(*tx->GetPostTxHash(), *tx->GetHeight());
            if (postAddress == nullptr)
            {
                auto notFound = true;

                for (const auto& otherTx : block)
                {
                    if (*otherTx->GetType() != CONTENT_POST)
                    {
                        continue;
                    }

                    //TODO question (brangr): if post in current block will be not valid, complain anyway will be valid?
                    if (*otherTx->GetHash() == *tx->GetPostTxHash())
                    {
                        notFound = false;
                        break;
                    }
                }

                if (notFound)
                {
                    return make_tuple(false, SocialConsensusResult_NotFound);
                }
            }
            else
            {
                if (*postAddress == *tx->GetAddress())
                {
                    return make_tuple(false, SocialConsensusResult_SelfComplain);
                }
            }

            if (PocketDb::ConsensusRepoInst.ExistsComplain(*tx->GetPostTxHash(), *tx->GetAddress(), *tx->GetHeight()))
            {
                return make_tuple(false, SocialConsensusResult_DoubleComplain);
            }

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
        
    private:
    
        tuple<bool, SocialConsensusResult> Check(shared_ptr<Complain> tx)
        {
            return make_tuple(true, SocialConsensusResult_Success);
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