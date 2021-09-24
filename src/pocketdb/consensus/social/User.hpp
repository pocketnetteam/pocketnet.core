// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETCONSENSUS_USER_HPP
#define POCKETCONSENSUS_USER_HPP

#include "pocketdb/consensus/Social.h"
#include "pocketdb/models/dto/User.h"

namespace PocketConsensus
{
    using namespace std;
    typedef shared_ptr<User> UserRef;

    /*******************************************************************************************************************
    *  User consensus base class
    *******************************************************************************************************************/
    class UserConsensus : public SocialConsensus<User>
    {
    public:
        UserConsensus(int height) : SocialConsensus<User>(height) {}
        ConsensusValidateResult Validate(const UserRef& ptx, const PocketBlockRef& block) override
        {
            // Base validation with calling block or mempool check
            if (auto[baseValidate, baseValidateCode] = SocialConsensus::Validate(ptx, block); !baseValidate)
                return {false, baseValidateCode};

            if (ConsensusRepoInst.ExistsAnotherByName(*ptx->GetAddress(), *ptx->GetPayloadName()))
            {
                PocketHelpers::SocialCheckpoints socialCheckpoints;
                if (!socialCheckpoints.IsCheckpoint(*ptx->GetHash(), *ptx->GetType(),
                    SocialConsensusResult_NicknameDouble))
                    return {false, SocialConsensusResult_NicknameDouble};
            }

            return ValidateEdit(ptx);
        }
        ConsensusValidateResult Check(const CTransactionRef& tx, const UserRef& ptx) override
        {
            if (auto[baseCheck, baseCheckCode] = SocialConsensus::Check(tx, ptx); !baseCheck)
                return {false, baseCheckCode};

            // Check required fields
            if (IsEmpty(ptx->GetAddress())) return {false, SocialConsensusResult_Failed};

            // Check payload
            if (!ptx->GetPayload()) return {false, SocialConsensusResult_Failed};
            if (IsEmpty(ptx->GetPayloadName())) return {false, SocialConsensusResult_Failed};

            // Self referring
            if (!IsEmpty(ptx->GetReferrerAddress()) && *ptx->GetAddress() == *ptx->GetReferrerAddress())
                return make_tuple(false, SocialConsensusResult_ReferrerSelf);

            // Maximum length for user name
            auto name = *ptx->GetPayloadName();
            if (name.empty() || name.size() > 35)
            {
                PocketHelpers::SocialCheckpoints socialCheckpoints;
                if (!socialCheckpoints.IsCheckpoint(*ptx->GetHash(), *ptx->GetType(),
                    SocialConsensusResult_NicknameLong))
                    return {false, SocialConsensusResult_NicknameLong};
            }

            // Trim spaces
            if (boost::algorithm::ends_with(name, "%20") || boost::algorithm::starts_with(name, "%20"))
            {
                PocketHelpers::SocialCheckpoints socialCheckpoints;
                if (!socialCheckpoints.IsCheckpoint(*ptx->GetHash(), *ptx->GetType(), SocialConsensusResult_Failed))
                    return {false, SocialConsensusResult_Failed};
            }

            return Success;
        }

    protected:
        ConsensusValidateResult ValidateBlock(const UserRef& ptx, const PocketBlockRef& block) override
        {
            // Only one transaction allowed in block
            for (auto& blockTx: *block)
            {
                if (!IsIn(*blockTx->GetType(), {ACCOUNT_USER}))
                    continue;

                auto blockPtx = static_pointer_cast<User>(blockTx);

                if (*blockPtx->GetHash() == *ptx->GetHash())
                    continue;

                if (*ptx->GetAddress() == *blockPtx->GetAddress())
                {
                    PocketHelpers::SocialCheckpoints socialCheckpoints;
                    if (!socialCheckpoints.IsCheckpoint(*ptx->GetHash(), *ptx->GetType(),
                        SocialConsensusResult_ChangeInfoDoubleInBlock))
                        return {false, SocialConsensusResult_ChangeInfoDoubleInBlock};
                }
            }

            if (GetChainCount(ptx) > GetConsensusLimit(ConsensusLimit_edit_user_limit))
                return {false, SocialConsensusResult_ChangeInfoLimit};

            return Success;
        }
        ConsensusValidateResult ValidateMempool(const UserRef& ptx) override
        {
            if (ConsensusRepoInst.CountMempoolUser(*ptx->GetAddress()) > 0)
                return {false, SocialConsensusResult_ChangeInfoDoubleInBlock};

            return Success;
        }
        vector<string> GetAddressesForCheckRegistration(const UserRef& ptx) override
        {
            return {};
        }

