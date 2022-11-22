// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETCONSENSUS_USER_HPP
#define POCKETCONSENSUS_USER_HPP

#include <boost/algorithm/string.hpp>

#include "pocketdb/consensus/Social.h"
#include "pocketdb/models/dto/account/User.h"

namespace PocketConsensus
{
    using namespace std;
    typedef shared_ptr<User> UserRef;

    /*******************************************************************************************************************
    *  AccountUser consensus base class
    *******************************************************************************************************************/
    class AccountUserConsensus : public SocialConsensus<User>
    {
    public:
        AccountUserConsensus(int height) : SocialConsensus<User>(height) {}
        
        ConsensusValidateResult Validate(const CTransactionRef& tx, const UserRef& ptx, const PocketBlockRef& block) override
        {
            // Check payload size
            if (auto[ok, code] = ValidatePayloadSize(ptx); !ok)
                return {false, code};

            // Duplicate name
            if (ConsensusRepoInst.ExistsAnotherByName(*ptx->GetAddress(), *ptx->GetPayloadName()))
            {
                if (!CheckpointRepoInst.IsSocialCheckpoint(*ptx->GetHash(), *ptx->GetType(), SocialConsensusResult_NicknameDouble))
                    return {false, SocialConsensusResult_NicknameDouble};
            }

            // The deleted account cannot be restored
            if (auto[ok, type] = ConsensusRepoInst.GetLastAccountType(*ptx->GetAddress()); ok)
                if (type == TxType::ACCOUNT_DELETE)
                    return {false, SocialConsensusResult_AccountDeleted};

            return SocialConsensus::Validate(tx, ptx, block);
        }

        ConsensusValidateResult Check(const CTransactionRef& tx, const UserRef& ptx) override
        {
            if (auto[baseCheck, baseCheckCode] = SocialConsensus::Check(tx, ptx); !baseCheck)
                return {false, baseCheckCode};

            // Check required fields
            if (IsEmpty(ptx->GetAddress())) return {false, SocialConsensusResult_Failed};

            // Check payload
            if (!ptx->GetPayload()) return {false, SocialConsensusResult_Failed};

            // Self referring
            if (!IsEmpty(ptx->GetReferrerAddress()) && *ptx->GetAddress() == *ptx->GetReferrerAddress())
                return make_tuple(false, SocialConsensusResult_ReferrerSelf);

            // Name check
            if (auto[ok, result] = CheckLogin(ptx); !ok)
                return {false, result};

            return Success;
        }

        ConsensusValidateResult CheckOpReturnHash(const CTransactionRef& tx, const UserRef& ptx) override
        {
            return Success;
        }

    protected:

        ConsensusValidateResult ValidateBlock(const UserRef& ptx, const PocketBlockRef& block) override
        {
            // Only one transaction allowed in block
            for (auto& blockTx: *block)
            {
                if (!TransactionHelper::IsIn(*blockTx->GetType(), { ACCOUNT_USER, ACCOUNT_DELETE }))
                    continue;

                if (*blockTx->GetHash() == *ptx->GetHash())
                    continue;

                if (*ptx->GetAddress() == *blockTx->GetString1())
                {
                    if (!CheckpointRepoInst.IsSocialCheckpoint(*ptx->GetHash(), *ptx->GetType(), SocialConsensusResult_ChangeInfoDoubleInBlock))
                        return {false, SocialConsensusResult_ChangeInfoDoubleInBlock};
                }

                if (TransactionHelper::IsIn(*blockTx->GetType(), { ACCOUNT_USER }))
                {
                    auto blockPtx = static_pointer_cast<User>(blockTx);
                    if (auto[ok, result] = ValidateBlockDuplicateName(ptx, blockPtx); !ok)
                        return {false, result};
                }
            }

            if (GetChainCount(ptx) > GetConsensusLimit(ConsensusLimit_edit_user_daily_count))
                return {false, SocialConsensusResult_ChangeInfoLimit};

            return Success;
        }

