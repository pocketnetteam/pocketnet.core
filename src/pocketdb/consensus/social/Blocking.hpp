// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETCONSENSUS_BLOCKING_HPP
#define POCKETCONSENSUS_BLOCKING_HPP

#include "pocketdb/consensus/Social.h"
#include "pocketdb/models/dto/action/Blocking.h"

namespace PocketConsensus
{
    typedef shared_ptr<Blocking> BlockingRef;

    /*******************************************************************************************************************
    *  Blocking consensus base class
    *******************************************************************************************************************/
    class BlockingConsensus : public SocialConsensus<Blocking>
    {
    public:
        BlockingConsensus() : SocialConsensus<Blocking>()
        {
            // TODO (limits): set limits
        }

        ConsensusValidateResult Validate(const CTransactionRef& tx, const BlockingRef& ptx, const PocketBlockRef& block) override
        {
            // Double blocking in chain
            if (auto[existsBlocking, blockingType] = PocketDb::ConsensusRepoInst.GetLastBlockingType(
                    *ptx->GetAddress(),
                    *ptx->GetAddressTo()
                ); existsBlocking && blockingType == ACTION_BLOCKING)
            {
                if (!CheckpointRepoInst.IsSocialCheckpoint(*ptx->GetHash(), *ptx->GetType(), ConsensusResult_DoubleBlocking))
                    return {false, ConsensusResult_DoubleBlocking};
            }

            return SocialConsensus::Validate(tx, ptx, block);
        }
        ConsensusValidateResult Check(const CTransactionRef& tx, const BlockingRef& ptx) override
        {
            if (auto[baseCheck, baseCheckCode] = SocialConsensus::Check(tx, ptx); !baseCheck)
                return {false, baseCheckCode};

            // Check required fields
            if (IsEmpty(ptx->GetAddress())) return {false, ConsensusResult_Failed};
            if (IsEmpty(ptx->GetAddressTo())) return {false, ConsensusResult_Failed};

            // Blocking self
            if (*ptx->GetAddress() == *ptx->GetAddressTo())
                return {false, ConsensusResult_SelfBlocking};

            return Success;
        }

    protected:
        ConsensusValidateResult ValidateBlock(const BlockingRef& ptx, const PocketBlockRef& block) override
        {
            for (auto& blockTx : *block)
            {
                if (!TransactionHelper::IsIn(*blockTx->GetType(), {ACTION_BLOCKING, ACTION_BLOCKING_CANCEL}))
                    continue;

                auto blockPtx = static_pointer_cast<Blocking>(blockTx);

                if (*blockPtx->GetHash() == *ptx->GetHash())
                    continue;

                if (*ptx->GetAddress() == *blockPtx->GetAddress() && *ptx->GetAddressTo() == *blockPtx->GetAddressTo())
                    return {false, ConsensusResult_ManyTransactions};
            }

            return Success;
        }
        ConsensusValidateResult ValidateMempool(const BlockingRef& ptx) override
        {
            if (ConsensusRepoInst.CountMempoolBlocking(*ptx->GetAddress(), *ptx->GetAddressTo()) > 0)
                return {false, ConsensusResult_ManyTransactions};

            return Success;
        }
        vector<string> GetAddressesForCheckRegistration(const BlockingRef& ptx) override
        {
            return {*ptx->GetAddress(), *ptx->GetAddressTo()};
        }
    };

    class BlockingConsensus_checkpoint_multiple_blocking : public BlockingConsensus
    {
    public:
        BlockingConsensus_checkpoint_multiple_blocking() : BlockingConsensus() {}

