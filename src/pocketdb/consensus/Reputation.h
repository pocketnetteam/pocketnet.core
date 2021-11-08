// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETCONSENSUS_REPUTATION_H
#define POCKETCONSENSUS_REPUTATION_H

#include "pocketdb/consensus/Base.h"

namespace PocketConsensus
{
    using namespace std;

    // ------------------------------------------
    // Consensus checkpoint at 0 block
    class ReputationConsensus : public BaseConsensus
    {
    protected:
        virtual int64_t GetMinLikers(int addressId);
        virtual bool AllowModifyReputation(int addressId);
        virtual tuple<int, string> SelectAddressScoreContent(shared_ptr<ScoreDataDto>& scoreData, bool lottery);
        virtual bool AllowModifyReputationOverPost(shared_ptr<ScoreDataDto>& scoreData, bool lottery);
        virtual bool AllowModifyReputationOverComment(shared_ptr<ScoreDataDto>& scoreData, bool lottery);

    public:
        explicit ReputationConsensus(int height) : BaseConsensus(height) {}

        virtual AccountMode GetAccountMode(int reputation, int64_t balance);
        virtual tuple<AccountMode, int, int64_t> GetAccountMode(string& address);
        virtual bool AllowModifyReputation(shared_ptr<ScoreDataDto>& scoreData, bool lottery);
        virtual bool AllowModifyOldPosts(int64_t scoreTime, int64_t contentTime, TxType contentType);
        virtual void PrepareAccountLikers(map<int, vector<int>>& accountLikersSrc, map<int, vector<int>>& accountLikers);
    };

    // ------------------------------------------
    // Consensus checkpoint at 151600 block
    class ReputationConsensus_checkpoint_151600 : public ReputationConsensus
    {
    public:
        explicit ReputationConsensus_checkpoint_151600(int height) : ReputationConsensus(height) {}
    protected:
        tuple<int, string> SelectAddressScoreContent(shared_ptr<ScoreDataDto>& scoreData, bool lottery) override;
    };

    // ------------------------------------------
    // Consensus checkpoint at 1180000 block
    class ReputationConsensus_checkpoint_1180000 : public ReputationConsensus_checkpoint_151600
    {
    public:
        explicit ReputationConsensus_checkpoint_1180000(int height) : ReputationConsensus_checkpoint_151600(height) {}
    protected:
        int64_t GetMinLikers(int addressId) override;
    };

    // ------------------------------------------
    // Consensus checkpoint at 1324655 block
    class ReputationConsensus_checkpoint_1324655 : public ReputationConsensus_checkpoint_1180000
    {
    public:
        explicit ReputationConsensus_checkpoint_1324655(int height) : ReputationConsensus_checkpoint_1180000(height) {}
        AccountMode GetAccountMode(int reputation, int64_t balance) override;
    };

    // ------------------------------------------
    // Consensus checkpoint at 1324655_2 block
    class ReputationConsensus_checkpoint_1324655_2 : public ReputationConsensus_checkpoint_1324655
    {
    public:
        explicit ReputationConsensus_checkpoint_1324655_2(int height) : ReputationConsensus_checkpoint_1324655(height) {}
        void PrepareAccountLikers(map<int, vector<int>>& accountLikersSrc, map<int, vector<int>>& accountLikers) override;
    };

    // ------------------------------------------
    //  Factory for select actual rules version
    class ReputationConsensusFactory
    {
    private:
        const vector<ConsensusCheckpoint<ReputationConsensus>> m_rules = {
            {0,       -1,    [](int height) { return make_shared<ReputationConsensus>(height); }},
            {151600,  -1,    [](int height) { return make_shared<ReputationConsensus_checkpoint_151600>(height); }},
            {1180000, 0,     [](int height) { return make_shared<ReputationConsensus_checkpoint_1180000>(height); }},
            {1324655, 65000, [](int height) { return make_shared<ReputationConsensus_checkpoint_1324655>(height); }},
            {1324655, 75000, [](int height) { return make_shared<ReputationConsensus_checkpoint_1324655_2>(height); }},
        };
    public:
        shared_ptr<ReputationConsensus> Instance(int height)
        {
            int m_height = (height > 0 ? height : 0);
            return (--upper_bound(m_rules.begin(), m_rules.end(), m_height,
                [&](int target, const ConsensusCheckpoint<ReputationConsensus>& itm)
                {
                    return target < itm.Height(Params().NetworkIDString());
                }
            ))->m_func(height);
        }
    };

    extern ReputationConsensusFactory ReputationConsensusFactoryInst;
}

#endif // POCKETCONSENSUS_REPUTATION_H