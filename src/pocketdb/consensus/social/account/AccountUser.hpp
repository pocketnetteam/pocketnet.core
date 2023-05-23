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

    class AccountUserConsensusOld : public AccountConsensus<User>
    {
    private:
        using Base = AccountConsensus<User>;

    public:
        AccountUserConsensusOld() : AccountConsensus<User>()
        {
            // TODO (limits): set current limits
        }
        
        ConsensusValidateResult Validate(const CTransactionRef& tx, const UserRef& ptx, const PocketBlockRef& block) override
        {
            if (auto[ok, code] = Base::Validate(tx, ptx, block); !ok)
                return {false, code};

            // Get all the necessary data for transaction validation
            consensusData = ConsensusRepoInst.AccountUser(
                *ptx->GetAddress(),
                Height - (int)GetConsensusLimit(ConsensusLimit_depth),
                *ptx->GetPayloadName()
            );

            // Duplicate name
            if (consensusData.DuplicatesChainCount > 0)
                if (!CheckpointRepoInst.IsSocialCheckpoint(*ptx->GetHash(), *ptx->GetType(), ConsensusResult_NicknameDouble))
                    return {false, ConsensusResult_NicknameDouble};

            // The deleted account cannot be restored
            if ((TxType)consensusData.LastTxType == TxType::ACCOUNT_DELETE)
                return {false, ConsensusResult_AccountDeleted};

            // Daily change limit
            // if (consensusData.EditsCount > GetConsensusLimit(edit_account_daily_count))
            //     if (!CheckpointRepoInst.IsSocialCheckpoint(*ptx->GetHash(), *ptx->GetType(), ConsensusResult_ChangeInfoLimit))
            //         return {false, ConsensusResult_ChangeInfoLimit};

            return Success;
        }

        ConsensusValidateResult Check(const CTransactionRef& tx, const UserRef& ptx) override
        {
            if (auto[ok, code] = Base::Check(tx, ptx); !ok)
                return {false, code};
             
            // Check payload
            if (!ptx->GetPayload()) return {false, ConsensusResult_Failed};

            // Self referring
            if (!IsEmpty(ptx->GetReferrerAddress()) && *ptx->GetAddress() == *ptx->GetReferrerAddress())
                return {false, ConsensusResult_ReferrerSelf};

            // Name check
            if (auto[ok, result] = CheckLogin(ptx); !ok)
                return {false, result};

            return Success;
        }

    protected:
        ConsensusData_AccountUser consensusData;

        ConsensusValidateResult ValidateBlock(const UserRef& ptx, const PocketBlockRef& block) override
        {
            // Only one transaction allowed in block
            for (auto& blockTx: *block)
            {
                if (!TransactionHelper::IsIn(*blockTx->GetType(), { ACCOUNT_USER, ACCOUNT_DELETE }) || *blockTx->GetHash() == *ptx->GetHash())
                    continue;

                auto blockPtx = static_pointer_cast<SocialTransaction>(blockTx);

                // In the early stages of the network, double transactions were allowed in blocks
                if (*ptx->GetAddress() == *blockPtx->GetAddress())
                    if (!CheckpointRepoInst.IsSocialCheckpoint(*ptx->GetHash(), *ptx->GetType(), ConsensusResult_ChangeInfoDoubleInBlock))
                        return {false, ConsensusResult_ChangeInfoDoubleInBlock};

                // We can allow "capture" another username in one block
                if (TransactionHelper::IsIn(*blockPtx->GetType(), { ACCOUNT_USER }))
                {
                    auto blockUserPtx = static_pointer_cast<User>(blockTx);
                    if (auto[ok, result] = ValidateBlockDuplicateName(ptx, blockUserPtx); !ok)
                        return {false, result};
                }
            }

            return Success;
        }

        ConsensusValidateResult ValidateMempool(const UserRef& ptx) override
        {
            if (consensusData.MempoolCount > 0)
                return {false, ConsensusResult_ChangeInfoDoubleInMempool};

            if (consensusData.DuplicatesMempoolCount > 0)
                return {false, ConsensusResult_NicknameDouble};

            return Success;
        }
        
        ConsensusValidateResult ValidatePayloadSize(const UserRef& ptx)
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
                return {false, ConsensusResult_ContentSizeLimit};

            return Success;
        }
    
        virtual ConsensusValidateResult CheckLogin(const UserRef& ptx)
        {
            if (IsEmpty(ptx->GetPayloadName()))
                if (!CheckpointRepoInst.IsSocialCheckpoint(*ptx->GetHash(), *ptx->GetType(), ConsensusResult_Failed))
                    return {false, ConsensusResult_Failed};

            auto name = *ptx->GetPayloadName();
            boost::algorithm::to_lower(name);

            if (name.size() > 20)
                if (!CheckpointRepoInst.IsSocialCheckpoint(*ptx->GetHash(), *ptx->GetType(), ConsensusResult_NicknameLong))
                    return {false, ConsensusResult_NicknameLong};
            
            if (!all_of(name.begin(), name.end(), [](unsigned char ch) { return ::isalnum(ch) || ch == '_'; }))
                if (!CheckpointRepoInst.IsSocialCheckpoint(*ptx->GetHash(), *ptx->GetType(), ConsensusResult_Failed))
                    return {false, ConsensusResult_Failed};

            return Success;
        }
    
        virtual ConsensusValidateResult ValidateBlockDuplicateName(const UserRef& ptx, const UserRef& blockPtx)
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

            // The deleted account cannot be restored
            if (auto[ok, type] = ConsensusRepoInst.GetLastAccountType(*ptx->GetAddress()); ok)
                if (type == TxType::ACCOUNT_DELETE)
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
            // TODO (aok) : use here ExtractBlockPtxs

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

            // TODO (aok): DEBUG!
            if (ptxName == blockPtxName)
                //if (!CheckpointRepoInst.IsSocialCheckpoint(*ptx->GetHash(), *ptx->GetType(), ConsensusResult_NicknameDouble))
                    return {false, ConsensusResult_NicknameDouble};

            return Success;
        }
    };

    class AccountUserConsensusFactory : public BaseConsensusFactory<AccountUserConsensus>
    {
    public:
        AccountUserConsensusFactory()
        {
            Checkpoint({       0,     -1, -1, make_shared<AccountUserConsensus>() });
            Checkpoint({ 1180000,      0, -1, make_shared<AccountUserConsensus_checkpoint_depth>() });
            Checkpoint({ 1381841, 162000, -1, make_shared<AccountUserConsensus_checkpoint_chain_count>() });
            Checkpoint({ 1647000, 650000,  0, make_shared<AccountUserConsensus_checkpoint_login_limitation>() });
        }
    };

    static AccountUserConsensusFactory ConsensusFactoryInst_AccountUser;
}

#endif // POCKETCONSENSUS_USER_HPP
