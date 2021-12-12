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
        ConsensusValidateResult Validate(const CTransactionRef& tx, const UserRef& ptx, const PocketBlockRef& block) override
        {
            // Base validation with calling block or mempool check
            if (auto[baseValidate, baseValidateCode] = SocialConsensus::Validate(tx, ptx, block); !baseValidate)
                return {false, baseValidateCode};

            // Duplicate name
            if (ConsensusRepoInst.ExistsAnotherByName(*ptx->GetAddress(), *ptx->GetPayloadName()))
            {
                if (!CheckpointRepoInst.IsSocialCheckpoint(*ptx->GetHash(), *ptx->GetType(), SocialConsensusResult_NicknameDouble))
                    return {false, SocialConsensusResult_NicknameDouble};
            }

            // Check payload size
            if (auto[ok, code] = ValidatePayloadSize(ptx); !ok)
                return {false, code};

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

            // TODO (brangr): enable with fork height
            //if (IsEmpty(ptx->GetPayloadName())) return {false, SocialConsensusResult_Failed};

            // Self referring
            if (!IsEmpty(ptx->GetReferrerAddress()) && *ptx->GetAddress() == *ptx->GetReferrerAddress())
                return make_tuple(false, SocialConsensusResult_ReferrerSelf);

            // Maximum length for user name
            auto name = *ptx->GetPayloadName();

            // TODO (brangr): enable with fork height
            // if (name.empty() || name.size() > 35)
            // {
            //     if (!CheckpointRepoInst.IsSocialCheckpoint(*ptx->GetHash(), *ptx->GetType(), SocialConsensusResult_NicknameLong))
            //         return {false, SocialConsensusResult_NicknameLong};
            // }

            // Trim spaces
            if (boost::algorithm::ends_with(name, "%20") || boost::algorithm::starts_with(name, "%20"))
            {
                if (!CheckpointRepoInst.IsSocialCheckpoint(*ptx->GetHash(), *ptx->GetType(),
                    SocialConsensusResult_Failed))
                    return {false, SocialConsensusResult_Failed};
            }

            return Success;
        }

        ConsensusValidateResult CheckOpReturnHash(const CTransactionRef& tx, const UserRef& ptx) override
        {
            auto ptxORHash = ptx->BuildHash();
            auto txORHash = TransactionHelper::ExtractOpReturnHash(tx);

            if (ptxORHash != txORHash)
            {
                auto ptxORHashRef = ptx->BuildHash(false);
                if (ptxORHashRef != txORHash)
                {
                    auto data = ptx->PreBuildHash();
                    LogPrint(BCLog::CONSENSUS, "--- %s\n", data);
                    LogPrint(BCLog::CONSENSUS, "Warning: FailedOpReturn for USER %s: %s | %s != %s\n",
                        *ptx->GetHash(), ptxORHash, ptxORHashRef, txORHash);
                    //if (!CheckpointRepoInst.IsOpReturnCheckpoint(*ptx->GetHash(), ptxORHash))
                    //    return {false, SocialConsensusResult_FailedOpReturn};
                }
            }

            return Success;
        }

    protected:
        ConsensusValidateResult ValidateBlock(const UserRef& ptx, const PocketBlockRef& block) override
        {
            // Only one transaction allowed in block
            for (auto& blockTx: *block)
            {
                if (!TransactionHelper::IsIn(*blockTx->GetType(), {ACCOUNT_USER}))
                    continue;

                auto blockPtx = static_pointer_cast<User>(blockTx);

                if (*blockPtx->GetHash() == *ptx->GetHash())
                    continue;

                if (*ptx->GetAddress() == *blockPtx->GetAddress())
                {
                    if (!CheckpointRepoInst.IsSocialCheckpoint(*ptx->GetHash(), *ptx->GetType(), SocialConsensusResult_ChangeInfoDoubleInBlock))
                        return {false, SocialConsensusResult_ChangeInfoDoubleInBlock};
                }
            }

            if (GetChainCount(ptx) > GetConsensusLimit(ConsensusLimit_edit_user_daily_count))
                return {false, SocialConsensusResult_ChangeInfoLimit};

            return Success;
        }
        ConsensusValidateResult ValidateMempool(const UserRef& ptx) override
        {
            if (ConsensusRepoInst.CountMempoolUser(*ptx->GetAddress()) > 0)
                return {false, SocialConsensusResult_ChangeInfoDoubleInBlock};

            if (GetChainCount(ptx) > GetConsensusLimit(ConsensusLimit_edit_user_daily_count))
                return {false, SocialConsensusResult_ChangeInfoLimit};

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

        virtual ConsensusValidateResult ValidatePayloadSize(const UserRef& ptx)
        {
            size_t dataSize =
                (ptx->GetPayloadName() ? ptx->GetPayloadName()->size() : 0) +
                (ptx->GetPayloadUrl() ? ptx->GetPayloadUrl()->size() : 0) +
                (ptx->GetPayloadLang() ? ptx->GetPayloadLang()->size() : 0) +
                (ptx->GetPayloadAbout() ? ptx->GetPayloadAbout()->size() : 0) +
                (ptx->GetPayloadAvatar() ? ptx->GetPayloadAvatar()->size() : 0) +
                (ptx->GetPayloadDonations() ? ptx->GetPayloadDonations()->size() : 0) +
                (ptx->GetReferrerAddress() ? ptx->GetReferrerAddress()->size() : 0) +
                (ptx->GetPayloadPubkey() ? ptx->GetPayloadPubkey()->size() : 0);

            if (dataSize > (size_t) GetConsensusLimit(ConsensusLimit_max_user_size))
                return {false, SocialConsensusResult_ContentSizeLimit};

            return Success;
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
    *  Start checkpoint at 1381841 block
    *******************************************************************************************************************/
    class UserConsensus_checkpoint_1381841 : public UserConsensus_checkpoint_1180000
    {
    public:
        UserConsensus_checkpoint_1381841(int height) : UserConsensus_checkpoint_1180000(height) {}
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
            { 1381841, 162000, [](int height) { return make_shared<UserConsensus_checkpoint_1381841>(height); }},
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
