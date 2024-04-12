// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETCONSENSUS_BOOSTCONTENT_HPP
#define POCKETCONSENSUS_BOOSTCONTENT_HPP

#include "pocketdb/consensus/Social.h"
#include "pocketdb/models/dto/action/BoostContent.h"

namespace PocketConsensus
{
    typedef shared_ptr<BoostContent> BoostContentRef;

    /*******************************************************************************************************************
    *  BoostContent consensus base class
    *******************************************************************************************************************/
    class BoostContentConsensus : public SocialConsensus<BoostContent>
    {
    public:
        BoostContentConsensus() : SocialConsensus<BoostContent>()
        {
            // TODO (limits): set limits
        }

        ConsensusValidateResult Validate(const CTransactionRef& tx, const BoostContentRef& ptx, const PocketBlockRef& block) override
        {
            // Check exists content transaction
            auto[contentOk, contentTx] = PocketDb::ConsensusRepoInst.GetLastContent(*ptx->GetContentTxHash(), { CONTENT_POST, CONTENT_VIDEO, CONTENT_ARTICLE, CONTENT_STREAM, CONTENT_AUDIO, CONTENT_DELETE });
            if (!contentOk)
                return {false, ConsensusResult_NotFound};

            if (*contentTx->GetType() == CONTENT_DELETE)
                return {false, ConsensusResult_CommentDeletedContent};

            // Check Blocking
            if (ValidateBlocking(*contentTx->GetString1(), *ptx->GetAddress()))
                return {false, ConsensusResult_Blocking};

            return SocialConsensus::Validate(tx, ptx, block);
        }

        ConsensusValidateResult Check(const CTransactionRef& tx, const BoostContentRef& ptx) override
        {
            if (auto[baseCheck, baseCheckCode] = SocialConsensus::Check(tx, ptx); !baseCheck)
                return {false, baseCheckCode};

            // Check required fields
            if (IsEmpty(ptx->GetAddress())) return {false, ConsensusResult_Failed};
            if (IsEmpty(ptx->GetContentTxHash())) return {false, ConsensusResult_Failed};

            return Success;
        }

    protected:
        ConsensusValidateResult ValidateBlock(const BoostContentRef& ptx, const PocketBlockRef& block) override
        {
            return Success;
        }

        ConsensusValidateResult ValidateMempool(const BoostContentRef& ptx) override
        {
            return Success;
        }

        vector<string> GetAddressesForCheckRegistration(const BoostContentRef& ptx) override
        {
            return { *ptx->GetAddress() };
        }

        virtual bool ValidateBlocking(const string& address1, const string& address2)
        {
            return false;
        }
    };

    // Disable scores for blocked accounts
    class BoostContentConsensus_checkpoint_disable_for_blocked : public BoostContentConsensus
    {
    public:
        BoostContentConsensus_checkpoint_disable_for_blocked() : BoostContentConsensus() {}
    protected:
        bool ValidateBlocking(const string& address1, const string& address2) override
        {
            auto[existsBlocking, blockingType] = PocketDb::ConsensusRepoInst.GetLastBlockingType(address1, address2);
            return existsBlocking && blockingType == ACTION_BLOCKING;
        }
    };

    // ----------------------------------------------------------------------------------------------
    class BoostContentConsensus_checkpoint_pip_105 : public BoostContentConsensus_checkpoint_disable_for_blocked
    {
    public:
        BoostContentConsensus_checkpoint_pip_105() : BoostContentConsensus_checkpoint_disable_for_blocked() {}
    protected:
        bool ValidateBlocking(const string& address1, const string& address2) override
        {
            return SocialConsensus::CheckBlocking(address1, address2);
        }
    };


    // ----------------------------------------------------------------------------------------------
    // Factory for select actual rules version
    class BoostContentConsensusFactory : public BaseConsensusFactory<BoostContentConsensus>
    {
    public:
        BoostContentConsensusFactory()
        {
            Checkpoint({       0,       0, -1, make_shared<BoostContentConsensus>() });
            Checkpoint({ 1757000,  953000, -1, make_shared<BoostContentConsensus_checkpoint_disable_for_blocked>() });
            Checkpoint({ 2794500, 2574300,  0, make_shared<BoostContentConsensus_checkpoint_pip_105>() });
        }
    };

    static BoostContentConsensusFactory ConsensusFactoryInst_BoostContent;
}

#endif //POCKETCONSENSUS_BOOSTCONTENT_HPP
