// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 Bitcoin developers
// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETCONSENSUS_SCORECOMMENT_HPP
#define POCKETCONSENSUS_SCORECOMMENT_HPP

#include "pocketdb/consensus/social/Base.hpp"
#include "pocketdb/models/dto/ScoreComment.hpp"
#include "pocketdb/consensus/Reputation.hpp"

namespace PocketConsensus
{
    /*******************************************************************************************************************
    *
    *  ScoreComment consensus base class
    *
    *******************************************************************************************************************/
    class ScoreCommentConsensus : public SocialBaseConsensus
    {
    public:
        ScoreCommentConsensus(int height) : SocialBaseConsensus(height) {}
        ScoreCommentConsensus() : SocialBaseConsensus() {}

        tuple<bool, SocialConsensusResult> Validate(shared_ptr<Transaction> tx, PocketBlock& block) override
        {
            if (auto[ok, result] = SocialBaseConsensus::Validate(tx, block); !ok)
                return make_tuple(false, result);

            if (auto[ok, result] = Validate(static_pointer_cast<ScoreComment>(tx), block); !ok)
                return make_tuple(false, result);
                
            return make_tuple(true, SocialConsensusResult_Success);
        }

        tuple<bool, SocialConsensusResult> Check(shared_ptr<Transaction> tx) override
        {
            if (auto[ok, result] = SocialBaseConsensus::Check(tx); !ok)
                return make_tuple(false, result);

            if (auto[ok, result] = Check(static_pointer_cast<ScoreComment>(tx)); !ok)
                return make_tuple(false, result);
                
            return make_tuple(true, SocialConsensusResult_Success);
        }

    protected:
        virtual int64_t GetFullAccountScoreCommentLimit() { return 600; }
        virtual int64_t GetTrialAccountScoreCommentLimit() { return 300; }

        virtual int64_t GetScoreCommentLimit(AccountMode mode)
        {
            return mode == AccountMode_Full ? GetFullAccountScoreCommentLimit() : GetTrialAccountScoreCommentLimit();
        }