        ConsensusValidateResult Validate(const CTransactionRef& tx, const BlockingRef& ptx, const PocketBlockRef& block) override
        {
            // Base validation with calling block or mempool check
            if (auto[baseValidate, baseValidateCode] = SocialConsensus::Validate(tx, ptx, block); !baseValidate)
                return {false, baseValidateCode};

            // Double blocking in chain
            if (PocketDb::ConsensusRepoInst.ExistBlocking(
                    *ptx->GetAddress(),
                    IsEmpty(ptx->GetAddressTo()) ? "" : *ptx->GetAddressTo(),
                    IsEmpty(ptx->GetAddressesTo()) ? "[]" : *ptx->GetAddressesTo()
                ))
            {
                if (!CheckpointRepoInst.IsSocialCheckpoint(*ptx->GetHash(), *ptx->GetType(), ConsensusResult_DoubleBlocking))
                    return {false, ConsensusResult_DoubleBlocking};
            }

            return Success;
        }
        ConsensusValidateResult Check(const CTransactionRef& tx, const BlockingRef& ptx) override
        {
            if (auto[baseCheck, baseCheckCode] = SocialConsensus::Check(tx, ptx); !baseCheck)
                return {false, baseCheckCode};

            // Check required fields
            if (IsEmpty(ptx->GetAddress())) return {false, ConsensusResult_Failed};
            if (IsEmpty(ptx->GetAddressTo()) && IsEmpty(ptx->GetAddressesTo())) return {false, ConsensusResult_Failed};
            if (!IsEmpty(ptx->GetAddressTo()) && !IsEmpty(ptx->GetAddressesTo())) return {false, ConsensusResult_Failed};
            if (!IsEmpty(ptx->GetAddressesTo()))
            {
                UniValue addrs(UniValue::VARR);
                if (!addrs.read(*ptx->GetAddressesTo()))
                    return {false, ConsensusResult_Failed};
                if (addrs.size() > (size_t)GetConsensusLimit(ConsensusLimit_multiple_lock_addresses_count))
                    return {false, ConsensusResult_Failed};
            }

            // Blocking self
            auto blockingaddresses = IsEmpty(ptx->GetAddressTo()) ? "" : *ptx->GetAddressTo();
            blockingaddresses.append(IsEmpty(ptx->GetAddressesTo()) ? "" : *ptx->GetAddressesTo());
            if ((blockingaddresses).find(*ptx->GetAddress()) != std::string::npos)
                return {false, ConsensusResult_SelfBlocking};

            return Success;
        }
    protected:
        ConsensusValidateResult ValidateBlock(const BlockingRef& ptx, const PocketBlockRef& block) override
        {
            for (auto& blockTx: *block)
            {
                if (!TransactionHelper::IsIn(*blockTx->GetType(), {ACTION_BLOCKING, ACTION_BLOCKING_CANCEL}))
                    continue;

                if (*blockTx->GetHash() == *ptx->GetHash())
                    continue;

                auto blockPtx = static_pointer_cast<Blocking>(blockTx);

                // TODO (aok, o1q): disable blocking 1-1 - allow only 1-N
                if (*ptx->GetAddress() == *blockPtx->GetAddress()) {
                    if (!IsEmpty(ptx->GetAddressTo()) &&
                        !IsEmpty(blockPtx->GetAddressTo()) &&
                        *ptx->GetAddressTo() == *blockPtx->GetAddressTo())
                        return {false, ConsensusResult_ManyTransactions};
                    if (!IsEmpty(ptx->GetAddressesTo()) &&
                        !IsEmpty(blockPtx->GetAddressesTo()))
                        return {false, ConsensusResult_ManyTransactions};
                }
            }

            return Success;
        }
        ConsensusValidateResult ValidateMempool(const BlockingRef& ptx) override
        {
            if (ConsensusRepoInst.CountMempoolBlocking(*ptx->GetAddress(), IsEmpty(ptx->GetAddressTo()) ? "" : *ptx->GetAddressTo()) > 0)
                return {false, ConsensusResult_ManyTransactions};

            return Success;
        }
        vector <string> GetAddressesForCheckRegistration(const BlockingRef& ptx) override
        {
            vector <string> addresses;
            if (!IsEmpty(ptx->GetAddressTo()))
                addresses.emplace_back(*ptx->GetAddressTo());

            if (!IsEmpty(ptx->GetAddressesTo()))
            {
                UniValue addrs(UniValue::VARR);
                addrs.read(*ptx->GetAddressesTo());
                for (size_t i = 0; i < addrs.size(); ++i)
                    addresses.emplace_back(addrs[i].get_str());
            }
            return addresses;
        }
    };


    // ----------------------------------------------------------------------------------------------
    // Factory for select actual rules version
    class BlockingConsensusFactory : public BaseConsensusFactory<BlockingConsensus>
    {
    public:
        BlockingConsensusFactory()
        {
            Checkpoint({       0,       0, -1, make_shared<BlockingConsensus>() });
            Checkpoint({ 1873500, 1114500,  0, make_shared<BlockingConsensus_checkpoint_multiple_blocking>() });
        }
    };

    static BlockingConsensusFactory ConsensusFactoryInst_Blocking;
}

#endif // POCKETCONSENSUS_BLOCKING_HPP