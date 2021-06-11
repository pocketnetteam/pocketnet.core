// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 Bitcoin developers
// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETCONSENSUS_BLOCKING_HPP
#define POCKETCONSENSUS_BLOCKING_HPP

#include "pocketdb/consensus/social/Base.hpp"
#include "pocketdb/models/dto/Blocking.hpp"

namespace PocketConsensus
{
    /*******************************************************************************************************************
    *
    *  Blocking consensus base class
    *
    *******************************************************************************************************************/
    class BlockingConsensus : public SocialBaseConsensus
    {
    protected:
    public:
        BlockingConsensus(int height) : SocialBaseConsensus(height) {}

        tuple<bool, SocialConsensusResult> Validate(shared_ptr<Blocking> tx, PocketBlock& block)
        {
            // Check registration account "from"
            if (auto[ok, result] = CheckRegistration(tx->GetAddress()); !ok)
                return make_tuple(false, SocialConsensusResult_NotRegistered);

            // Check registration account "to"
            if (auto[ok, result] = CheckRegistration(tx->GetAddressTo()); !ok)
                return make_tuple(false, SocialConsensusResult_NotRegistered);

            // Blocking self
            if (*tx->GetAddress() == *tx->GetAddressTo())
                return make_tuple(false, SocialConsensusResult_SelfBlocking);

            // TODO (brangr): implement
            // TODO (brangr): Check already exists blocking for "from -> to"
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

            // Check block
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

            make_tuple(true, SocialConsensusResult_Success);
        }

    };

    /*******************************************************************************************************************
        *
        *  Factory for select actual rules version
        *  Каждая новая перегрузка добавляет новый функционал, поддерживающийся с некоторым условием - например высота
        *
        *******************************************************************************************************************/
    class BlockingConsensusFactory
    {
    private:
        static inline const std::map<int, std::function<BlockingConsensus*(int height)>> m_rules =
        {
            {0, [](int height) { return new BlockingConsensus(height); }},
        };

    public:
        shared_ptr <BlockingConsensus> Instance(int height)
        {
            return shared_ptr<BlockingConsensus>(
                (--m_rules.upper_bound(height))->second(height)
            );
        }
    };
}

#endif // POCKETCONSENSUS_BLOCKING_HPP