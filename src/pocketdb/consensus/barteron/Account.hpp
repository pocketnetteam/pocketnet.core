// Copyright (c) 2023 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETCONSENSUS_BARTERON_ACCOUNT_HPP
#define POCKETCONSENSUS_BARTERON_ACCOUNT_HPP

#include "pocketdb/consensus/Reputation.h"
#include "pocketdb/consensus/Social.h"
#include "pocketdb/models/dto/barteron/Account.h"

namespace PocketConsensus
{
    using namespace std;
    typedef shared_ptr<BarteronAccount> BarteronAccountRef;

    /*******************************************************************************************************************
    *  BarteronAccount consensus base class
    *******************************************************************************************************************/
    class BarteronAccountConsensus : public SocialConsensus<BarteronAccount>
    {
    public:
        BarteronAccountConsensus(int height) : SocialConsensus<BarteronAccount>(height) {}
        ConsensusValidateResult Validate(const CTransactionRef& tx, const BarteronAccountRef& ptx, const PocketBlockRef& block) override
        {
            if (auto[ok, code] = SocialConsensus::Validate(tx, ptx, block); !ok)
                return {false, code};

            // Check payload size
            if (auto[ok, code] = ValidatePayloadSize(ptx); !ok)
                return {false, code};
            
            // TODO (barteron): implement

            return Success;
        }
        ConsensusValidateResult Check(const CTransactionRef& tx, const BarteronAccountRef& ptx) override
        {
            if (auto[ok, code] = SocialConsensus::Check(tx, ptx); !ok)
                return {false, code};

            // TODO (barteron): implement

            return Success;
        }

    protected:
    
        ConsensusValidateResult ValidateBlock(const BarteronAccountRef& ptx, const PocketBlockRef& block) override
        {
            // TODO (barteron): implement

            // Get count from chain
            // int count = GetChainCount(ptx);

            // // Get count from block
            // for (auto& blockTx : *block)
            // {
            //     if (!TransactionHelper::IsIn(*blockTx->GetType(), {BARTERON_ACCOUNT}))
            //         continue;

            //     auto blockPtx = static_pointer_cast<BarteronAccount>(blockTx);

            //     if (*ptx->GetAddress() != *blockPtx->GetAddress())
            //         continue;

            //     if (*blockPtx->GetHash() == *ptx->GetHash())
            //         continue;

            //     // if (blockPtx->IsEdit())
            //     //     continue;

            //     count += 1;
            // }

            // return ValidateLimit(ptx, count);

            return Success;
        }
        ConsensusValidateResult ValidateMempool(const BarteronAccountRef& ptx) override
        {
            // TODO (barteron): implement
            
            // Get count from chain
            // int count = GetChainCount(ptx);

            // // and from mempool
            // count += ConsensusRepoInst.CountMempoolBarteronAccount(*ptx->GetAddress());

            // return ValidateLimit(ptx, count);

            return Success;
        }
        
        virtual ConsensusValidateResult ValidatePayloadSize(const BarteronAccountRef& ptx)
        {
            // TODO (barteron): implement

            // size_t dataSize =
            //         (ptx->GetPayloadUrl() ? ptx->GetPayloadUrl()->size() : 0) +
            //         (ptx->GetPayloadCaption() ? ptx->GetPayloadCaption()->size() : 0) +
            //         (ptx->GetPayloadMessage() ? ptx->GetPayloadMessage()->size() : 0) +
            //         (ptx->GetRelayTxHash() ? ptx->GetRelayTxHash()->size() : 0) +
            //         (ptx->GetPayloadSettings() ? ptx->GetPayloadSettings()->size() : 0) +
            //         (ptx->GetPayloadLang() ? ptx->GetPayloadLang()->size() : 0);

            // if (ptx->GetRootTxHash() && *ptx->GetRootTxHash() != *ptx->GetHash())
            //     dataSize += ptx->GetRootTxHash()->size();

            // if (!IsEmpty(ptx->GetPayloadTags()))
            // {
            //     UniValue tags(UniValue::VARR);
            //     tags.read(*ptx->GetPayloadTags());
            //     for (size_t i = 0; i < tags.size(); ++i)
            //         dataSize += tags[i].get_str().size();
            // }

            // if (!IsEmpty(ptx->GetPayloadImages()))
            // {
            //     UniValue images(UniValue::VARR);
            //     images.read(*ptx->GetPayloadImages());
            //     for (size_t i = 0; i < images.size(); ++i)
            //         dataSize += images[i].get_str().size();
            // }

            // if (dataSize > (size_t)GetConsensusLimit(ConsensusLimit_max_barteron_request_size))
            //     return {false, SocialConsensusResult_ContentSizeLimit};

            return Success;
        }
    };

    /*******************************************************************************************************************
    *  Factory for select actual rules version
    *******************************************************************************************************************/
    class BarteronAccountConsensusFactory
    {
    private:
        const vector<ConsensusCheckpoint<BarteronAccountConsensus>> m_rules = {
                { 99999999, 99999999, 0, [](int height) { return make_shared<BarteronAccountConsensus>(height); }},
        };
    public:
        shared_ptr<BarteronAccountConsensus> Instance(int height)
        {
            int m_height = (height > 0 ? height : 0);
            return (--upper_bound(m_rules.begin(), m_rules.end(), m_height,
                                  [&](int target, const ConsensusCheckpoint<BarteronAccountConsensus>& itm)
                                  {
                                      return target < itm.Height(Params().NetworkID());
                                  }
            ))->m_func(m_height);
        }
    };
}

#endif // POCKETCONSENSUS_BARTERON_ACCOUNT_HPP
