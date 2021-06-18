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

    protected:

        tuple<bool, SocialConsensusResult> ValidateModel(shared_ptr <Transaction> tx) override
        {
            auto ptx = static_pointer_cast<Subscribe>(tx);

            // Check registration
            vector<string> addresses = {*ptx->GetAddress(), *ptx->GetAddressTo()};
            if (!PocketDb::ConsensusRepoInst.ExistsUserRegistrations(addresses))
                return {false, SocialConsensusResult_NotRegistered};

            auto[subscribeExists, subscribeType] = PocketDb::ConsensusRepoInst.GetLastSubscribeType(
                *ptx->GetAddress(),
                *ptx->GetAddressTo());

            if (subscribeExists && subscribeType == ACTION_SUBSCRIBE)
            {
                // TODO (brangr): я думаю все чекпойнты прямо тут и сложить
                if (!IsCheckpointTransaction(*tx->GetHash()))
                {
                    return {false, SocialConsensusResult_DoubleBlocking};
                }
                else
                {
                    LogPrintf("Found checkpoint transaction %s\n", *tx->GetHash());
                }
            }

            return Success;
        }

        tuple<bool, SocialConsensusResult> ValidateLimit(shared_ptr <Transaction> tx, PocketBlock& block) override
        {
            // if (blockVtx.Exists("Subscribes")) {
            //     for (auto& mtx : blockVtx.Data["Subscribes"]) {
            //         if (mtx["txid"].get_str() != _txid && mtx["address"].get_str() == _address && mtx["address_to"].get_str() == _address_to) {
            //             result = ANTIBOTRESULT::ManyTransactions;
            //             return false;
            //         }
            //     }
            // }
        }

        tuple<bool, SocialConsensusResult> ValidateLimit(shared_ptr <Transaction> tx) override
        {
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
        }

        tuple<bool, SocialConsensusResult> CheckModel(shared_ptr <Transaction> tx) override
        {
            auto ptx = static_pointer_cast<Subscribe>(tx);

            if (*ptx->GetAddress() == *ptx->GetAddressTo())
                return {false, SocialConsensusResult_SelfSubscribe};

            return Success;
        }
    };

    /*******************************************************************************************************************
    *
    *  Factory for select actual rules version
    *
    *******************************************************************************************************************/
    class SubscribeConsensusFactory
    {
    private:
        static inline const std::map<int, std::function<SubscribeConsensus*(int height)>> m_rules =
            {
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