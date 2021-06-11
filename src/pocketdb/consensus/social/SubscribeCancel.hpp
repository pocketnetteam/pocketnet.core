// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 Bitcoin developers
// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETCONSENSUS_SUBSCRIBECANCEL_HPP
#define POCKETCONSENSUS_SUBSCRIBECANCEL_HPP

#include "pocketdb/consensus/Base.hpp"

namespace PocketConsensus
{
    /*******************************************************************************************************************
    *
    *  SubscribeCancel consensus base class
    *
    *******************************************************************************************************************/
    class SubscribeCancelConsensus : public SocialBaseConsensus
    {
    protected:
    public:
        SubscribeCancelConsensus(int height) : SocialBaseConsensus(height) {}

        tuple<bool, SocialConsensusResult> Validate(PocketBlock& txs) override
        {
            std::string _txid = oitm["txid"].get_str();
            std::string _address = oitm["address"].get_str();
            std::string _address_to = oitm["address_to"].get_str();
            bool _private = oitm["private"].get_bool();
            bool _unsubscribe = oitm["unsubscribe"].get_bool();
            int64_t _time = oitm["time"].get_int64();

            if (!CheckRegistration(oitm, _address, checkMempool, checkWithTime, height, blockVtx, result)) {
                return false;
            }

            if (!CheckRegistration(oitm, _address_to, checkMempool, checkWithTime, height, blockVtx, result)) {
                return false;
            }

            // Also check mempool
            if (checkMempool) {
                reindexer::QueryResults res;
                if (g_pocketdb->Select(reindexer::Query("Mempool").Where("table", CondEq, "Subscribes").Not().Where("txid", CondEq, _txid), res).ok()) {
                    for (auto& m : res) {
                        reindexer::Item mItm = m.GetItem();
                        std::string t_src = DecodeBase64(mItm["data"].As<string>());

                        reindexer::Item t_itm = g_pocketdb->DB()->NewItem("Subscribes");
                        if (t_itm.FromJSON(t_src).ok()) {
                            if (t_itm["address"].As<string>() == _address && t_itm["address_to"].As<string>() == _address_to) {
                                if (!checkWithTime || t_itm["time"].As<int64_t>() <= _time) {
                                    result = ANTIBOTRESULT::ManyTransactions;
                                    return false;
                                }
                            }
                        }
                    }
                }
            }

            // Check block
            if (blockVtx.Exists("Subscribes")) {
                for (auto& mtx : blockVtx.Data["Subscribes"]) {
                    if (mtx["txid"].get_str() != _txid && mtx["address"].get_str() == _address && mtx["address_to"].get_str() == _address_to) {
                        result = ANTIBOTRESULT::ManyTransactions;
                        return false;
                    }
                }
            }

            if (_address == _address_to) {
                result = ANTIBOTRESULT::SelfSubscribe;
                return false;
            }

            reindexer::Item sItm;
            Error err = g_pocketdb->SelectOne(
                reindexer::Query("SubscribesView")
                    .Where("address", CondEq, _address)
                    .Where("address_to", CondEq, _address_to)
                    .Where("block", CondLt, height),
                sItm);

            if (_unsubscribe && !err.ok()) {
                result = ANTIBOTRESULT::InvalideSubscribe;
                return false;
            }

            if (!_unsubscribe && err.ok() && sItm["private"].As<bool>() == _private) {
                if (!IsCheckpointTransaction(_txid)) {
                    result = ANTIBOTRESULT::DoubleSubscribe;
                    return false;
                } else {
                    LogPrintf("Found checkpoint transaction %s\n", _txid);
                }
            }

            return true;
        }

    };


    /*******************************************************************************************************************
    *
    *  Start checkpoint
    *
    *******************************************************************************************************************/
    class SubscribeCancelConsensus_checkpoint_0 : public SubscribeCancelConsensus
    {
    protected:
    public:

        SubscribeCancelConsensus_checkpoint_0(int height) : SubscribeCancelConsensus(height) {}

    }; // class SubscribeCancelConsensus_checkpoint_0


    /*******************************************************************************************************************
    *
    *  Consensus checkpoint at 1 block
    *
    *******************************************************************************************************************/
    class SubscribeCancelConsensus_checkpoint_1 : public SubscribeCancelConsensus_checkpoint_0
    {
    protected:
        int CheckpointHeight() override { return 1; }
    public:
        SubscribeCancelConsensus_checkpoint_1(int height) : SubscribeCancelConsensus_checkpoint_0(height) {}
    };


    /*******************************************************************************************************************
    *
    *  Factory for select actual rules version
    *  Каждая новая перегрузка добавляет новый функционал, поддерживающийся с некоторым условием - например высота
    *
    *******************************************************************************************************************/
    class SubscribeCancelConsensusFactory
    {
    private:
        inline static std::vector<std::pair<int, std::function<SubscribeCancelConsensus*(int height)>>> m_rules
            {
                {1, [](int height) { return new SubscribeCancelConsensus_checkpoint_1(height); }},
                {0, [](int height) { return new SubscribeCancelConsensus_checkpoint_0(height); }},
            };
    public:
        shared_ptr <SubscribeCancelConsensus> Instance(int height)
        {
            for (const auto& rule : m_rules)
                if (height >= rule.first)
                    return shared_ptr<SubscribeCancelConsensus>(rule.second(height));
        }
    };
}

#endif // POCKETCONSENSUS_SUBSCRIBECANCEL_HPP