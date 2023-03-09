// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include "pocketdb/consensus/Lottery.h"

namespace PocketConsensus
{
    // ----------------------------------------
    // Lottery base class
    void LotteryConsensus::SortWinners(map<string, int>& candidates, CDataStream& hashProofOfStakeSource, vector <string>& winners)
    {
        vector < pair < string, pair < int, arith_uint256>>> candidatesSorted;
        for (auto& it: candidates)
        {
            CDataStream ss(hashProofOfStakeSource);
            ss << it.first;
            arith_uint256 hashSortRating = UintToArith256(Hash(ss)) / it.second;
            candidatesSorted.emplace_back(std::make_pair(it.first, std::make_pair(it.second, hashSortRating)));
        }

        if (!candidatesSorted.empty())
        {
            std::sort(candidatesSorted.begin(), candidatesSorted.end(), [](auto& a, auto& b)
            {
                return a.second.second < b.second.second;
            });

            if ((int) candidatesSorted.size() > MaxWinnersCount())
                candidatesSorted.resize(MaxWinnersCount());

            for (auto& it : candidatesSorted)
                winners.push_back(it.first);
        }
    }

    void LotteryConsensus::ExtendReferrer(const ScoreDataDtoRef& scoreData, map <string, string>& refs)
    {

    }

    void LotteryConsensus::ExtendReferrers()
    {

    }

    // Get all lottery winner
    LotteryWinners& LotteryConsensus::Winners(const CBlock& block, CDataStream& hashProofOfStakeSource)
    {
        auto reputationConsensus = PocketConsensus::ReputationConsensusFactoryInst.Instance(Height);

        map<string, int> postCandidates;
        map <string, string> postReferrersCandidates;

        map<string, int> commentCandidates;
        map <string, string> commentReferrersCandidates;

        for (const auto& tx : block.vtx)
        {
            // Get destination address and score value
            // In lottery allowed only likes to posts and comments
            // Also in lottery allowed only positive scores
            auto[parseScoreOk, scoreTxData] = TransactionHelper::ParseScore(tx);
            if (!parseScoreOk)
                continue;

            if (scoreTxData->ScoreType == ACTION_SCORE_COMMENT
                && scoreTxData->ScoreValue != 1)
                continue;

            if (scoreTxData->ScoreType == ACTION_SCORE_CONTENT
                && scoreTxData->ScoreValue != 4 && scoreTxData->ScoreValue != 5)
                continue;

            auto scoreData = PocketDb::ConsensusRepoInst.GetScoreData(tx->GetHash().GetHex());
            if (!scoreData)
            {
                LogPrintf("%s: Failed get score data for tx: %s\n", __func__, tx->GetHash().GetHex());
                continue;
            }

            if (!reputationConsensus->AllowModifyReputation(
                scoreData,
                true
            ))
                continue;

            if (scoreData->ScoreType == ACTION_SCORE_CONTENT)
            {
                postCandidates[scoreData->ContentAddressHash] += (scoreData->ScoreValue - 3);
                ExtendReferrer(scoreData, postReferrersCandidates);
            }

            if (scoreData->ScoreType == ACTION_SCORE_COMMENT)
            {
                commentCandidates[scoreData->ContentAddressHash] += scoreData->ScoreValue;
                ExtendReferrer(scoreData, commentReferrersCandidates);
            }
        }

        // Sort founded users
        SortWinners(postCandidates, hashProofOfStakeSource, _winners.PostWinners);
        SortWinners(commentCandidates, hashProofOfStakeSource, _winners.CommentWinners);

        // Extend referrers
        {
            if (!postReferrersCandidates.empty())
                for (auto& winner : _winners.PostWinners)
                    if (postReferrersCandidates.find(winner) != postReferrersCandidates.end())
                        _winners.PostReferrerWinners.push_back(postReferrersCandidates[winner]);

            if (!commentReferrersCandidates.empty())
                for (auto& winner : _winners.CommentWinners)
                    if (commentReferrersCandidates.find(winner) != commentReferrersCandidates.end())
                        _winners.CommentReferrerWinners.push_back(commentReferrersCandidates[winner]);

            // Find referrers for final winners list
            // This function replaced function ExtendReferrer with wrong logic (Time -> Height)
            ExtendReferrers();
        }

        return _winners;
    }

