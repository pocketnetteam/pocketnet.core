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

    protected:

        tuple<bool, SocialConsensusResult> ValidateModel(shared_ptr<Transaction> tx) override
        {
            auto ptx = static_pointer_cast<SubscribePrivate>(tx);

            vector<string> addresses = {*ptx->GetAddress(), *ptx->GetAddressTo()};
            if (!PocketDb::ConsensusRepoInst.ExistsUserRegistrations(addresses))
                return {false, SocialConsensusResult_NotRegistered};

            // Check double blocking
            auto[subscribeExists, subscribeType] = PocketDb::ConsensusRepoInst.GetLastSubscribeType(
                *ptx->GetAddress(),
                *ptx->GetAddressTo());

            if (subscribeExists && subscribeType == ACTION_SUBSCRIBE_PRIVATE)
            {
                // TODO (brangr): чекпойнты сюда
                if (!IsCheckpointTransaction(*tx->GetHash()))
                    return {false, SocialConsensusResult_DoubleBlocking};
            }

            return Success;
        }

        tuple<bool, SocialConsensusResult> ValidateLimit(shared_ptr<Transaction> tx, const PocketBlock& block) override
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

        tuple<bool, SocialConsensusResult> ValidateLimit(shared_ptr<Transaction> tx) override
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

        tuple<bool, SocialConsensusResult> CheckModel(shared_ptr<Transaction> tx) override
        {
            auto ptx = static_pointer_cast<SubscribePrivate>(tx);

            if (*ptx->GetAddress() == *ptx->GetAddressTo())
                return {false, SocialConsensusResult_SelfSubscribe};

            return Success;
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