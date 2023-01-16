// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETCONSENSUS_LOTTERY_H
#define POCKETCONSENSUS_LOTTERY_H

#include "streams.h"
#include <boost/algorithm/string.hpp>
#include "pocketdb/consensus/Reputation.h"
#include "pocketdb/consensus/Base.h"
#include "pocketdb/helpers/TransactionHelper.h"

namespace PocketConsensus
{
    using namespace std;
    using namespace PocketTx;
    using namespace PocketHelpers;

    struct LotteryWinners
    {
        vector<string> PostWinners;
        vector<string> PostReferrerWinners;
        vector<string> CommentWinners;
        vector<string> CommentReferrerWinners;
    };

    // ---------------------------------------
    // Lottery base class
    class LotteryConsensus : public BaseConsensus
    {
    protected:
        LotteryWinners _winners;
        virtual int64_t MaxWinnersCount() { return 25; }

        void SortWinners(map<string, int>& candidates, CDataStream& hashProofOfStakeSource, vector<string>& winners);
        virtual void ExtendReferrer(const ScoreDataDtoRef& scoreData, map<string, string>& refs);
        virtual void ExtendReferrers();

    public:
        explicit LotteryConsensus(int height) : BaseConsensus(height) {}

        // Get all lottery winner
        virtual LotteryWinners& Winners(const CBlock& block, CDataStream& hashProofOfStakeSource);
        virtual CAmount RatingReward(CAmount nCredit, opcodetype code);
        virtual void ExtendWinnerTypes(opcodetype type, std::vector<opcodetype>& winner_types);
    };

    // ---------------------------------------
    // Lottery checkpoint at 514185 block
    class LotteryConsensus_checkpoint_514185 : public LotteryConsensus
    {
    public:
        explicit LotteryConsensus_checkpoint_514185(int height) : LotteryConsensus(height) {}
        void ExtendWinnerTypes(opcodetype type, std::vector<opcodetype>& winner_types) override;
        CAmount RatingReward(CAmount nCredit, opcodetype code) override;
    protected:
        void ExtendReferrer(const ScoreDataDtoRef& scoreData, map<string, string>& refs) override;
    };

    // ---------------------------------------
    // Lottery checkpoint at 1035000 block
    class LotteryConsensus_checkpoint_1035000 : public LotteryConsensus_checkpoint_514185
    {
    public:
        explicit LotteryConsensus_checkpoint_1035000(int height) : LotteryConsensus_checkpoint_514185(height) {}
    protected:
        void ExtendReferrer(const ScoreDataDtoRef& scoreData, map<string, string>& refs) override;
    };

    // ---------------------------------------
    // Lottery checkpoint at 1124000 block
    class LotteryConsensus_checkpoint_1124000 : public LotteryConsensus_checkpoint_1035000
    {
    public:
        explicit LotteryConsensus_checkpoint_1124000(int height) : LotteryConsensus_checkpoint_1035000(height) {}
        CAmount RatingReward(CAmount nCredit, opcodetype code) override;
    };

    // ---------------------------------------
    // Lottery checkpoint at 1180000 block
    class LotteryConsensus_checkpoint_1180000 : public LotteryConsensus_checkpoint_1124000
    {
    public:
        explicit LotteryConsensus_checkpoint_1180000(int height) : LotteryConsensus_checkpoint_1124000(height) {}
        CAmount RatingReward(CAmount nCredit, opcodetype code) override;
    };

    // ---------------------------------------
    // Lottery checkpoint at _ block
    class LotteryConsensus_checkpoint_ : public LotteryConsensus_checkpoint_1180000
    {
    public:
        explicit LotteryConsensus_checkpoint_(int height) : LotteryConsensus_checkpoint_1180000(height) {}
    protected:
        void ExtendReferrer(const ScoreDataDtoRef& scoreData, map<string, string>& refs) override;
        void ExtendReferrers() override;
    };

    // ---------------------------------------
    // Lottery factory for select actual rules version
    class LotteryConsensusFactory
    {
    private:
        const vector<ConsensusCheckpoint < LotteryConsensus>> m_rules = {
            {0,       -1, -1, [](int height) { return make_shared<LotteryConsensus>(height); }},
            {514185,  -1, -1, [](int height) { return make_shared<LotteryConsensus_checkpoint_514185>(height); }},
            {1035000, -1, -1, [](int height) { return make_shared<LotteryConsensus_checkpoint_1035000>(height); }},
            {1124000, -1, -1, [](int height) { return make_shared<LotteryConsensus_checkpoint_1124000>(height); }},
            {1180000,  0,  0, [](int height) { return make_shared<LotteryConsensus_checkpoint_1180000>(height); }},
        };
    public:
        shared_ptr<LotteryConsensus> Instance(int height)
        {
            int m_height = (height > 0 ? height : 0);
            return (--upper_bound(m_rules.begin(), m_rules.end(), m_height,
                [&](int target, const ConsensusCheckpoint<LotteryConsensus>& itm)
                {
                    return target < itm.Height(Params().NetworkID());
                }
            ))->m_func(m_height);
        }
    };

    extern LotteryConsensusFactory LotteryConsensusFactoryInst;
}

#endif // POCKETCONSENSUS_LOTTERY_H
