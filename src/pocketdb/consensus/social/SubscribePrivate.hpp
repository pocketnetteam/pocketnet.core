// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 Bitcoin developers
// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETCONSENSUS_SUBSCRIBEPRIVATE_HPP
#define POCKETCONSENSUS_SUBSCRIBEPRIVATE_HPP

#include "pocketdb/consensus/social/Base.hpp"
#include "pocketdb/models/base/Transaction.hpp"
#include "pocketdb/models/dto/SubscribePrivate.hpp"

namespace PocketConsensus
{
    /*******************************************************************************************************************
    *
    *  SubscribePrivate consensus base class
    *
    *******************************************************************************************************************/
    class SubscribePrivateConsensus : public SocialBaseConsensus
    {
    public:
        SubscribePrivateConsensus(int height) : SocialBaseConsensus(height) {}
        SubscribePrivateConsensus() : SocialBaseConsensus() {}

        tuple<bool, SocialConsensusResult> Validate(shared_ptr<Transaction> tx, PocketBlock& block) override
        {
            return Validate(static_pointer_cast<SubscribePrivate>(tx), block);
        }

        tuple<bool, SocialConsensusResult> Check(shared_ptr<Transaction> tx) override
        {
            return Check(static_pointer_cast<SubscribePrivate>(tx));
        }
    protected:
        tuple<bool, SocialConsensusResult> Validate(shared_ptr<SubscribePrivate> tx, PocketBlock& block)
        {
            vector<string> addresses = {*tx->GetAddress(), *tx->GetAddressTo()};
            if (!PocketDb::ConsensusRepoInst.ExistsUserRegistrations(addresses, *tx->GetHeight()))
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
            if (subscribeExists && subscribeType == ACTION_SUBSCRIBE_PRIVATE)
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

        tuple<bool, SocialConsensusResult> Check(shared_ptr<SubscribePrivate> tx)
        {
            if (*tx->GetAddress() == *tx->GetAddressTo())
            {
                return make_tuple(false, SocialConsensusResult_SelfSubscribe);
            }

            return make_tuple(true, SocialConsensusResult_Success);
        }
    };

    /*******************************************************************************************************************
    *
    *  Consensus checkpoint at 1 block
    *
    *******************************************************************************************************************/
    class SubscribePrivateConsensus_checkpoint_1 : public SubscribePrivateConsensus
    {
    protected:
        int CheckpointHeight() override { return 1; }
    public:
        SubscribePrivateConsensus_checkpoint_1(int height) : SubscribePrivateConsensus(height) {}
    };


    /*******************************************************************************************************************
    *
    *  Factory for select actual rules version
    *  Каждая новая перегрузка добавляет новый функционал, поддерживающийся с некоторым условием - например высота
    *
    *******************************************************************************************************************/
    class SubscribePrivateConsensusFactory
    {
    private:
        static inline const std::map<int, std::function<SubscribePrivateConsensus*(int height)>> m_rules =
        {
            {1, [](int height) { return new SubscribePrivateConsensus_checkpoint_1(height); }},
            {0, [](int height) { return new SubscribePrivateConsensus(height); }},
        };
    public:
        shared_ptr <SubscribePrivateConsensus> Instance(int height)
        {
            return shared_ptr<SubscribePrivateConsensus>(
                (--m_rules.upper_bound(height))->second(height)
            );
        }
    };
}

#endif // POCKETCONSENSUS_SUBSCRIBEPRIVATE_HPP