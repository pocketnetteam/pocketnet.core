// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETCONSENSUS_USER_HPP
#define POCKETCONSENSUS_USER_HPP

#include <boost/algorithm/string.hpp>
#include "pocketdb/consensus/social/account/Account.hpp"
#include "pocketdb/models/dto/account/User.h"

namespace PocketConsensus
{
    using namespace std;
    using namespace PocketDb;
    typedef shared_ptr<User> UserRef;

    class AccountUserConsensus : public AccountConsensus<User>
    {
    private:
        using Base = AccountConsensus<User>;

    public:
        AccountUserConsensus() : AccountConsensus<User>()
        {
            Limits.Set("edit_account_daily_count", 10, 10, 10);
            Limits.Set("edit_account_depth", 86400, 1440, 1440);
            Limits.Set("payload_size", 2000, 2000, 2000);
        }
        
        ConsensusValidateResult Validate(const CTransactionRef& tx, const UserRef& ptx, const PocketBlockRef& block) override
        {
            // Duplicate name
            if (ConsensusRepoInst.ExistsAnotherByName(*ptx->GetAddress(), *ptx->GetPayloadName()))
            {
                if (!CheckpointRepoInst.IsSocialCheckpoint(*ptx->GetHash(), *ptx->GetType(), ConsensusResult_NicknameDouble))
                    return {false, ConsensusResult_NicknameDouble};
            }

            if (!CheckDeleted(ptx))
                return {false, ConsensusResult_AccountDeleted};

            return SocialConsensus::Validate(tx, ptx, block);
        }

        ConsensusValidateResult Check(const CTransactionRef& tx, const UserRef& ptx) override
        {
            if (auto[baseCheck, baseCheckCode] = SocialConsensus::Check(tx, ptx); !baseCheck)
                return {false, baseCheckCode};

            // Check required fields
            if (IsEmpty(ptx->GetAddress())) return {false, ConsensusResult_Failed};

            // Check payload
            if (!ptx->GetPayload()) return {false, ConsensusResult_Failed};

            // Self referring
            if (!IsEmpty(ptx->GetReferrerAddress()) && *ptx->GetAddress() == *ptx->GetReferrerAddress())
                return make_tuple(false, ConsensusResult_ReferrerSelf);

            // Name check
            if (auto[ok, result] = CheckLogin(ptx); !ok)
                return {false, result};

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
                    if (!CheckpointRepoInst.IsSocialCheckpoint(*ptx->GetHash(), *ptx->GetType(), ConsensusResult_ChangeInfoDoubleInBlock))
                        return {false, ConsensusResult_ChangeInfoDoubleInBlock};
                }

                if (TransactionHelper::IsIn(*blockTx->GetType(), { ACCOUNT_USER }))
                {
                    auto blockPtx = static_pointer_cast<User>(blockTx);
                    if (auto[ok, result] = ValidateBlockDuplicateName(ptx, blockPtx); !ok)
                        return {false, result};
                }
            }

            if (GetChainCount(ptx) > Limits.Get("edit_account_daily_count"))
                return {false, ConsensusResult_ChangeInfoLimit};