    CAmount LotteryConsensus::RatingReward(CAmount nCredit, opcodetype code)
    {
        if (code == OP_WINNER_COMMENT) return nCredit * 0.5 / 10;
        return nCredit * 0.5;
    }

    void LotteryConsensus::ExtendWinnerTypes(opcodetype type, std::vector <opcodetype>& winner_types)
    {

    }

    // ----------------------------------------
    // Lottery checkpoint at 514185 block
    void LotteryConsensus_checkpoint_514185::ExtendReferrer(const ScoreDataDtoRef& scoreData, map <string, string>& refs)
    {
        if (refs.find(scoreData->ContentAddressHash) != refs.end())
            return;

        auto[ok, referrer] = PocketDb::ConsensusRepoInst.GetReferrer(scoreData->ContentAddressHash);
        if (!ok || referrer == scoreData->ScoreAddressHash) return;

        refs.emplace(scoreData->ContentAddressHash, referrer);
    }
    void LotteryConsensus_checkpoint_514185::ExtendWinnerTypes(opcodetype type, std::vector <opcodetype>& winner_types)
    {
        winner_types.push_back(type);
    }
    CAmount LotteryConsensus_checkpoint_514185::RatingReward(CAmount nCredit, opcodetype code)
    {
        // Referrer program 5 - 100%; 2.0 - nodes; 3.0 - all for lottery;
        // 2.0 - posts; 0.4 - referrer over posts (20%); 0.5 - comment; 0.1 - referrer over comment (20%);
        if (code == OP_WINNER_POST) return nCredit * 0.40;
        if (code == OP_WINNER_POST_REFERRAL) return nCredit * 0.08;
        if (code == OP_WINNER_COMMENT) return nCredit * 0.10;
        if (code == OP_WINNER_COMMENT_REFERRAL) return nCredit * 0.02;
        return 0;
    }

    // ----------------------------------------
    // Lottery checkpoint at 1035000 block
    void LotteryConsensus_checkpoint_1035000::ExtendReferrer(const ScoreDataDtoRef& scoreData, map<string, string>& refs)
    {
        if (refs.find(scoreData->ContentAddressHash) != refs.end())
            return;

        auto regTime = PocketDb::ConsensusRepoInst.GetAccountRegistrationTime(scoreData->ContentAddressId);
        if (regTime < (scoreData->ScoreTime - GetConsensusLimit(ConsensusLimit_lottery_referral_depth))) return;

        auto[ok, referrer] = PocketDb::ConsensusRepoInst.GetReferrer(scoreData->ContentAddressHash);
        if (!ok || referrer == scoreData->ScoreAddressHash) return;

        refs.emplace(scoreData->ContentAddressHash, referrer);
    }

    // ----------------------------------------
    // Lottery checkpoint at 1124000 block
    CAmount LotteryConsensus_checkpoint_1124000::RatingReward(CAmount nCredit, opcodetype code)
    {
        // Referrer program 5 - 100%; 4.75 - nodes; 0.25 - all for lottery;
        // .1 - posts (2%); .1 - referrer over posts (2%); 0.025 - comment (.5%); 0.025 - referrer over comment (.5%);
        if (code == OP_WINNER_POST) return nCredit * 0.02;
        if (code == OP_WINNER_POST_REFERRAL) return nCredit * 0.02;
        if (code == OP_WINNER_COMMENT) return nCredit * 0.005;
        if (code == OP_WINNER_COMMENT_REFERRAL) return nCredit * 0.005;
        return 0;
    }