        virtual tuple<bool, SocialConsensusResult> Validate(shared_ptr<ScoreComment> tx, PocketBlock& block)
        {
            vector<string> addresses = {*tx->GetAddress()};
            if (!PocketDb::ConsensusRepoInst.ExistsUserRegistrations(addresses))
                return make_tuple(false, SocialConsensusResult_NotRegistered);

            // // Check score to self
            auto commentAddress = PocketDb::ConsensusRepoInst.GetLastActiveCommentAddress(*tx->GetCommentTxHash());

            if (commentAddress != nullptr)
            {
                if (*tx->GetAddress() == *commentAddress)
                {
                    return make_tuple(false, SocialConsensusResult_SelfCommentScore);
                }
            }
            else
            {
                auto notFound = true;

                for (const auto& otherTx : block)
                {
                    if (*otherTx->GetType() == CONTENT_COMMENT)
                    {
                        auto comment = dynamic_cast<Comment&>(*otherTx); //TODO check cast

                        if (*comment.GetRootTxHash() == *tx->GetCommentTxHash())
                        {
                            notFound = false;
                            commentAddress = comment.GetAddress();
                        }
                    }
                    else if (*otherTx->GetType() == CONTENT_COMMENT_DELETE)
                    {
                        auto comment = dynamic_cast<CommentDelete&>(*otherTx);

                        if (*comment.GetRootTxHash() == *tx->GetCommentTxHash())
                        {
                            notFound = true;
                            break;
                        }
                    }
                }

                if (notFound)
                {
                    return make_tuple(false, SocialConsensusResult_NotFound);
                }
            }

            // Blocking
            if (Height >= Params().GetConsensus().score_blocking_on
                && Height < Params().GetConsensus().score_blocking_off)
            {
                auto[existsBlocking, blockingType] = PocketDb::ConsensusRepoInst.GetLastBlockingType(*commentAddress, *tx->GetAddress());
                if (existsBlocking && blockingType == ACTION_BLOCKING)
                {
                    return make_tuple(false, SocialConsensusResult_Blocking);
                }
            }

            if (PocketDb::ConsensusRepoInst.ExistsScore(*tx->GetAddress(), *tx->GetCommentTxHash(), ACTION_SCORE_COMMENT))
            {
                return make_tuple(false, SocialConsensusResult_DoubleCommentScore);
            }

            // // Check limit scores
            // {
            //     reindexer::QueryResults scoresRes;
            //     if (!g_pocketdb->DB()->Select(
            //                             reindexer::Query("CommentScores")
            //                                 .Where("address", CondEq, _address)
            //                                 .Where("time", CondGe, _time - 86400)
            //                                 .Where("block", CondLt, height),
            //                             scoresRes)
            //             .ok()) {
            //         result = ANTIBOTRESULT::Failed;
            //         return false;
            //     }

            //     int scoresCount = scoresRes.Count();

            //     // Also check mempool
            //     if (checkMempool) {
            //         reindexer::QueryResults res;
            //         if (g_pocketdb->Select(reindexer::Query("Mempool").Where("table", CondEq, "CommentScores").Not().Where("txid", CondEq, _txid), res).ok()) {
            //             for (auto& m : res) {
            //                 reindexer::Item mItm = m.GetItem();
            //                 std::string t_src = DecodeBase64(mItm["data"].As<string>());

            //                 reindexer::Item t_itm = g_pocketdb->DB()->NewItem("CommentScores");
            //                 if (t_itm.FromJSON(t_src).ok()) {
            //                     if (t_itm["address"].As<string>() == _address) {
            //                         if (!checkWithTime || t_itm["time"].As<int64_t>() <= _time)
            //                             scoresCount += 1;

            //                         if (t_itm["commentid"].As<string>() == _comment_id) {
            //                             result = ANTIBOTRESULT::DoubleCommentScore;
            //                             return false;
            //                         }
            //                     }
            //                 }
            //             }
            //         }
            //     }

            //     // Check block
            //     if (blockVtx.Exists("CommentScores")) {
            //         for (auto& mtx : blockVtx.Data["CommentScores"]) {
            //             if (mtx["txid"].get_str() != _txid && mtx["address"].get_str() == _address) {
            //                 if (!checkWithTime || mtx["time"].get_int64() <= _time)
            //                     scoresCount += 1;

            //                 if (mtx["commentid"].get_str() == _comment_id) {
            //                     result = ANTIBOTRESULT::DoubleCommentScore;
            //                     return false;
            //                 }
            //             }
            //         }
            //     }

            auto reputationConsensus = ReputationConsensusFactory::Instance(Height);
            auto accountMode = reputationConsensus->GetAccountMode(*tx->GetAddress());
            auto limit = GetScoreCommentLimit(accountMode);
            //TODO (joni): Get scoresCount

            //     ABMODE mode;
            //     getMode(_address, mode, height);
            //     int limit = getLimit(CommentScore, mode, height);
            //     if (scoresCount >= limit) {
            //         result = ANTIBOTRESULT::CommentScoreLimit;
            //         return false;
            //     }
            // }

            // // Check OP_RETURN
            // std::vector<std::string> vasm;
            // boost::split(vasm, oitm["asm"].get_str(), boost::is_any_of("\t "));

            // // Check address and value in asm == reindexer data
            // if (vasm.size() >= 4) {
            //     std::stringstream _op_return_data;
            //     _op_return_data << vasm[3];
            //     std::string _op_return_hex = _op_return_data.str();

            //     std::string _score_itm_val = _comment_address + " " + std::to_string(_score_value);
            //     std::string _score_itm_hex = HexStr(_score_itm_val.begin(), _score_itm_val.end());

            //     if (_op_return_hex != _score_itm_hex) {
            //         result = ANTIBOTRESULT::OpReturnFailed;
            //         return false;
            //     }
            // }

            return make_tuple(true, SocialConsensusResult_Success);
        }
        
    private:

        tuple<bool, SocialConsensusResult> Check(shared_ptr<ScoreComment> tx)
        {
            auto value = *tx->GetValue();
            if (value != 1 && value != -1)
            {
                return make_tuple(false, SocialConsensusResult_Failed);
            }

            return make_tuple(true, SocialConsensusResult_Success);
        }
    };

    /*******************************************************************************************************************
    *
    *  Consensus checkpoint at 1 block
    *
    *******************************************************************************************************************/
    class ScoreCommentConsensus_checkpoint_1 : public ScoreCommentConsensus
    {
    protected:
        int CheckpointHeight() override { return 1; }
    public:
        ScoreCommentConsensus_checkpoint_1(int height) : ScoreCommentConsensus(height) {}
    };


    /*******************************************************************************************************************
    *
    *  Factory for select actual rules version
    *  Каждая новая перегрузка добавляет новый функционал, поддерживающийся с некоторым условием - например высота
    *
    *******************************************************************************************************************/
    class ScoreCommentConsensusFactory
    {
    private:
        static inline const std::map<int, std::function<ScoreCommentConsensus*(int height)>> m_rules =
        {
            {1, [](int height) { return new ScoreCommentConsensus_checkpoint_1(height); }},
            {0, [](int height) { return new ScoreCommentConsensus(height); }},
        };
    public:
        shared_ptr <ScoreCommentConsensus> Instance(int height)
        {
            return shared_ptr<ScoreCommentConsensus>(
                (--m_rules.upper_bound(height))->second(height)
            );
        }
    };
}

#endif // POCKETCONSENSUS_SCORECOMMENT_HPP