            return Success;
        }

        ConsensusValidateResult ValidateMempool(const UserRef& ptx) override
        {
            if (ConsensusRepoInst.Exists_MS1T(*ptx->GetAddress(), { ACCOUNT_USER, ACCOUNT_DELETE }))
                return {false, ConsensusResult_ChangeInfoDoubleInMempool};

            if (GetChainCount(ptx) > Limits.Get("edit_account_daily_count"))
                return {false, ConsensusResult_ChangeInfoLimit};

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
    
        virtual ConsensusValidateResult CheckLogin(const UserRef& ptx)
        {
            auto name = *ptx->GetPayloadName();

            // Trim spaces
            if (boost::algorithm::ends_with(name, "%20") || boost::algorithm::starts_with(name, "%20"))
            {
                if (!CheckpointRepoInst.IsSocialCheckpoint(*ptx->GetHash(), *ptx->GetType(),
                    ConsensusResult_Failed))
                    return {false, ConsensusResult_Failed};
            }

            return Success;
        }
    
        virtual ConsensusValidateResult ValidateBlockDuplicateName(const UserRef& ptx, const UserRef& blockPtx)
        {
            return Success;
        }

        virtual bool CheckDeleted(const UserRef& ptx)
        {
            // The deleted account cannot be restored
            if (auto[ok, type] = ConsensusRepoInst.GetLastAccountType(*ptx->GetAddress()); ok)
                if (type == TxType::ACCOUNT_DELETE)
                    return false;
                    
            return true;
        }
    };

    class AccountUserConsensus_checkpoint_depth : public AccountUserConsensus
    {
    public:
        AccountUserConsensus_checkpoint_depth() : AccountUserConsensus()
        {
            Limits.Set("edit_account_depth", 1440, 1440, 1440);
        }
    };

    class AccountUserConsensus_checkpoint_chain_count : public AccountUserConsensus_checkpoint_depth
    {
    public:
        AccountUserConsensus_checkpoint_chain_count() : AccountUserConsensus_checkpoint_depth() {}

    protected:
        int GetChainCount(const UserRef& ptx) override
        {
            return ConsensusRepoInst.CountChainAccount(
                *ptx->GetType(),
                *ptx->GetAddress(),
                Height - (int)Limits.Get("edit_account_depth")
            );
        }
    };

    class AccountUserConsensus_checkpoint_login_limitation : public AccountUserConsensus_checkpoint_chain_count
    {
    public:
        AccountUserConsensus_checkpoint_login_limitation() : AccountUserConsensus_checkpoint_chain_count() {}

    protected:
        ConsensusValidateResult CheckLogin(const UserRef& ptx) override
        {
            if (IsEmpty(ptx->GetPayloadName()))
                return {false, ConsensusResult_Failed};

            auto name = *ptx->GetPayloadName();
            boost::algorithm::to_lower(name);

            if (name.size() > 20)
                return {false, ConsensusResult_NicknameLong};
            
            if (!all_of(name.begin(), name.end(), [](unsigned char ch) { return ::isalnum(ch) || ch == '_'; }))
                return {false, ConsensusResult_Failed};

            return Success;
        }
        
        ConsensusValidateResult ValidateBlockDuplicateName(const UserRef& ptx, const UserRef& blockPtx) override
        {
            auto ptxName = ptx->GetPayloadName() ? *ptx->GetPayloadName() : "";
            boost::algorithm::to_lower(ptxName);

            auto blockPtxName = blockPtx->GetPayloadName() ? *blockPtx->GetPayloadName() : "";
            boost::algorithm::to_lower(blockPtxName);

            if (ptxName == blockPtxName)
                if (!CheckpointRepoInst.IsSocialCheckpoint(*ptx->GetHash(), *ptx->GetType(), ConsensusResult_NicknameDouble))
                    return {false, ConsensusResult_NicknameDouble};

            return Success;
        }
    };

    class AccountUserConsensus_checkpoint_pip_106 : public AccountUserConsensus_checkpoint_login_limitation
    {
    public:
        AccountUserConsensus_checkpoint_pip_106() : AccountUserConsensus_checkpoint_login_limitation() {}
    protected:
        bool CheckDeleted(const UserRef& ptx) override
        {
            // The deleted account can be restored
            return true;
        }    
    };

    class AccountUserConsensusFactory : public BaseConsensusFactory<AccountUserConsensus>
    {
    public:
        AccountUserConsensusFactory()
        {
            Checkpoint({       0,      -1, -1, make_shared<AccountUserConsensus>() });
            Checkpoint({ 1180000,       0, -1, make_shared<AccountUserConsensus_checkpoint_depth>() });
            Checkpoint({ 1381841,  162000, -1, make_shared<AccountUserConsensus_checkpoint_chain_count>() });
            Checkpoint({ 1647000,  650000,  0, make_shared<AccountUserConsensus_checkpoint_login_limitation>() });
            Checkpoint({ 2870000, 2850000,  0, make_shared<AccountUserConsensus_checkpoint_pip_106>() });
        }
    };

    static AccountUserConsensusFactory ConsensusFactoryInst_AccountUser;
}

#endif // POCKETCONSENSUS_USER_HPP