    // ----------------------------------------
    // Lottery checkpoint at 1180000 block
    CAmount LotteryConsensus_checkpoint_1180000::RatingReward(CAmount nCredit, opcodetype code)
    {
        // Reduce all winnings by 10 times
        // Referrer program 5 - 100%; 4.975 - nodes; 0.025 - all for lottery;
        if (code == OP_WINNER_POST) return nCredit * 0.002;
        if (code == OP_WINNER_POST_REFERRAL) return nCredit * 0.002;
        if (code == OP_WINNER_COMMENT) return nCredit * 0.0005;
        if (code == OP_WINNER_COMMENT_REFERRAL) return nCredit * 0.0005;
        return 0;
    }

    // ----------------------------------------
    // Lottery checkpoint at 2162400 block
    // Disable lottery payments for likes to comments.
    // Also disable referral program
    CAmount LotteryConsensus_bip_100::RatingReward(CAmount nCredit, opcodetype code)
    {
        if (code == OP_WINNER_POST) return nCredit * 0.025;
        return 0;
    }

    LotteryWinners& LotteryConsensus_bip_100::Winners(const CBlock& block, CDataStream& hashProofOfStakeSource)
    {
        auto reputationConsensus = PocketConsensus::ReputationConsensusFactoryInst.Instance(Height);

        map<string, int> postCandidates;

        for (const auto& tx : block.vtx)
        {
            // Get destination address and score value
            // In lottery allowed only likes to posts and comments
            // Also in lottery allowed only positive scores
            auto[parseScoreOk, scoreTxData] = TransactionHelper::ParseScore(tx);
            if (!parseScoreOk)
                continue;

            // BIP100: Only scores to content allowed
            if (scoreTxData->ScoreType == ACTION_SCORE_COMMENT)
                continue;
            if (scoreTxData->ScoreType == ACTION_SCORE_CONTENT
                && scoreTxData->ScoreValue != 4 && scoreTxData->ScoreValue != 5)
                continue;

            auto scoreData = PocketDb::ConsensusRepoInst.GetScoreData(tx->GetHash().GetHex());
            if (!scoreData)
            {
                LogPrintf("%s: Failed get score data for tx: %s\n", __func__, tx->GetHash().GetHex());
                continue;
            }

            if (!reputationConsensus->AllowModifyReputation(
                scoreData,
                true
            ))
                continue;

            postCandidates[scoreData->ContentAddressHash] += (scoreData->ScoreValue - 3);
        }

        // Sort founded users
        SortWinners(postCandidates, hashProofOfStakeSource, _winners.PostWinners);

        return _winners;
    }

    // ----------------------------------------
    // Lottery checkpoint at _ block
    void LotteryConsensus_checkpoint_::ExtendReferrer(const ScoreDataDtoRef& scoreData, map<string, string>& refs)
    {
        // This logic replaced with ExtendReferrers()
    }

    void LotteryConsensus_checkpoint_::ExtendReferrers()
    {
        auto& postWinners = _winners.PostWinners;
        auto& postRefs = _winners.PostReferrerWinners;
        auto& commentWinners = _winners.CommentWinners;
        auto& commentRefs = _winners.CommentReferrerWinners;

        vector <string> winners;
        for (const string& addr : postWinners)
            if (find(winners.begin(), winners.end(), addr) == winners.end())
                winners.push_back(addr);

        for (const string& addr : commentWinners)
            if (find(winners.begin(), winners.end(), addr) == winners.end())
                winners.push_back(addr);

        auto referrers = PocketDb::ConsensusRepoInst.GetReferrers(winners, Height - GetConsensusLimit(ConsensusLimit_lottery_referral_depth));
        if (referrers->empty()) return;

        for (const auto& it : *referrers)
        {
            if (find(postWinners.begin(), postWinners.end(), it.first) != postWinners.end())
                if (find(postRefs.begin(), postRefs.end(), it.second) == postRefs.end())
                    postRefs.push_back(it.second);

            if (find(commentWinners.begin(), commentWinners.end(), it.first) != commentWinners.end())
                if (find(commentRefs.begin(), commentRefs.end(), it.second) == commentRefs.end())
                    commentRefs.push_back(it.second);
        }
    }

    // ----------------------------------------
    LotteryConsensusFactory LotteryConsensusFactoryInst;
}