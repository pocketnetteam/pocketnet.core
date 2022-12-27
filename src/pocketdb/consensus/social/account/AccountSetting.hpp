// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETCONSENSUS_ACCOUNT_SETTING_HPP
#define POCKETCONSENSUS_ACCOUNT_SETTING_HPP

#include "pocketdb/consensus/social/account/Account.hpp"
#include "pocketdb/models/dto/account/Setting.h"

namespace PocketConsensus
{
    using namespace std;
    
    typedef shared_ptr<AccountSetting> AccountSettingRef;

    /*******************************************************************************************************************
    *  AccountSetting consensus base class
    *******************************************************************************************************************/
    class AccountSettingConsensus : public AccountConsensus<AccountSetting>
    {
    private:
        using Base = AccountConsensus<AccountSetting>;

    public:
        AccountSettingConsensus(int height) : AccountConsensus<AccountSetting>(height) {}
        ConsensusValidateResult Validate(const CTransactionRef& tx, const AccountSettingRef& ptx, const PocketBlockRef& block) override
        {
            if (auto[ok, code] = Base::Validate(tx, ptx, block); !ok)
                return {false, code};

            int count = GetChainCount(ptx);
            return ValidateLimit(account_settings_daily_count, count);
        }
        ConsensusValidateResult Check(const CTransactionRef& tx, const AccountSettingRef& ptx) override
        {
            if (auto[ok, code] = Base::Check(tx, ptx); !ok)
                return {false, code};

            if (IsEmpty(ptx->GetPayloadData()))
                return {false, SocialConsensusResult_Failed};

            return Success;
        }

    protected:
        ConsensusValidateResult ValidateBlock(const AccountSettingRef& ptx, const PocketBlockRef& block) override
        {
            // Only one transaction allowed in block
            for (auto& blockTx : *block)
            {
                if (!TransactionHelper::IsIn(*blockTx->GetType(), {ACCOUNT_SETTING}))
                    continue;

                auto blockPtx = static_pointer_cast<AccountSetting>(blockTx);

                if (*blockPtx->GetHash() == *ptx->GetHash())
                    continue;

                if (*ptx->GetAddress() == *blockPtx->GetAddress())
                    return {false, SocialConsensusResult_AccountSettingsDouble};
            }

            return Success;
        }

        ConsensusValidateResult ValidateMempool(const AccountSettingRef& ptx) override
        {
            if (ConsensusRepoInst.CountMempoolAccountSetting(*ptx->GetAddress()) > 0)
                return {false, SocialConsensusResult_AccountSettingsDouble};

            return Success;
        }
        
        ConsensusValidateResult ValidatePayloadSize(const AccountSettingRef& ptx) override
        {
            if ((int64_t)ptx->GetPayloadData()->size() > GetConsensusLimit(ConsensusLimit_max_account_setting_size))
                return {false, SocialConsensusResult_ContentSizeLimit};

            return Success;
        }
        
        virtual int GetChainCount(const AccountSettingRef& ptx)
        {
            return ConsensusRepoInst.CountChainAccountSetting(
                *ptx->GetAddress(),
                Height - (int)GetConsensusLimit(ConsensusLimit_depth)
            );
        }
    };

    /*******************************************************************************************************************
    *  Factory for select actual rules version
    *******************************************************************************************************************/
    class AccountSettingConsensusFactory
    {
    private:
        const vector<ConsensusCheckpoint<AccountSettingConsensus>> m_rules = {
            { 0, 0, [](int height) { return make_shared<AccountSettingConsensus>(height); }},
        };
    public:
        shared_ptr<AccountSettingConsensus> Instance(int height)
        {
            int m_height = (height > 0 ? height : 0);
            return (--upper_bound(m_rules.begin(), m_rules.end(), m_height,
                [&](int target, const ConsensusCheckpoint<AccountSettingConsensus>& itm)
                {
                    return target < itm.Height(Params().NetworkIDString());
                }
            ))->m_func(m_height);
        }
    };

} // namespace PocketConsensus

#endif // POCKETCONSENSUS_ACCOUNT_SETTING_HPP
