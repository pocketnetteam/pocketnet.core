// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 Bitcoin developers
// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETCONSENSUS_SUBSCRIBE_HPP
#define POCKETCONSENSUS_SUBSCRIBE_HPP

#include "pocketdb/consensus/social/Base.hpp"
#include "pocketdb/models/base/Transaction.hpp"
#include "pocketdb/models/dto/Subscribe.hpp"

namespace PocketConsensus
{
    /*******************************************************************************************************************
    *
    *  Subscribe consensus base class
    *
    *******************************************************************************************************************/
    class SubscribeConsensus : public SocialBaseConsensus
    {
    public:
        SubscribeConsensus(int height) : SocialBaseConsensus(height) {}
        SubscribeConsensus() : SocialBaseConsensus() {}

        tuple<bool, SocialConsensusResult> Validate(shared_ptr<Transaction> tx, PocketBlock& block) override
        {
            if (auto[ok, result] = SocialBaseConsensus::Validate(tx, block); !ok)
                return make_tuple(false, result);

            if (auto[ok, result] = Validate(static_pointer_cast<Subscribe>(tx), block); !ok)
                return make_tuple(false, result);
                
            return make_tuple(true, SocialConsensusResult_Success);
        }

        tuple<bool, SocialConsensusResult> Check(shared_ptr<Transaction> tx) override
        {
            if (auto[ok, result] = SocialBaseConsensus::Check(tx); !ok)
                return make_tuple(false, result);

            if (auto[ok, result] = Check(static_pointer_cast<Subscribe>(tx)); !ok)
                return make_tuple(false, result);
                
            return make_tuple(true, SocialConsensusResult_Success);
        }

    protected:

        virtual tuple<bool, SocialConsensusResult> Validate(shared_ptr<Subscribe> tx, PocketBlock& block)
        {
            vector<string> addresses = {*tx->GetAddress(), *tx->GetAddressTo()};
            if (!PocketDb::ConsensusRepoInst.ExistsUserRegistrations(addresses))
                return make_tuple(false, SocialConsensusResult_NotRegistered);

            // // Also check mempool
            // if (checkMempool) {
            //     reindexer::QueryResults res;
            //     if (g_pocketdb->Select(reindexer::Query("Mempool").Where("table", CondEq, "Subscribes").Not().Where("txid", CondEq, _txid), res).ok()) {
            //         for (auto& m : res) {
            //             reindexer::Item mItm = m.GetItem();
            //             std::string t_src = DecodeBase64(mItm["data"].As<string>());

            //             reindexer::Item t_itm = g_pocketdb->DB()->NewItem("Subscribes");
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
            // if (blockVtx.Exists("Subscribes")) {
            //     for (auto& mtx : blockVtx.Data["Subscribes"]) {
            //         if (mtx["txid"].get_str() != _txid && mtx["address"].get_str() == _address && mtx["address_to"].get_str() == _address_to) {
            //             result = ANTIBOTRESULT::ManyTransactions;
            //             return false;
            //         }
            //     }
            // }

            auto[subscribeExists, subscribeType] = PocketDb::ConsensusRepoInst.GetLastSubscribeType(*tx->GetAddress(),
             *tx->GetAddressTo(), *tx->GetHeight());
            if (subscribeExists && subscribeType == ACTION_SUBSCRIBE)
            {
                if (!IsCheckpointTransaction(*tx->GetHash()))
                {
                    return make_tuple(false, SocialConsensusResult_DoubleBlocking);
                }
                else
                {
                    LogPrintf("Found checkpoint transaction %s\n", *tx->GetHash());
                }
            }

            return make_tuple(true, SocialConsensusResult_Success);
        }
        
    private:

        tuple<bool, SocialConsensusResult> Check(shared_ptr<Subscribe> tx)
        {
            if (*tx->GetAddress() == *tx->GetAddressTo())
                return make_tuple(false, SocialConsensusResult_SelfSubscribe);

            return make_tuple(true, SocialConsensusResult_Success);
        }
    };

    /*******************************************************************************************************************
    *
    *  Consensus checkpoint at 1 block
    *
    *******************************************************************************************************************/
    class SubscribeConsensus_checkpoint_1 : public SubscribeConsensus
    {
    protected:
        int CheckpointHeight() override { return 1; }
    public:
        SubscribeConsensus_checkpoint_1(int height) : SubscribeConsensus(height) {}
    };


    /*******************************************************************************************************************
    *
    *  Factory for select actual rules version
    *  Каждая новая перегрузка добавляет новый функционал, поддерживающийся с некоторым условием - например высота
    *
    *******************************************************************************************************************/
    class SubscribeConsensusFactory
    {
    private:
        static inline const std::map<int, std::function<SubscribeConsensus*(int height)>> m_rules =
        {
            {1, [](int height) { return new SubscribeConsensus_checkpoint_1(height); }},
            {0, [](int height) { return new SubscribeConsensus(height); }},
        };
    public:
        shared_ptr <SubscribeConsensus> Instance(int height)
        {
            return shared_ptr<SubscribeConsensus>(
                (--m_rules.upper_bound(height))->second(height)
            );
        }
    };
}

#endif // POCKETCONSENSUS_SUBSCRIBE_HPP