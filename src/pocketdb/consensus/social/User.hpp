// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 Bitcoin developers
// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETCONSENSUS_USER_HPP
#define POCKETCONSENSUS_USER_HPP

#include "pocketdb/consensus/social/Base.hpp"
#include "pocketdb/models/base/Transaction.hpp"
#include "pocketdb/models/dto/User.hpp"

namespace PocketConsensus
{
    /*******************************************************************************************************************
    *
    *  User consensus base class
    *
    *******************************************************************************************************************/
    class UserConsensus : public SocialBaseConsensus
    {
    protected:
    public:
        UserConsensus(int height) : SocialBaseConsensus(height) {}

        tuple<bool, SocialConsensusResult> Validate(shared_ptr<Transaction> tx, PocketBlock& block) override
        {
            // TODO (brangr): implement
            // std::string _txid = oitm["txid"].get_str();
            // std::string _address = oitm["address"].get_str();
            // std::string _address_referrer = oitm["referrer"].get_str();
            // std::string _name = oitm["name"].get_str();
            // int64_t _time = oitm["time"].get_int64();

            // // Referrer to self? Seriously?;)
            // if (_address == _address_referrer) {
            //     result = ANTIBOTRESULT::ReferrerSelf;
            //     return false;
            // }

            // // Get last updated item
            // reindexer::Item userItm;
            // if (g_pocketdb->SelectOne(
            //                 reindexer::Query("UsersView")
            //                     .Where("address", CondEq, _address)
            //                     .Where("block", CondLt, height),
            //                 userItm)
            //         .ok()) {
            //     if (!_address_referrer.empty()) {
            //         result = ANTIBOTRESULT::ReferrerAfterRegistration;
            //         return false;
            //     }

            //     auto userUpdateTime = userItm["time"].As<int64_t>();
            //     if (_time - userUpdateTime <= GetActualLimit(Limit::change_info_timeout, height)) {
            //         result = ANTIBOTRESULT::ChangeInfoLimit;
            //         return false;
            //     }
            // }

            // // Also check mempool
            // if (checkMempool) {
            //     reindexer::QueryResults res;
            //     if (g_pocketdb->Select(reindexer::Query("Mempool").Where("table", CondEq, "Users").Not().Where("txid", CondEq, _txid), res).ok()) {
            //         for (auto& m : res) {
            //             reindexer::Item mItm = m.GetItem();
            //             std::string t_src = DecodeBase64(mItm["data"].As<string>());

            //             reindexer::Item t_itm = g_pocketdb->DB()->NewItem("Users");
            //             if (t_itm.FromJSON(t_src).ok()) {
            //                 if (t_itm["address"].As<string>() == _address) {
            //                     if (!checkWithTime || t_itm["time"].As<int64_t>() <= _time) {
            //                         result = ANTIBOTRESULT::ChangeInfoLimit;
            //                         return false;
            //                     }
            //                 }
            //             }
            //         }
            //     }
            // }

            // // Check block
            // if (blockVtx.Exists("Users")) {
            //     for (auto& mtx : blockVtx.Data["Users"]) {
            //         if (mtx["address"].get_str() == _address && mtx["txid"].get_str() != _txid) {
            //             result = ANTIBOTRESULT::ChangeInfoLimit;
            //             return false;
            //         }
            //     }
            // }

            // // Check long nickname
            // if (_name.size() < 1 && _name.size() > 35) {
            //     result = ANTIBOTRESULT::NicknameLong;
            //     return false;
            // }

            // // Check double nickname
            // if (g_pocketdb->SelectCount(reindexer::Query("UsersView").Where("name", CondEq, _name).Not().Where("address", CondEq, _address).Where("block", CondLt, height)) > 0) {
            //     result = ANTIBOTRESULT::NicknameDouble;
            //     return false;
            // }

            // // TODO (brangr): block all spaces
            // // Check spaces in begin and end
            // if (boost::algorithm::ends_with(_name, "%20") || boost::algorithm::starts_with(_name, "%20")) {
            //     result = ANTIBOTRESULT::Failed;
            //     return false;
            // }

            // return true;
        }

    };

    /*******************************************************************************************************************
    *
    *  Start checkpoint
    *
    *******************************************************************************************************************/
    class UserConsensus_checkpoint_0 : public UserConsensus
    {
    protected:
    public:

        UserConsensus_checkpoint_0(int height) : UserConsensus(height) {}

    }; // class UserConsensus_checkpoint_0

    /*******************************************************************************************************************
    *
    *  Consensus checkpoint at 1 block
    *
    *******************************************************************************************************************/
    class UserConsensus_checkpoint_1 : public UserConsensus_checkpoint_0
    {
    protected:
        int CheckpointHeight() override { return 1; }
    public:
        UserConsensus_checkpoint_1(int height) : UserConsensus_checkpoint_0(height) {}
    };

    /*******************************************************************************************************************
    *
    *  Factory for select actual rules version
    *  Каждая новая перегрузка добавляет новый функционал, поддерживающийся с некоторым условием - например высота
    *
    *******************************************************************************************************************/
    class UserConsensusFactory
    {
    private:
        inline static std::vector<std::pair<int, std::function<UserConsensus*(int height)>>> m_rules
            {
                {1, [](int height) { return new UserConsensus_checkpoint_1(height); }},
                {0, [](int height) { return new UserConsensus_checkpoint_0(height); }},
            };
    public:
        shared_ptr <UserConsensus> Instance(int height)
        {
            for (const auto& rule : m_rules)
                if (height >= rule.first)
                    return shared_ptr<UserConsensus>(rule.second(height));
        }
    };
}

#endif // POCKETCONSENSUS_USER_HPP
