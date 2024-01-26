// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETCONSENSUS_SOCIAL_H
#define POCKETCONSENSUS_SOCIAL_H

#include <utility>

#include "univalue/include/univalue.h"
#include "core_io.h"

#include "pocketdb/pocketnet.h"
#include "pocketdb/models/base/Base.h"
#include "pocketdb/consensus/Base.h"
#include "pocketdb/helpers/TransactionHelper.h"
#include "pocketdb/util/Empty.h"

namespace PocketConsensus
{
    using namespace std;
    using namespace PocketDb;
    using namespace PocketUtil;
    using PocketTx::TxType;

    template<class T>
    class SocialConsensus : public BaseConsensus
    {
    private:
        typedef shared_ptr<T> TRef;

    public:
        SocialConsensus() : BaseConsensus()
        {
            Limits.Set("payload_size", 2048, 2048, 2048);
        }

        // Validate transaction in block for miner & network full block sync
        virtual ConsensusValidateResult Validate(const CTransactionRef& tx, const TRef& ptx, const PocketBlockRef& block)
        {
            Result(ConsensusResult_NotRegistered, [&]()
            {
                // Account must be registered
                vector<string> addressesForCheck;
                vector<string> addresses = GetAddressesForCheckRegistration(ptx);
                if (!addresses.empty())
                {
                    // First check block - maybe user registration this?
                    if (block)
                    {
                        for (const string& address : addresses)
                        {
                            bool inBlock = false;
                            for (auto& blockTx: *block)
                            {
                                if (!TransactionHelper::IsIn(*blockTx->GetType(), { ACCOUNT_USER, ACCOUNT_DELETE }))
                                    continue;

                                if (*blockTx->GetString1() == address)
                                {
                                    inBlock = true;
                                    break;
                                }
                            }

                            if (!inBlock)
                                addressesForCheck.push_back(address);
                        }
                    }
                    else
                    {
                        addressesForCheck = addresses;
                    }
                }

                // Check registrations in DB
                return (!addressesForCheck.empty() && !PocketDb::ConsensusRepoInst.ExistsUserRegistrations(addressesForCheck));
            });
            if (ResultCode != ConsensusResult_Success) return {false, ResultCode};

            // Check active account ban
            Result(ConsensusResult_AccountBanned, [&]()
            {
                // This code disables the prohibition of transactions for accounts banned by the moderation system.
                // This is a temporary measure to study the behavior of the system and make a final decision on the issues of the punishment system.
                return false;
                    
                return PocketDb::ConsensusRepoInst.ExistsAccountBan(*ptx->GetString1(), Height);
            });
            if (ResultCode != ConsensusResult_Success) return {false, ResultCode};

            // Check limits
            if (block)
                return ValidateBlock(ptx, block);
            else
                return ValidateMempool(ptx);
        }

        // Generic transactions validating
        virtual ConsensusValidateResult Check(const CTransactionRef& tx, const TRef& ptx)
        {
            // All social transactions must have an address
            Result(ConsensusResult_Failed, [&]()
            {
                return IsEmpty(ptx->GetAddress());
            });

            Result(ConsensusResult_FailedOpReturn, [&]()
            {
                auto ptxORHash = ptx->BuildHash();
                auto txORHash = TransactionHelper::ExtractOpReturnHash(tx);

                if (ptxORHash != txORHash)
                    if (!CheckpointRepoInst.IsOpReturnCheckpoint(*ptx->GetHash(), ptxORHash))
                        return true;

                return false;
            });

            Result(ConsensusResult_Size, [&]()
            {
                return ptx->PayloadSize() > (size_t)Limits.Get("payload_size");
            });

            if (ResultCode != ConsensusResult_Success) return {false, ResultCode}; // TODO (aok): remove when all consensus classes support Result
            return Success;
        }

    protected:

        virtual ConsensusValidateResult ValidateBlock(const TRef& ptx, const PocketBlockRef& block) = 0;

        virtual ConsensusValidateResult ValidateMempool(const TRef& ptx) = 0;

        virtual ConsensusValidateResult ValidateLimit(ConsensusLimit limit, int count)
        {
            if (count >= GetConsensusLimit(limit))
                return {false, ConsensusResult_ExceededLimit};

            return Success;
        }
        
        // Get addresses from transaction for check registration
        virtual vector<string> GetAddressesForCheckRegistration(const TRef& ptx)
        {
            return { *ptx->GetAddress() };
        }

        // Find transactions in block
        virtual vector<SocialTransactionRef> ExtractBlockPtxs(const PocketBlockRef& block, const shared_ptr<T>& ptx, const vector<TxType>& inclTypes)
        {
            vector<SocialTransactionRef> lst;

            for (auto& blockTx : *block)
            {
                if (!TransactionHelper::IsIn(*blockTx->GetType(), inclTypes))
                    continue;

                if (*blockTx->GetHash() == *ptx->GetHash())
                    continue;

                auto blockPtx = static_pointer_cast<SocialTransaction>(blockTx);
                if (*ptx->GetAddress() != *blockPtx->GetAddress())
                    continue;

                lst.push_back(blockPtx);
            }

            return lst;
        }
    
        virtual bool CheckBlocking(const string& address1, const string& address2, bool from1to2, bool from2to1)
        {
            if (from1to2 == from2to1)
                return false;
                
            if (from1to2 && PocketDb::ConsensusRepoInst.ExistsBlocking(address1, address2))
                return true;
            
            if (from2to1 && PocketDb::ConsensusRepoInst.ExistsBlocking(address2, address1))
                return true;

            return false;
        }
    };
}

#endif // POCKETCONSENSUS_SOCIAL_H
