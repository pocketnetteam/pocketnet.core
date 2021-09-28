// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include "pocketdb/web/WebSocketRpc.h"

namespace PocketWeb::PocketWebRpc
{
    UniValue GetMissedInfo(const JSONRPCRequest& request)
    {
        if (request.fHelp)
            throw std::runtime_error(
                "getmissedinfo \"address\" block_number\n"
                "\nGet missed info.\n");

        RPCTypeCheck(request.params, {UniValue::VSTR, UniValue::VNUM});

        // Get address from arguments
        RPCTypeCheckArgument(request.params[0], UniValue::VSTR);
        CTxDestination dest = DecodeDestination(request.params[0].get_str());
        if (!IsValidDestination(dest))
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, std::string("Invalid address: ") + request.params[0].get_str());
        string address = request.params[0].get_str();

        // Get initial block number
        int blockNumber = request.params[1].get_int();

        // Get count of result records
        int cntResult = 30;
        if (!request.params[2].isNull() && request.params[2].isNum())
            cntResult = request.params[2].get_int();

        // Build result array
        UniValue result(UniValue::VARR);

        auto[contentCount, contentLangCount] = request.DbConnection()->WebRepoInst->GetContentLanguages(blockNumber);

        // Full statistic
        UniValue fullStat(UniValue::VOBJ);
        fullStat.pushKV("block", chainActive.Height());
        fullStat.pushKV("cntposts", contentCount);
        fullStat.pushKV("contentsLang", contentLangCount);
        result.push_back(fullStat);

        // Pocketnet Team content
        std::string teamAddress = "PEj7QNjKdDPqE9kMDRboKoCtp8V6vZeZPd";
        auto[teamCount, teamData] = request.DbConnection()->WebRepoInst->GetLastAddressContent(teamAddress, blockNumber, 99);
        for (const string& key : teamData.getKeys())
            teamData.At(key).pushKV("msg", "sharepocketnet");
        result.push_back(teamData);

        // Private subscribers news
        auto privateSubscribes = request.DbConnection()->WebRepoInst->GetSubscribesAddresses({ address }, { ACTION_SUBSCRIBE_PRIVATE });
        UniValue subs = privateSubscribes[address];
        for (size_t i = 0; i < subs.size(); i++)
        {
            UniValue sub = subs[i];
            string subAddress = sub["adddress"].get_str();
            if (auto[subCount, subData] = request.DbConnection()->WebRepoInst->GetLastAddressContent(subAddress, blockNumber, 1); subCount > 0)
            {
                subData.At(0).pushKV("msg", "event");
                subData.At(0).pushKV("mesType", "postfromprivate");
                subData.At(0).pushKV("addrFrom", subAddress);
                subData.At(0).pushKV("postsCnt", subCount);
                result.push_back(subData.At(0));
            }
        }

        // TODO (brangr): implement
        // reindexer::QueryResults reposts;
        // g_pocketdb->DB()->Select(reindexer::Query("Posts").Where("block", CondGt, blockNumber).InnerJoin("txidRepost", "txid", CondEq,
        //     reindexer::Query("Posts").Where("address", CondEq, address)), reposts);
        // for (auto it : reposts)
        // {
        //     reindexer::Item itm(it.GetItem());
        //     UniValue msg(UniValue::VOBJ);
        //     msg.pushKV("msg", "reshare");
        //     msg.pushKV("txid", itm["txid"].As<string>());
        //     msg.pushKV("txidRepost", itm["txidRepost"].As<string>());
        //     msg.pushKV("addrFrom", itm["address"].As<string>());
        //     msg.pushKV("time", itm["time"].As<string>());
        //     msg.pushKV("nblock", itm["block"].As<int>());
        //
        //     a.push_back(msg);
        // }

        // TODO (brangr): implement
        // reindexer::QueryResults subscribers;
        // g_pocketdb->DB()->Select(
        //     reindexer::Query("SubscribesView").Where("address_to", CondEq, address).Where("block", CondGt, blockNumber).Sort("time", true).Limit(
        //         cntResult), subscribers);
        // for (auto it : subscribers)
        // {
        //     reindexer::Item itm(it.GetItem());
        //
        //     reindexer::QueryResults resubscribe;
        //     reindexer::Error errResubscr = g_pocketdb->DB()->Select(
        //         reindexer::Query("Subscribes", 0, 1).Where("address", CondEq, itm["address"].As<string>()).Where("address_to", CondEq,
        //             itm["address_to"].As<string>()).Where("time", CondLt, itm["time"].As<int64_t>()).Sort("time", true), resubscribe);
        //     if (errResubscr.ok() && resubscribe.Count() > 0)
        //     {
        //         reindexer::Item reitm(resubscribe[0].GetItem());
        //         if (reitm["unsubscribe"].As<bool>())
        //         {
        //             UniValue msg(UniValue::VOBJ);
        //             msg.pushKV("addr", itm["address_to"].As<string>());
        //             msg.pushKV("addrFrom", itm["address"].As<string>());
        //             msg.pushKV("msg", "event");
        //             msg.pushKV("txid", itm["txid"].As<string>());
        //             msg.pushKV("time", itm["time"].As<string>());
        //             msg.pushKV("mesType", "subscribe");
        //             msg.pushKV("nblock", itm["block"].As<int>());
        //
        //             a.push_back(msg);
        //         }
        //     }
        // }

