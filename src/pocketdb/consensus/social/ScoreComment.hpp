// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 Bitcoin developers
// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETCONSENSUS_SCORECOMMENT_HPP
#define POCKETCONSENSUS_SCORECOMMENT_HPP

#include "pocketdb/consensus/Base.hpp"

namespace PocketConsensus
{
    /*******************************************************************************************************************
    *
    *  ScoreComment consensus base class
    *
    *******************************************************************************************************************/
    class ScoreCommentConsensus : public SocialBaseConsensus
    {
    protected:
    public:
        ScoreCommentConsensus(int height) : SocialBaseConsensus(height) {}

        tuple<bool, SocialConsensusResult> Validate(PocketBlock& txs) override
        {
            std::string _txid = oitm["txid"].get_str();
            std::string _address = oitm["address"].get_str();
            std::string _comment_id = oitm["commentid"].get_str();
            int _score_value = oitm["value"].get_int();
            int64_t _time = oitm["time"].get_int64();

            if (_score_value != -1 && _score_value != 1) {
                result = ANTIBOTRESULT::Failed;
                return false;
            }

            if (!CheckRegistration(oitm, _address, checkMempool, checkWithTime, height, blockVtx, result)) {
                return false;
            }

            // Check score to self
            bool not_found = false;
            std::string _comment_address;
            reindexer::Item commentItm;
            if (g_pocketdb->SelectOne(reindexer::Query("Comment").Where("otxid", CondEq, _comment_id).Where("last", CondEq, true).Not().Where("msg", CondEq, "").Where("block", CondLt, height), commentItm).ok()) {
                _comment_address = commentItm["address"].As<string>();

                // Score to self comment
                if (_comment_address == _address) {
                    result = ANTIBOTRESULT::SelfCommentScore;
                    return false;
                }
            } else {
                // Comment not found
                not_found = true;

                // Maybe in current block?
                if (blockVtx.Exists("Comment")) {
                    for (auto& mtx : blockVtx.Data["Comment"]) {
                        if (mtx["otxid"].get_str() == _comment_id && mtx["msg"].get_str() != "") {
                            _comment_address = mtx["address"].get_str();
                            not_found = false;
                            break;
                        }
                    }
                }

                if (not_found) {
                    result = ANTIBOTRESULT::NotFound;
                    return false;
                }
            }

            // Blocking
            if (height >= Params().GetConsensus().score_blocking_on && height < Params().GetConsensus().score_blocking_off && g_pocketdb->Exists(Query("BlockingView").Where("address", CondEq, _comment_address).Where("address_to", CondEq, _address).Where("block", CondLt, height))) {
                result = ANTIBOTRESULT::Blocking;
                return false;
            }

            // Check double score to comment
            reindexer::Item doubleScoreItm;
            if (g_pocketdb->SelectOne(
                            reindexer::Query("CommentScores")
                                .Where("address", CondEq, _address)
                                .Where("commentid", CondEq, _comment_id)
                                .Where("block", CondLt, height),
                            doubleScoreItm)
                    .ok()) {
                result = ANTIBOTRESULT::DoubleCommentScore;
                return false;
            }

            // Check limit scores
            {
                reindexer::QueryResults scoresRes;
                if (!g_pocketdb->DB()->Select(
                                        reindexer::Query("CommentScores")
                                            .Where("address", CondEq, _address)
                                            .Where("time", CondGe, _time - 86400)
                                            .Where("block", CondLt, height),
                                        scoresRes)
                        .ok()) {
                    result = ANTIBOTRESULT::Failed;
                    return false;
                }

                int scoresCount = scoresRes.Count();

                // Also check mempool
                if (checkMempool) {
                    reindexer::QueryResults res;
                    if (g_pocketdb->Select(reindexer::Query("Mempool").Where("table", CondEq, "CommentScores").Not().Where("txid", CondEq, _txid), res).ok()) {
                        for (auto& m : res) {
                            reindexer::Item mItm = m.GetItem();
                            std::string t_src = DecodeBase64(mItm["data"].As<string>());

                            reindexer::Item t_itm = g_pocketdb->DB()->NewItem("CommentScores");
                            if (t_itm.FromJSON(t_src).ok()) {
                                if (t_itm["address"].As<string>() == _address) {
                                    if (!checkWithTime || t_itm["time"].As<int64_t>() <= _time)
                                        scoresCount += 1;

                                    if (t_itm["commentid"].As<string>() == _comment_id) {
                                        result = ANTIBOTRESULT::DoubleCommentScore;
                                        return false;
                                    }
                                }
                            }
                        }
                    }
                }

                // Check block
                if (blockVtx.Exists("CommentScores")) {
                    for (auto& mtx : blockVtx.Data["CommentScores"]) {
                        if (mtx["txid"].get_str() != _txid && mtx["address"].get_str() == _address) {
                            if (!checkWithTime || mtx["time"].get_int64() <= _time)
                                scoresCount += 1;

                            if (mtx["commentid"].get_str() == _comment_id) {
                                result = ANTIBOTRESULT::DoubleCommentScore;
                                return false;
                            }
                        }
                    }
                }

                ABMODE mode;
                getMode(_address, mode, height);
                int limit = getLimit(CommentScore, mode, height);
                if (scoresCount >= limit) {
                    result = ANTIBOTRESULT::CommentScoreLimit;
                    return false;
                }
            }

            // Check OP_RETURN
            std::vector<std::string> vasm;
            boost::split(vasm, oitm["asm"].get_str(), boost::is_any_of("\t "));

            // Check address and value in asm == reindexer data
            if (vasm.size() >= 4) {
                std::stringstream _op_return_data;
                _op_return_data << vasm[3];
                std::string _op_return_hex = _op_return_data.str();

                std::string _score_itm_val = _comment_address + " " + std::to_string(_score_value);
                std::string _score_itm_hex = HexStr(_score_itm_val.begin(), _score_itm_val.end());

                if (_op_return_hex != _score_itm_hex) {
                    result = ANTIBOTRESULT::OpReturnFailed;
                    return false;
                }
            }

            return true;
        }

    };


    /*******************************************************************************************************************
    *
    *  Start checkpoint
    *
    *******************************************************************************************************************/
    class ScoreCommentConsensus_checkpoint_0 : public ScoreCommentConsensus
    {
    protected:
    public:

        ScoreCommentConsensus_checkpoint_0(int height) : ScoreCommentConsensus(height) {}

    }; // class ScoreCommentConsensus_checkpoint_0


    /*******************************************************************************************************************
    *
    *  Consensus checkpoint at 1 block
    *
    *******************************************************************************************************************/
    class ScoreCommentConsensus_checkpoint_1 : public ScoreCommentConsensus_checkpoint_0
    {
    protected:
        int CheckpointHeight() override { return 1; }
    public:
        ScoreCommentConsensus_checkpoint_1(int height) : ScoreCommentConsensus_checkpoint_0(height) {}
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
        inline static std::vector<std::pair<int, std::function<ScoreCommentConsensus*(int height)>>> m_rules
            {
                {1, [](int height) { return new ScoreCommentConsensus_checkpoint_1(height); }},
                {0, [](int height) { return new ScoreCommentConsensus_checkpoint_0(height); }},
            };
    public:
        shared_ptr <ScoreCommentConsensus> Instance(int height)
        {
            for (const auto& rule : m_rules)
                if (height >= rule.first)
                    return shared_ptr<ScoreCommentConsensus>(rule.second(height));
        }
    };
}

#endif // POCKETCONSENSUS_SCORECOMMENT_HPP