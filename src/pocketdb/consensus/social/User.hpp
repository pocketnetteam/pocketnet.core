// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETCONSENSUS_USER_HPP
#define POCKETCONSENSUS_USER_HPP

#include "pocketdb/consensus/Social.hpp"
#include "pocketdb/models/base/Transaction.hpp"
#include "pocketdb/models/dto/User.hpp"

namespace PocketConsensus
{
    /*******************************************************************************************************************
    *  User consensus base class
    *******************************************************************************************************************/
    class UserConsensus : public SocialConsensus
    {
    public:
        UserConsensus(int height) : SocialConsensus(height) {}

        ConsensusValidateResult Validate(const PTransactionRef& ptx, const PocketBlockRef& block) override
        {
            auto _ptx = static_pointer_cast<User>(ptx);

            // Base validation with calling block or mempool check
            if (auto[baseValidate, baseValidateCode] = SocialConsensus::Validate(_ptx, block); !baseValidate)
                return {false, baseValidateCode};
                
            // TODO (brangr) (v0.21.0): unique names disabled in future
            if (ConsensusRepoInst.ExistsAnotherByName(*_ptx->GetAddress(), *_ptx->GetPayloadName()))
            {
                PocketHelpers::SocialCheckpoints socialCheckpoints;
                if (!socialCheckpoints.IsCheckpoint(*_ptx->GetHash(), *_ptx->GetType(), SocialConsensusResult_NicknameDouble))
                    return {false, SocialConsensusResult_NicknameDouble};
            }

            return ValidateEdit(_ptx);
        }

        ConsensusValidateResult Check(const CTransactionRef& tx, const PTransactionRef& ptx) override
        {
            auto _ptx = static_pointer_cast<User>(ptx);

            if (auto[baseCheck, baseCheckCode] = SocialConsensus::Check(tx, _ptx); !baseCheck)
                return {false, baseCheckCode};

            // Check required fields
            if (IsEmpty(_ptx->GetAddress())) return {false, SocialConsensusResult_Failed};

            // Check payload
            if (!_ptx->GetPayload()) return {false, SocialConsensusResult_Failed};
            if (IsEmpty(_ptx->GetPayloadName())) return {false, SocialConsensusResult_Failed};
            if (IsEmpty(_ptx->GetPayloadAvatar())) return {false, SocialConsensusResult_Failed};

            // Self referring
            if (!IsEmpty(_ptx->GetReferrerAddress()) && *_ptx->GetAddress() == *_ptx->GetReferrerAddress())
                return make_tuple(false, SocialConsensusResult_ReferrerSelf);

            // Maximum length for user name
            auto name = *_ptx->GetPayloadName();
            if (name.empty() || name.size() > 35)
            {
                PocketHelpers::SocialCheckpoints socialCheckpoints;
                if (!socialCheckpoints.IsCheckpoint(*_ptx->GetHash(), *_ptx->GetType(), SocialConsensusResult_NicknameLong))
                    return {false, SocialConsensusResult_NicknameLong};
            }

            // Trim spaces
            if (boost::algorithm::ends_with(name, "%20") || boost::algorithm::starts_with(name, "%20"))
            {
                PocketHelpers::SocialCheckpoints socialCheckpoints;
                if (!socialCheckpoints.IsCheckpoint(*_ptx->GetHash(), *_ptx->GetType(), SocialConsensusResult_Failed))
                    return {false, SocialConsensusResult_Failed};
            }

            return Success;
        }


    protected:
        virtual int64_t GetChangeInfoDepth() { return 3600; }


        virtual ConsensusValidateResult ValidateEdit(const PTransactionRef& ptx)
        {
            auto _ptx = static_pointer_cast<User>(ptx);

            // First user account transaction allowed without next checks
            auto[prevOk, prevTx] = ConsensusRepoInst.GetLastAccount(*_ptx->GetAddress());
            if (!prevOk)
                return Success;

            // We allow edit profile only with delay
            if ((*_ptx->GetTime() - *prevTx->GetTime()) <= GetChangeInfoDepth())
                return {false, SocialConsensusResult_ChangeInfoLimit};

            // For edit user profile referrer not allowed
            if (_ptx->GetReferrerAddress() != nullptr)
                return {false, SocialConsensusResult_ReferrerAfterRegistration};

            return Success;
        }

        ConsensusValidateResult ValidateBlock(const PTransactionRef& ptx, const PocketBlockRef& block) override
        {
            auto _ptx = static_pointer_cast<User>(ptx);

            // Only one transaction allowed in block
            for (auto& blockTx : *block)
            {
                if (!IsIn(*blockTx->GetType(), {ACCOUNT_USER}))
                    continue;

                if (*blockTx->GetHash() == *ptx->GetHash())
                    continue;

                if (*_ptx->GetAddress() == *blockTx->GetString1())
                {
                    PocketHelpers::SocialCheckpoints socialCheckpoints;
                    if (!socialCheckpoints.IsCheckpoint(*_ptx->GetHash(), *_ptx->GetType(), SocialConsensusResult_ChangeInfoLimit))
                        return {false, SocialConsensusResult_ChangeInfoLimit};
                }
            }

            return Success;
        }

        ConsensusValidateResult ValidateMempool(const PTransactionRef& ptx) override
        {
            auto _ptx = static_pointer_cast<User>(ptx);
            if (ConsensusRepoInst.CountMempoolUser(*_ptx->GetAddress()) > 0)
                return {false, SocialConsensusResult_ChangeInfoLimit};

            return Success;
        }

        vector<string> GetAddressesForCheckRegistration(const PTransactionRef& ptx) override
        {
            return {};
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
        int64_t GetChangeInfoDepth() override { return 60; }
        ConsensusValidateResult ValidateEdit(const PTransactionRef& ptx) override
        {
            auto _ptx = static_pointer_cast<User>(ptx);

            // First user account transaction allowed without next checks
            auto[ok, prevTxHeight] = ConsensusRepoInst.GetLastAccountHeight(*_ptx->GetAddress());
            if (!ok) return Success;

            // We allow edit profile only with delay
            if ((Height - prevTxHeight) <= GetChangeInfoDepth())
                return {false, SocialConsensusResult_ChangeInfoLimit};

            // For edit user profile referrer not allowed
            if (_ptx->GetReferrerAddress() != nullptr)
                return {false, SocialConsensusResult_ReferrerAfterRegistration};

            return Success;
        }
    };

    /*******************************************************************************************************************
    *  Start checkpoint at ?? block
    *******************************************************************************************************************/
    class UserConsensus_checkpoint_ : public UserConsensus_checkpoint_1180000
    {
    public:
        UserConsensus_checkpoint_(int height) : UserConsensus_checkpoint_1180000(height) {}
    protected:
        // TODO (brangr) (v0.21.0): Starting from this block, we disable the uniqueness of Name
        virtual ConsensusValidateResult CheckDoubleName(const PTransactionRef& ptx)
        {
            return make_tuple(true, SocialConsensusResult_Success);
        }
    };

    /*******************************************************************************************************************
    *  Factory for select actual rules version
    *******************************************************************************************************************/
    class UserConsensusFactory : public SocialConsensusFactory
    {
    private:
        const vector<ConsensusCheckpoint> _rules = {
            {0,       -1, [](int height) { return make_shared<UserConsensus>(height); }},
            {1180000, 0,  [](int height) { return make_shared<UserConsensus_checkpoint_1180000>(height); }},
        };
    protected:
        const vector<ConsensusCheckpoint>& m_rules() override { return _rules; }
    };
} // namespace PocketConsensus

#endif // POCKETCONSENSUS_USER_HPP
