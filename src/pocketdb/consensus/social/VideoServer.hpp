// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETCONSENSUS_VIDEO_SERVER_HPP
#define POCKETCONSENSUS_VIDEO_SERVER_HPP

#include "pocketdb/consensus/Social.h"
#include "pocketdb/models/base/PocketTypes.h"
#include "pocketdb/models/dto/VideoServer.h"

namespace PocketConsensus
{
    using namespace std;
    typedef shared_ptr<VideoServer> VideoServerRef;

    /*******************************************************************************************************************
    *  User consensus base class
    *******************************************************************************************************************/
    class VideoServerConsensus : public SocialConsensus<VideoServer>
    {
    public:
        VideoServerConsensus(int height) : SocialConsensus<VideoServer>(height) {}
        ConsensusValidateResult Validate(const CTransactionRef& tx, const VideoServerRef& ptx, const PocketBlockRef& block) override
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
        ConsensusValidateResult Check(const CTransactionRef& tx, const VideoServerRef& ptx) override
        {
            if (auto[baseCheck, baseCheckCode] = SocialConsensus::Check(tx, ptx); !baseCheck)
                return {false, baseCheckCode};

            // Check required fields
            if (IsEmpty(ptx->GetAddress())) return {false, SocialConsensusResult_Failed};

            // Check payload
            if (!ptx->GetPayload()) return {false, SocialConsensusResult_Failed};

            // TODO (brangr): enable with fork height
            //if (IsEmpty(ptx->GetPayloadName())) return {false, SocialConsensusResult_Failed};

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

        ConsensusValidateResult CheckOpReturnHash(const CTransactionRef& tx, const VideoServerRef& ptx) override
        {
            auto ptxORHash = ptx->BuildHash();
            auto txORHash = TransactionHelper::ExtractOpReturnHash(tx);

            if (ptxORHash != txORHash)
            {
                auto ptxORHashRef = ptx->BuildHash();
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
        ConsensusValidateResult ValidateBlock(const VideoServerRef& ptx, const PocketBlockRef& block) override
        {
            // Only one transaction allowed in block
            for (auto& blockTx: *block)
            {
                if (!TransactionHelper::IsIn(*blockTx->GetType(), {ACCOUNT_USER}))
                    continue;

                if (*blockTx->GetHash() == *ptx->GetHash())
                    continue;

                auto blockPtx = static_pointer_cast<User>(blockTx);
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
        ConsensusValidateResult ValidateMempool(const VideoServerRef& ptx) override
        {
            if (ConsensusRepoInst.CountMempoolUser(*ptx->GetAddress()) > 0)
                return {false, SocialConsensusResult_ChangeInfoDoubleInMempool};

            if (GetChainCount(ptx) > GetConsensusLimit(ConsensusLimit_edit_user_daily_count))
                return {false, SocialConsensusResult_ChangeInfoLimit};

            return Success;
        }
        vector<string> GetAddressesForCheckRegistration(const VideoServerRef& ptx) override
        {
            return {};
        }

        virtual ConsensusValidateResult ValidateEdit(const VideoServerRef& ptx)
        {
            // First user account transaction allowed without next checks
            if (auto[ok, prevTxHeight] = ConsensusRepoInst.GetLastAccountHeight(*ptx->GetAddress()); !ok)
                return Success;

            // Check editing limits
            if (auto[ok, code] = ValidateEditLimit(ptx); !ok)
                return {false, code};

            return Success;
        }

        virtual ConsensusValidateResult ValidateEditLimit(const VideoServerRef& ptx)
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

        virtual int GetChainCount(const VideoServerRef& ptx)
        {
            return 0;
        }

        virtual ConsensusValidateResult ValidatePayloadSize(const VideoServerRef& ptx)
        {
            size_t dataSize =
                (ptx->GetPayloadName() ? ptx->GetPayloadName()->size() : 0) +
                (ptx->GetPayloadIP() ? ptx->GetPayloadIP()->size() : 0) +
                (ptx->GetPayloadDomain() ? ptx->GetPayloadDomain()->size() : 0) +
                (ptx->GetPayloadAbout() ? ptx->GetPayloadAbout()->size() : 0);

            if (dataSize > (size_t) GetConsensusLimit(ConsensusLimit_max_user_size))
                return {false, SocialConsensusResult_ContentSizeLimit};

            return Success;
        }
    };

    /*******************************************************************************************************************
    *  Factory for select actual rules version
    *******************************************************************************************************************/
    class UserConsensusFactory
    {
    private:
        const vector<ConsensusCheckpoint < VideoServerConsensus>> m_rules = {
            { 0, -1, [](int height) { return make_shared<VideoServerConsensus>(height); }},
        };
    public:
        shared_ptr<VideoServerConsensus> Instance(int height)
        {
            int m_height = (height > 0 ? height : 0);
            return (--upper_bound(m_rules.begin(), m_rules.end(), m_height,
                [&](int target, const ConsensusCheckpoint<VideoServerConsensus>& itm)
                {
                    return target < itm.Height(Params().NetworkIDString());
                }
            ))->m_func(height);
        }
    };

} // namespace PocketConsensus

#endif // POCKETCONSENSUS_VIDEO_SERVER_HPP