        virtual ConsensusValidateResult ValidateEdit(const UserRef& ptx)
        {
            // First user account transaction allowed without next checks
            if (auto[ok, prevTxHeight] = ConsensusRepoInst.GetLastAccountHeight(*ptx->GetAddress()); !ok)
                return Success;

            // Check editing limits
            if (auto[ok, code] = ValidateEditLimit(ptx); !ok)
                return {false, code};

            // For edit user profile referrer not allowed
            if (ptx->GetReferrerAddress() != nullptr)
                return {false, SocialConsensusResult_ReferrerAfterRegistration};

            return Success;
        }

        virtual ConsensusValidateResult ValidateEditLimit(const UserRef& ptx)
        {
            // First user account transaction allowed without next checks
            auto[prevOk, prevTx] = ConsensusRepoInst.GetLastAccount(*ptx->GetAddress());
            if (!prevOk)
                return Success;

            // We allow edit profile only with delay
            if ((*ptx->GetTime() - *prevTx->GetTime()) <= GetConsensusLimit(ConsensusLimit_edit_user_depth))
                return {false, SocialConsensusResult_ChangeInfoLimit};

            return Success;
        }

        virtual int GetChainCount(const UserRef& ptx)
        {
            return 0;
        }
    };

    /*******************************************************************************************************************
    *  Start checkpoint at 1180000 block
    *******************************************************************************************************************/
    class UserConsensus_checkpoint_1180000 : public UserConsensus
    {
    public:
        UserConsensus_checkpoint_1180000(int height) : UserConsensus(height) {}
    protected:
        ConsensusValidateResult ValidateEditLimit(const UserRef& ptx) override
        {
            // First user account transaction allowed without next checks
            auto[ok, prevTxHeight] = ConsensusRepoInst.GetLastAccountHeight(*ptx->GetAddress());
            if (!ok) return Success;

            // We allow edit profile only with delay
            if ((Height - prevTxHeight) <= GetConsensusLimit(ConsensusLimit_edit_user_depth))
                return {false, SocialConsensusResult_ChangeInfoLimit};

            return Success;
        }
    };

    /*******************************************************************************************************************
    *  Start checkpoint at 9999999 block
    *******************************************************************************************************************/
    // TODO (brangr): set checkpoint after v0.19.14
    class UserConsensus_checkpoint_9999999 : public UserConsensus_checkpoint_1180000
    {
    public:
        UserConsensus_checkpoint_9999999(int height) : UserConsensus_checkpoint_1180000(height) {}
    protected:
        ConsensusValidateResult ValidateEditLimit(const UserRef& ptx) override
        {
            return Success;
        }
        int GetChainCount(const UserRef& ptx) override
        {
            return ConsensusRepoInst.CountChainAccount(
                *ptx->GetType(),
                *ptx->GetAddress(),
                Height - (int)GetConsensusLimit(ConsensusLimit_depth)
            );
        }
    };

    /*******************************************************************************************************************
    *  Factory for select actual rules version
    *******************************************************************************************************************/
    class UserConsensusFactory
    {
    private:
        const vector<ConsensusCheckpoint < UserConsensus>> m_rules = {
            { 0, -1, [](int height) { return make_shared<UserConsensus>(height); }},
            { 1180000, 0, [](int height) { return make_shared<UserConsensus_checkpoint_1180000>(height); }},
            { 9999999, 162000, [](int height) { return make_shared<UserConsensus_checkpoint_9999999>(height); }},
        };
    public:
        shared_ptr<UserConsensus> Instance(int height)
        {
            int m_height = (height > 0 ? height : 0);
            return (--upper_bound(m_rules.begin(), m_rules.end(), m_height,
                [&](int target, const ConsensusCheckpoint<UserConsensus>& itm)
                {
                    return target < itm.Height(Params().NetworkIDString());
                }
            ))->m_func(height);
        }
    };

} // namespace PocketConsensus

#endif // POCKETCONSENSUS_USER_HPP