        // TODO (brangr): implement
        // reindexer::QueryResults scores;
        // g_pocketdb->DB()->Select(reindexer::Query("Scores").Where("block", CondGt, blockNumber).InnerJoin("posttxid", "txid", CondEq,
        //     reindexer::Query("Posts").Where("address", CondEq, address)).Sort("time", true).Limit(cntResult), scores);
        // for (auto it : scores)
        // {
        //     reindexer::Item itm(it.GetItem());
        //     UniValue msg(UniValue::VOBJ);
        //     msg.pushKV("addr", address);
        //     msg.pushKV("addrFrom", itm["address"].As<string>());
        //     msg.pushKV("msg", "event");
        //     msg.pushKV("txid", itm["txid"].As<string>());
        //     msg.pushKV("time", itm["time"].As<string>());
        //     msg.pushKV("posttxid", itm["posttxid"].As<string>());
        //     msg.pushKV("upvoteVal", itm["value"].As<int>());
        //     msg.pushKV("mesType", "upvoteShare");
        //     msg.pushKV("nblock", itm["block"].As<int>());
        //     a.push_back(msg);
        // }

        // TODO (brangr): implement
        // reindexer::QueryResults commentScores;
        // g_pocketdb->DB()->Select(
        //     reindexer::Query("CommentScores")
        //         .Where("block", CondGt, blockNumber)
        //         .InnerJoin("commentid", "txid", CondEq, reindexer::Query("Comment").Where("address", CondEq, address))
        //         .Sort("time", true)
        //         .Limit(cntResult),
        //     commentScores);
        // for (auto it : commentScores)
        // {
        //     reindexer::Item itm(it.GetItem());
        //     UniValue msg(UniValue::VOBJ);
        //     msg.pushKV("addr", address);
        //     msg.pushKV("addrFrom", itm["address"].As<string>());
        //     msg.pushKV("msg", "event");
        //     msg.pushKV("txid", itm["txid"].As<string>());
        //     msg.pushKV("time", itm["time"].As<string>());
        //     msg.pushKV("commentid", itm["commentid"].As<string>());
        //     msg.pushKV("upvoteVal", itm["value"].As<int>());
        //     msg.pushKV("mesType", "cScore");
        //     msg.pushKV("nblock", itm["block"].As<int>());
        //     a.push_back(msg);
        // }

        // TODO (brangr): implement
        // std::vector<std::string> txSent;
        // reindexer::QueryResults transactions;
        // g_pocketdb->DB()->Select(
        //     reindexer::Query("UTXO").Where("address", CondEq, address).Where("block", CondGt, blockNumber).Sort("time", true).Limit(cntResult),
        //     transactions);
        // for (auto it : transactions)
        // {
        //     reindexer::Item itm(it.GetItem());
        //
        //     // Double transaction notify not allowed
        //     if (std::find(txSent.begin(), txSent.end(), itm["txid"].As<string>()) != txSent.end()) continue;
        //
        //     UniValue msg(UniValue::VOBJ);
        //     msg.pushKV("addr", itm["address"].As<string>());
        //     msg.pushKV("msg", "transaction");
        //     msg.pushKV("txid", itm["txid"].As<string>());
        //     msg.pushKV("time", itm["time"].As<string>());
        //     msg.pushKV("amount", itm["amount"].As<int64_t>());
        //     msg.pushKV("nblock", itm["block"].As<int>());
        //
        //     uint256 hash = ParseHashV(itm["txid"].As<string>(), "txid");
        //     CTransactionRef tx;
        //     uint256 hash_block;
        //     CBlockIndex* blockindex = nullptr;
        //     if (GetTransaction(hash, tx, Params().GetConsensus(), hash_block, true, blockindex))
        //     {
        //         const CTxOut& txout = tx->vout[itm["txout"].As<int>()];
        //         std::string optype = "";
        //         if (txout.scriptPubKey[0] == OP_RETURN)
        //         {
        //             std::string asmstr = ScriptToAsmStr(txout.scriptPubKey);
        //             std::vector<std::string> spl;
        //             boost::split(spl, asmstr, boost::is_any_of("\t "));
        //             if (spl.size() == 3)
        //             {
        //                 if (spl[1] == OR_POST || spl[1] == OR_POSTEDIT)
        //                     optype = "share";
        //                 else if (spl[1] == OR_VIDEO)
        //                     optype = "video";
        //                 else if (spl[1] == OR_SCORE)
        //                     optype = "upvoteShare";
        //                 else if (spl[1] == OR_SUBSCRIBE)
        //                     optype = "subscribe";
        //                 else if (spl[1] == OR_SUBSCRIBEPRIVATE)
        //                     optype = "subscribePrivate";
        //                 else if (spl[1] == OR_USERINFO)
        //                     optype = "userInfo";
        //                 else if (spl[1] == OR_UNSUBSCRIBE)
        //                     optype = "unsubscribe";
        //                 else if (spl[1] == OR_COMMENT)
        //                     optype = "comment";
        //                 else if (spl[1] == OR_COMMENT_EDIT)
        //                     optype = "commentEdit";
        //                 else if (spl[1] == OR_COMMENT_DELETE)
        //                     optype = "commentDelete";
        //                 else if (spl[1] == OR_COMMENT_SCORE)
        //                     optype = "cScore";
        //             }
        //         }
        //
        //         if (optype != "") msg.pushKV("type", optype);
        //
        //         UniValue txinfo(UniValue::VOBJ);
        //         TxToJSON(*tx, hash_block, txinfo);
        //         msg.pushKV("txinfo", txinfo);
        //     }
        //
        //     a.push_back(msg);
        // }

