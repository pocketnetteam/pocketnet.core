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

        virtual tuple<bool, SocialConsensusResult> CheckDoubleName(shared_ptr<User> tx)
        {
            // TODO (brangr): implement
            // if (g_pocketdb->SelectCount(reindexer::Query("UsersView").Where("name", CondEq, _name).Not().Where("address", CondEq, _address).Where("block", CondLt, height)) > 0) {
            //     result = ANTIBOTRESULT::NicknameDouble;
            //     return false;
            // }
        }

        virtual tuple<bool, SocialConsensusResult> CheckEditProfileLimit(shared_ptr<User> tx)
        {
            // TODO (brangr): мы должны убедить что последнее редактирование было в пределах N секунд или блоков
            // нужна перегрузка
            // также для совместимости со старой логикой все транзакции редактирования профиля не должны
            // содержать поле реферрера

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
        }

        virtual tuple<bool, SocialConsensusResult> CheckEditProfileBlockLimit(shared_ptr<User> tx, PocketBlock& block)
        {
            // TODO (brangr): Need?
            // по идее мемпул лежит в базе на равне с остальными транзакциями
            // но сюда может насыпаться очень много всего и мы не можем проверить
            // двойное редактирование, тк при загрузке из сети свободных транзакций нет привязки к блоку
            // и поэтому либо разрешаем все такие транзакции либо думаем дальше

            // TODO (brangr): проверка по времени в мемпуле - по идее не нужна

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

            // TODO (brangr): implement
            // if (blockVtx.Exists("Users")) {
            //     for (auto& mtx : blockVtx.Data["Users"]) {
            //         if (mtx["address"].get_str() == _address && mtx["txid"].get_str() != _txid) {
            //             result = ANTIBOTRESULT::ChangeInfoLimit;
            //             return false;
            //         }
            //     }
            // }

        }

    public:
        UserConsensus(int height) : SocialBaseConsensus(height) {}
        UserConsensus() : SocialBaseConsensus() {}

        tuple<bool, SocialConsensusResult> Validate(shared_ptr<User> tx, PocketBlock& block)
        {
            if (auto[ok, result] = CheckDoubleName(tx); !ok)
                return make_tuple(false, result);

            if (auto[ok, result] = CheckDoubleName(tx); !ok)
                return make_tuple(false, result);
            
            if (auto[ok, result] = CheckDoubleName(tx); !ok)
                return make_tuple(false, result);

            return make_tuple(true, SocialConsensusResult_Success);
        }

        tuple<bool, SocialConsensusResult> Check(shared_ptr<User> tx)
        {
            // TODO (brangr): implement for users
            // if (*tx->GetAddress() == *tx->GetReferrerAddress())
            //     return make_tuple(false, SocialConsensusResult_ReferrerSelf);

            // TODO (brangr): implement for users
            // if (_name.size() < 1 && _name.size() > 35) {
            //     result = ANTIBOTRESULT::NicknameLong;
            //     return false;
            // }

            // TODO (brangr): implement for users
            // if (boost::algorithm::ends_with(_name, "%20") || boost::algorithm::starts_with(_name, "%20")) {
            //     result = ANTIBOTRESULT::Failed;
            //     return false;
            // }
        }

    };

    /*******************************************************************************************************************
    *
    *  Start checkpoint
    *
    *******************************************************************************************************************/
    class UserConsensus_checkpoint_1 : public UserConsensus
    {
    protected:
        int CheckpointHeight() override { return 1; }

        virtual tuple<bool, SocialConsensusResult> CheckDoubleName(shared_ptr<User> tx)
        {
            return make_tuple(true, SocialConsensusResult_Success);
        }

    public:

        UserConsensus_checkpoint_1(int height) : UserConsensus(height) {}

    };

    /*******************************************************************************************************************
    *
    *  Factory for select actual rules version
    *
    *******************************************************************************************************************/
    class UserConsensusFactory
    {
    private:
        static inline const std::map<int, std::function<UserConsensus*(int height)>> m_rules =
        {
            {0, [](int height) { return new UserConsensus(height); }},
        };
    public:
        shared_ptr <UserConsensus> Instance(int height)
        {
            return shared_ptr<UserConsensus>(
                (--m_rules.upper_bound(height))->second(height)
            );
        }

        shared_ptr <UserConsensus> Instance()
        {
            return shared_ptr<UserConsensus>(new UserConsensus());
        }
    };
}

#endif // POCKETCONSENSUS_USER_HPP
