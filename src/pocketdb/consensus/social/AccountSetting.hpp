// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETCONSENSUS_ACCOUNT_SETTING_HPP
#define POCKETCONSENSUS_ACCOUNT_SETTING_HPP

#include "pocketdb/consensus/Social.h"
#include "pocketdb/models/dto/account/Setting.h"

namespace PocketConsensus
{
    using namespace std;
    typedef shared_ptr<AccountSetting> AccountSettingRef;

    /*******************************************************************************************************************
    *  AccountSetting consensus base class
    *******************************************************************************************************************/
    class AccountSettingConsensus : public SocialConsensus<AccountSetting>
    {
    public:
        AccountSettingConsensus(int height) : SocialConsensus<AccountSetting>(height) {}
        ConsensusValidateResult Validate(const CTransactionRef& tx, const AccountSettingRef& ptx, const PocketBlockRef& block) override
        {
            // Check payload size
            if (auto[ok, code] = ValidatePayloadSize(ptx); !ok)
                return {false, code};

            return SocialConsensus::Validate(tx, ptx, block);
        }
        ConsensusValidateResult Check(const CTransactionRef& tx, const AccountSettingRef& ptx) override
        {
            if (auto[baseCheck, baseCheckCode] = SocialConsensus::Check(tx, ptx); !baseCheck)
                return {false, baseCheckCode};

            // Check required fields
            if (IsEmpty(ptx->GetAddress())) return {false, SocialConsensusResult_Failed};

            // Check payload
            if (!ptx->GetPayload()) return {false, SocialConsensusResult_Failed};
            if (IsEmpty(ptx->GetPayloadData())) return {false, SocialConsensusResult_Failed};

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

            int count = GetChainCount(ptx);
            return ValidateLimit(ptx, count);
        }
        ConsensusValidateResult ValidateMempool(const AccountSettingRef& ptx) override
        {
            if (ConsensusRepoInst.CountMempoolAccountSetting(*ptx->GetAddress()) > 0)
                return {false, SocialConsensusResult_AccountSettingsDouble};

            int count = GetChainCount(ptx);
            return ValidateLimit(ptx, count);
        }
        vector<string> GetAddressesForCheckRegistration(const AccountSettingRef& ptx) override
        {
            return {*ptx->GetAddress()};
        }
    
    
        virtual ConsensusValidateResult ValidateLimit(const AccountSettingRef& ptx, int count)
        {
            if (count >= GetConsensusLimit(ConsensusLimit_account_settings_daily_count))
                return {false, SocialConsensusResult_AccountSettingsLimit};

            return Success;
        }
        virtual int GetChainCount(const AccountSettingRef& ptx)
        {
            return ConsensusRepoInst.CountChainAccountSetting(
                *ptx->GetAddress(),
                Height - (int)GetConsensusLimit(ConsensusLimit_depth)
            );
        }
        virtual ConsensusValidateResult ValidatePayloadSize(const AccountSettingRef& ptx)
        {
            if ((int64_t)ptx->GetPayloadData()->size() > GetConsensusLimit(ConsensusLimit_max_account_setting_size))
                return {false, SocialConsensusResult_ContentSizeLimit};

            return Success;
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