        // TODO (brangr): implement
        // vector<string> answerpostids;
        // reindexer::QueryResults commentsAnswer;
        // g_pocketdb->DB()->Select(
        //     reindexer::Query("Comment")
        //         .Where("block", CondGt, blockNumber)
        //         .Where("last", CondEq, true)
        //         .InnerJoin("answerid", "otxid", CondEq, reindexer::Query("Comment").Where("address", CondEq, address).Where("last", CondEq, true))
        //         .Sort("time", true)
        //         .Limit(cntResult),
        //     commentsAnswer);
        //
        // for (auto it : commentsAnswer)
        // {
        //     reindexer::Item itm(it.GetItem());
        //     if (address != itm["address"].As<string>())
        //     {
        //         if (itm["msg"].As<string>() == "") continue;
        //         if (itm["otxid"].As<string>() != itm["txid"].As<string>()) continue;
        //
        //         UniValue msg(UniValue::VOBJ);
        //         msg.pushKV("addr", address);
        //         msg.pushKV("addrFrom", itm["address"].As<string>());
        //         msg.pushKV("nblock", itm["block"].As<int>());
        //         msg.pushKV("msg", "comment");
        //         msg.pushKV("mesType", "answer");
        //         msg.pushKV("txid", itm["otxid"].As<string>());
        //         msg.pushKV("posttxid", itm["postid"].As<string>());
        //         msg.pushKV("reason", "answer");
        //         msg.pushKV("time", itm["time"].As<string>());
        //         if (itm["parentid"].As<string>() != "") msg.pushKV("parentid", itm["parentid"].As<string>());
        //         if (itm["answerid"].As<string>() != "") msg.pushKV("answerid", itm["answerid"].As<string>());
        //
        //         a.push_back(msg);
        //
        //         answerpostids.push_back(itm["postid"].As<string>());
        //     }
        // }

        // TODO (brangr): implement
        // reindexer::QueryResults commentsPost;
        // g_pocketdb->DB()->Select(
        //     reindexer::Query("Comment").Where("block", CondGt, blockNumber).Where("last", CondEq, true).InnerJoin("postid", "txid", CondEq,
        //         reindexer::Query("Posts").Where("address", CondEq, address).Not().Where("txid", CondSet, answerpostids)).Sort("time", true).Limit(
        //         cntResult), commentsPost);
        //
        // for (auto it : commentsPost)
        // {
        //     reindexer::Item itm(it.GetItem());
        //     if (address != itm["address"].As<string>())
        //     {
        //         if (itm["msg"].As<string>() == "") continue;
        //         if (itm["otxid"].As<string>() != itm["txid"].As<string>()) continue;
        //
        //         UniValue msg(UniValue::VOBJ);
        //         msg.pushKV("addr", address);
        //         msg.pushKV("addrFrom", itm["address"].As<string>());
        //         msg.pushKV("nblock", itm["block"].As<int>());
        //         msg.pushKV("msg", "comment");
        //         msg.pushKV("mesType", "post");
        //         msg.pushKV("txid", itm["otxid"].As<string>());
        //         msg.pushKV("posttxid", itm["postid"].As<string>());
        //         msg.pushKV("reason", "post");
        //         msg.pushKV("time", itm["time"].As<string>());
        //         if (itm["parentid"].As<string>() != "") msg.pushKV("parentid", itm["parentid"].As<string>());
        //         if (itm["answerid"].As<string>() != "") msg.pushKV("answerid", itm["answerid"].As<string>());
        //
        //         a.push_back(msg);
        //     }
        // }

        // TODO (brangr): ???
        // reindexer::QueryResults likedPosts;
        // g_pocketdb->DB()->Select(reindexer::Query("Post"), likedPosts);

        return result;
    }
}