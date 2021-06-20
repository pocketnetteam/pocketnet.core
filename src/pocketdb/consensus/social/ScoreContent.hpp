// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 Bitcoin developers
// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETCONSENSUS_SCORECONTENT_HPP
#define POCKETCONSENSUS_SCORECONTENT_HPP

#include "pocketdb/consensus/social/Base.hpp"
#include "pocketdb/models/base/Transaction.hpp"
#include "pocketdb/models/dto/ScoreContent.hpp"

namespace PocketConsensus
{
    /*******************************************************************************************************************
    *
    *  ScorePost consensus base class
    *
    *******************************************************************************************************************/
    class ScoreContentConsensus : public SocialBaseConsensus
    {
    public:
        ScoreContentConsensus(int height) : SocialBaseConsensus(height) {}

    protected:
        virtual int64_t GetFullAccountScoresLimit() { return 90; }
        virtual int64_t GetTrialAccountScoresLimit() { return 45; }

        virtual int64_t GetScoresLimit(AccountMode mode)
        {
            return mode == AccountMode_Full ? GetFullAccountScoresLimit() : GetTrialAccountScoresLimit();
        }

        tuple<bool, SocialConsensusResult> ValidateModel(shared_ptr <Transaction> tx) override
        {
            return make_tuple(true, SocialConsensusResult_Success);
            // TODO (brangr): implement
            // std::string _txid = oitm["txid"].get_str();
            // std::string _address = oitm["address"].get_str();
            // std::string _post = oitm["posttxid"].get_str();
            // int _score_value = oitm["value"].get_int();
            // int64_t _time = oitm["time"].get_int64();

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

        tuple<bool, SocialConsensusResult> ValidateLimit(shared_ptr <Transaction> tx, const PocketBlock& block) override
        {
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



            // return ValidateLimit(tx, scoresCount);
        }

        tuple<bool, SocialConsensusResult> ValidateLimit(shared_ptr <Transaction> tx) override
        {
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


            // return ValidateLimit(tx, scoresCount);
        }

        virtual tuple<bool, SocialConsensusResult> ValidateLimit(shared_ptr <ScoreContent> tx, int count)
        {
            auto reputationConsensus = ReputationConsensusFactory::Instance(Height);
            auto accountMode = reputationConsensus->GetAccountMode(*tx->GetAddress());
            auto limit = GetScoresLimit(accountMode);

            if (count >= limit)
                return {false, SocialConsensusResult_ScoreLimit};

            return Success;
        }

        tuple<bool, SocialConsensusResult> CheckModel(shared_ptr <Transaction> tx) override
        {
            auto ptx = static_pointer_cast<ScoreContent>(tx);

            // Check required fields
            if (IsEmpty(ptx->GetAddress())) return {false, SocialConsensusResult_Failed};
            if (IsEmpty(ptx->GetContentTxHash())) return {false, SocialConsensusResult_Failed};
            if (IsEmpty(ptx->GetValue())) return {false, SocialConsensusResult_Failed};

            auto value = *ptx->GetValue();
            if (value < 1 || value > 5)
                return {false, SocialConsensusResult_Failed};

            return Success;
        }

    };

    /*******************************************************************************************************************
    *
    *  Consensus checkpoint at 175600 block
    *
    *******************************************************************************************************************/
    class ScoreContentConsensus_checkpoint_175600 : public ScoreContentConsensus
    {
    protected:
        int CheckpointHeight() override { return 175600; }
        int64_t GetFullAccountScoresLimit() override { return 200; }
        int64_t GetTrialAccountScoresLimit() override { return 100; }

    public:
        ScoreContentConsensus_checkpoint_175600(int height) : ScoreContentConsensus(height) {}
    };


    /*******************************************************************************************************************
    *
    *  Factory for select actual rules version
    *
    *******************************************************************************************************************/
    class ScoreContentConsensusFactory
    {
    private:
        static inline const std::map<int, std::function<ScoreContentConsensus*(int height)>> m_rules =
            {
                {175600, [](int height) { return new ScoreContentConsensus_checkpoint_175600(height); }},
                {0,      [](int height) { return new ScoreContentConsensus(height); }},
            };
    public:
        shared_ptr <ScoreContentConsensus> Instance(int height)
        {
            return shared_ptr<ScoreContentConsensus>(
                (--m_rules.upper_bound(height))->second(height)
            );
        }
    };
}

#endif // POCKETCONSENSUS_SCORECONTENT_HPP