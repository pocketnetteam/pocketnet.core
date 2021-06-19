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
    public:
        UserConsensus(int height) : SocialBaseConsensus(height) {}
        UserConsensus() : SocialBaseConsensus() {}


    protected:

        virtual int64_t GetChangeInfoTimeout() { return 3600; }


        tuple<bool, SocialConsensusResult> ValidateModel(shared_ptr <Transaction> tx) override
        {
            auto ptx = static_pointer_cast<User>(tx);

            if (ConsensusRepoInst.ExistsAnotherByName(*ptx->GetAddress(), *ptx->GetPayloadName()))
                return {false, SocialConsensusResult_NicknameDouble};

            // First user account transaction allowed without next checks
            auto prevTx = ConsensusRepoInst.GetLastAccountTransaction(*ptx->GetAddress());
            if (!prevTx)
                return Success;

            // We allow edit profile only with delay
            if (!ValidateEditProfileLimit(ptx, static_pointer_cast<User>(prevTx)))
                return {false, SocialConsensusResult_ChangeInfoLimit};

            // For edit user profile referrer not allowed
            if (ptx->GetReferrerAddress() != nullptr)
                return {false, SocialConsensusResult_ReferrerAfterRegistration};

            return Success;
        }

        virtual bool ValidateEditProfileLimit(shared_ptr <User> tx, shared_ptr <User> prevTx)
        {
            return (*tx->GetTime() - *prevTx->GetTime()) > GetChangeInfoTimeout();
        }

        tuple<bool, SocialConsensusResult> ValidateLimit(shared_ptr <Transaction> tx, const PocketBlock& block) override
        {
            auto ptx = static_pointer_cast<User>(tx);



            // if (blockVtx.Exists("Users")) {
            //     for (auto& mtx : blockVtx.Data["Users"]) {
            //         if (mtx["address"].get_str() == _address && mtx["txid"].get_str() != _txid) {
            //             result = ANTIBOTRESULT::ChangeInfoLimit;
            //             return false;
            //         }
            //     }
            // }
        }

        tuple<bool, SocialConsensusResult> ValidateLimit(shared_ptr <Transaction> tx) override
        {
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
        }


        tuple<bool, SocialConsensusResult> CheckOpReturnHash(shared_ptr <Transaction> tx) override
        {
            // TODO (brangr): implement
            if (auto[baseCheckOk, baseCheckResult] = SocialBaseConsensus::CheckOpReturnHash(tx); !baseCheckOk)
            {
                //     if (table != "Users" || (table == "Users" && vasm[2] != oitm["data_hash_without_ref"].get_str())) {
                //         LogPrintf("FailedOpReturn vasm: %s - oitm: %s\n", vasm[2], oitm.write());
                //         resultCode = ANTIBOTRESULT::FailedOpReturn;
                //         return;
                //     }
            }
        }

        // Check op_return hash
        tuple<bool, SocialConsensusResult> CheckModel(shared_ptr <Transaction> tx) override
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
    *  Start checkpoint at 1180000 block
    *
    *******************************************************************************************************************/
    class UserConsensus_checkpoint_1180000 : public UserConsensus
    {
    protected:
        int CheckpointHeight() override { return 1180000; }

        bool ValidateEditProfileLimit(shared_ptr <User> tx, shared_ptr <User> prevTx) override
        {
            //return (*tx->GetHeight() - *prevTx->GetHeight()) > GetChangeInfoTimeout();
            // TODO (brangr): implement
        }

    public:
        UserConsensus_checkpoint_1180000(int height) : UserConsensus(height) {}
    };

    /*******************************************************************************************************************
    *
    *  Start checkpoint at ?? block
    *
    *******************************************************************************************************************/
    class UserConsensus_checkpoint_ : public UserConsensus
    {
    protected:
        int CheckpointHeight() override { return 0; }

        // TODO (brangr): Starting from this block, we disable the uniqueness of Name
        virtual tuple<bool, SocialConsensusResult> CheckDoubleName(shared_ptr <User> tx)
        {
            return make_tuple(true, SocialConsensusResult_Success);
        }

    public:
        UserConsensus_checkpoint_(int height) : UserConsensus(height) {}
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
                {1180000, [](int height) { return new UserConsensus(height); }},
                {0,       [](int height) { return new UserConsensus(height); }},
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
