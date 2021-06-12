// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 Bitcoin developers
// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETCONSENSUS_SCOREPOST_HPP
#define POCKETCONSENSUS_SCOREPOST_HPP

#include "pocketdb/consensus/social/Base.hpp"
#include "pocketdb/models/base/Transaction.hpp"
#include "pocketdb/models/dto/ScorePost.hpp"

namespace PocketConsensus
{
    /*******************************************************************************************************************
    *
    *  ScorePost consensus base class
    *
    *******************************************************************************************************************/
    class ScorePostConsensus : public SocialBaseConsensus
    {
    protected:
    public:
        ScorePostConsensus(int height) : SocialBaseConsensus(height) {}
        ScorePostConsensus() : SocialBaseConsensus() {}

        tuple<bool, SocialConsensusResult> Validate(shared_ptr<Transaction> tx, PocketBlock& block)
        {
            return make_tuple(true, SocialConsensusResult_Success);
            // TODO (brangr): implement
            // std::string _txid = oitm["txid"].get_str();
            // std::string _address = oitm["address"].get_str();
            // std::string _post = oitm["posttxid"].get_str();
            // int _score_value = oitm["value"].get_int();
            // int64_t _time = oitm["time"].get_int64();

            // if (_score_value < 1 || _score_value > 5) {
            //     result = ANTIBOTRESULT::Failed;
            //     return false;
            // }

            // if (!CheckRegistration(oitm, _address, checkMempool, checkWithTime, height, blockVtx, result)) {
            //     return false;
            // }

            // // Check score to self
            // bool not_found = false;
            // std::string _post_address;
            // reindexer::Item postItm;
            // ContentType postType = ContentType::ContentPost;
            // if (g_pocketdb->SelectOne(reindexer::Query("Posts").Where("txid", CondEq, _post).Where("block", CondLt, height), postItm).ok()) {
            //     _post_address = postItm["address"].As<string>();
            //     postType = ContentType(postItm["type"].As<int>());

            //     // Score to self post
            //     if (_post_address == _address) {
            //         result = ANTIBOTRESULT::SelfScore;
            //         return false;
            //     }
            // } else {
            //     // Post not found
            //     not_found = true;

            //     // Maybe in current block?
            //     if (blockVtx.Exists("Posts")) {
            //         for (auto& mtx : blockVtx.Data["Posts"]) {
            //             if (mtx["txid"].get_str() == _post) {
            //                 _post_address = mtx["address"].get_str();
            //                 postType = ContentType(mtx["postType"].get_int());
            //                 not_found = false;
            //                 break;
            //             }
            //         }
            //     }

            //     if (not_found) {
            //         result = ANTIBOTRESULT::NotFound;
            //         return false;
            //     }
            // }

            // // Blocking
            // if (height >= Params().GetConsensus().score_blocking_on && height < Params().GetConsensus().score_blocking_off && g_pocketdb->Exists(Query("BlockingView").Where("address", CondEq, _post_address).Where("address_to", CondEq, _address).Where("block", CondLt, height))) {
            //     result = ANTIBOTRESULT::Blocking;
            //     return false;
            // }

            // // Check double score to post
            // reindexer::Item doubleScoreItm;
            // if (g_pocketdb->SelectOne(
            //                 reindexer::Query("Scores")
            //                     .Where("address", CondEq, _address)
            //                     .Where("posttxid", CondEq, _post)
            //                     .Where("block", CondLt, height),
            //                 doubleScoreItm)
            //         .ok()) {
            //     result = ANTIBOTRESULT::DoubleScore;
            //     return false;
            // }

            // // Check limit scores
            // reindexer::QueryResults scoresRes;
            // if (!g_pocketdb->DB()->Select(
            //                         reindexer::Query("Scores")
            //                             .Where("address", CondEq, _address)
            //                             .Where("time", CondGe, _time - 86400)
            //                             .Where("block", CondLt, height),
            //                         scoresRes)
            //         .ok()) {
            //     result = ANTIBOTRESULT::Failed;
            //     return false;
            // }

            // int scoresCount = scoresRes.Count();

            // // Also check mempool
            // if (checkMempool) {
            //     reindexer::QueryResults res;
            //     if (g_pocketdb->Select(reindexer::Query("Mempool").Where("table", CondEq, "Scores").Not().Where("txid", CondEq, _txid), res).ok()) {
            //         for (auto& m : res) {
            //             reindexer::Item mItm = m.GetItem();
            //             std::string t_src = DecodeBase64(mItm["data"].As<string>());

            //             reindexer::Item t_itm = g_pocketdb->DB()->NewItem("Scores");
            //             if (t_itm.FromJSON(t_src).ok()) {
            //                 if (t_itm["address"].As<string>() == _address) {
            //                     if (!checkWithTime || t_itm["time"].As<int64_t>() <= _time)
            //                         scoresCount += 1;

            //                     if (t_itm["posttxid"].As<string>() == _post) {
            //                         result = ANTIBOTRESULT::DoubleScore;
            //                         return false;
            //                     }
            //                 }
            //             }
            //         }
            //     }
            // }

            // // Check block
            // if (blockVtx.Exists("Scores")) {
            //     for (auto& mtx : blockVtx.Data["Scores"]) {
            //         if (mtx["txid"].get_str() != _txid && mtx["address"].get_str() == _address) {
            //             if (!checkWithTime || mtx["time"].get_int64() <= _time)
            //                 scoresCount += 1;

            //             if (mtx["posttxid"].get_str() == _post) {
            //                 result = ANTIBOTRESULT::DoubleScore;
            //                 return false;
            //             }
            //         }
            //     }
            // }

            // ABMODE mode;
            // getMode(_address, mode, height);
            // int limit = getLimit(Score, mode, height);
            // if (scoresCount >= limit) {
            //     result = ANTIBOTRESULT::ScoreLimit;
            //     return false;
            // }

            // // Check OP_RETURN
            // std::vector<std::string> vasm;
            // boost::split(vasm, oitm["asm"].get_str(), boost::is_any_of("\t "));

            // // Check address and value in asm == reindexer data
            // if (vasm.size() >= 4) {
            //     std::stringstream _op_return_data;
            //     _op_return_data << vasm[3];
            //     std::string _op_return_hex = _op_return_data.str();

            //     std::string _score_itm_val = _post_address + " " + std::to_string(_score_value);
            //     std::string _score_itm_hex = HexStr(_score_itm_val.begin(), _score_itm_val.end());

            //     if (_op_return_hex != _score_itm_hex) {
            //         result = ANTIBOTRESULT::OpReturnFailed;
            //         return false;
            //     }
            // }

            // return true;
        }

    };

    /*******************************************************************************************************************
    *
    *  Consensus checkpoint at 1 block
    *
    *******************************************************************************************************************/
    class ScorePostConsensus_checkpoint_1 : public ScorePostConsensus
    {
    protected:
        int CheckpointHeight() override { return 1; }
    public:
        ScorePostConsensus_checkpoint_1(int height) : ScorePostConsensus(height) {}
    };


    /*******************************************************************************************************************
    *
    *  Factory for select actual rules version
    *  Каждая новая перегрузка добавляет новый функционал, поддерживающийся с некоторым условием - например высота
    *
    *******************************************************************************************************************/
    class ScorePostConsensusFactory
    {
    private:
        static inline const std::map<int, std::function<ScorePostConsensus*(int height)>> m_rules =
        {
            {1, [](int height) { return new ScorePostConsensus_checkpoint_1(height); }},
            {0, [](int height) { return new ScorePostConsensus(height); }},
        };
    public:
        shared_ptr <ScorePostConsensus> Instance(int height)
        {
            return shared_ptr<ScorePostConsensus>(
                (--m_rules.upper_bound(height))->second(height)
            );
        }
    };
}

#endif // POCKETCONSENSUS_SCOREPOST_HPP