        ConsensusValidateResult ValidateMempool(const UserRef& ptx) override
        {
            if (ConsensusRepoInst.ExistsInMempool(*ptx->GetAddress(), { ACCOUNT_USER, ACCOUNT_DELETE }))
                return {false, SocialConsensusResult_ChangeInfoDoubleInMempool};

            if (GetChainCount(ptx) > GetConsensusLimit(ConsensusLimit_edit_user_daily_count))
                return {false, SocialConsensusResult_ChangeInfoLimit};

            return Success;
        }
        vector<string> GetAddressesForCheckRegistration(const UserRef& ptx) override
        {
            return { };
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
    
        virtual ConsensusValidateResult CheckLogin(const UserRef& ptx)
        {
            auto name = *ptx->GetPayloadName();

            // Trim spaces
            if (boost::algorithm::ends_with(name, "%20") || boost::algorithm::starts_with(name, "%20"))
            {
                if (!CheckpointRepoInst.IsSocialCheckpoint(*ptx->GetHash(), *ptx->GetType(),
                    SocialConsensusResult_Failed))
                    return {false, SocialConsensusResult_Failed};
            }

            return Success;
        }
    
        virtual ConsensusValidateResult ValidateBlockDuplicateName(const UserRef& ptx, const UserRef& blockPtx)
        {
            return Success;
        }
    };

    class AccountUserConsensus_checkpoint_chain_count : public AccountUserConsensus
    {
    public:
        AccountUserConsensus_checkpoint_chain_count(int height) : AccountUserConsensus(height) {}
    protected:
        int GetChainCount(const UserRef& ptx) override
        {
            return ConsensusRepoInst.CountChainAccount(
                *ptx->GetType(),
                *ptx->GetAddress(),
                Height - (int)GetConsensusLimit(ConsensusLimit_depth)
            );
        }
    };

    class AccountUserConsensus_checkpoint_login_limitation : public AccountUserConsensus_checkpoint_chain_count
    {
    public:
        AccountUserConsensus_checkpoint_login_limitation(int height) : AccountUserConsensus_checkpoint_chain_count(height) {}

    protected:
        ConsensusValidateResult CheckLogin(const UserRef& ptx) override
        {
            if (IsEmpty(ptx->GetPayloadName()))
                return {false, SocialConsensusResult_Failed};

            auto name = *ptx->GetPayloadName();
            boost::algorithm::to_lower(name);

            if (name.size() > 20)
                return {false, SocialConsensusResult_NicknameLong};
            
            if (!all_of(name.begin(), name.end(), [](unsigned char ch) { return ::isalnum(ch) || ch == '_'; }))
                return {false, SocialConsensusResult_Failed};

            return Success;
        }
        
        // TODO (brangr): move to base class after this checkpoint - set test checkpoint records
        ConsensusValidateResult ValidateBlockDuplicateName(const UserRef& ptx, const UserRef& blockPtx) override
        {
            auto ptxName = ptx->GetPayloadName() ? *ptx->GetPayloadName() : "";
            boost::algorithm::to_lower(ptxName);

            auto blockPtxName = blockPtx->GetPayloadName() ? *blockPtx->GetPayloadName() : "";
            boost::algorithm::to_lower(blockPtxName);

            if (ptxName == blockPtxName)
                if (!CheckpointRepoInst.IsSocialCheckpoint(*ptx->GetHash(), *ptx->GetType(), SocialConsensusResult_NicknameDouble))
                    return {false, SocialConsensusResult_NicknameDouble};

            return Success;
        }

        // TODO (brangr): move to base class after this checkpoint - set test checkpoint records
        ConsensusValidateResult CheckOpReturnHash(const CTransactionRef& tx, const UserRef& ptx) override
        {
            auto ptxORHash = ptx->BuildHash();
            auto txORHash = TransactionHelper::ExtractOpReturnHash(tx);
            if (ptxORHash == txORHash)
                return Success;

            if (CheckpointRepoInst.IsOpReturnCheckpoint(*ptx->GetHash(), ptxORHash))
                return Success;

            return {false, SocialConsensusResult_FailedOpReturn};
        }
    };


    class AccountUserConsensusFactory
    {
    private:
        const vector<ConsensusCheckpoint<AccountUserConsensus>> m_rules = {
            {       0,     -1, [](int height) { return make_shared<AccountUserConsensus>(height); }},
            { 1381841, 162000, [](int height) { return make_shared<AccountUserConsensus_checkpoint_chain_count>(height); }},
            { 1647000, 650000, [](int height) { return make_shared<AccountUserConsensus_checkpoint_login_limitation>(height); }},
        };
    public:
        shared_ptr<AccountUserConsensus> Instance(int height)
        {
            int m_height = (height > 0 ? height : 0);
            return (--upper_bound(m_rules.begin(), m_rules.end(), m_height,
                [&](int target, const ConsensusCheckpoint<AccountUserConsensus>& itm)
                {
                    return target < itm.Height(Params().NetworkIDString());
                }
            ))->m_func(m_height);
        }
    };

} // namespace PocketConsensus

#endif // POCKETCONSENSUS_USER_HPP
