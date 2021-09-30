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

        // ---------------------------------------------------------------------

        // Language statistic
        auto[contentCount, contentLangCount] = request.DbConnection()->WebRepoInst->GetContentLanguages(blockNumber);
        UniValue fullStat(UniValue::VOBJ);
        fullStat.pushKV("block", chainActive.Height());
        fullStat.pushKV("cntposts", contentCount);
        fullStat.pushKV("contentsLang", contentLangCount);
        result.push_back(fullStat);

        // ---------------------------------------------------------------------

        // Pocketnet Team content
        std::string teamAddress = "PEj7QNjKdDPqE9kMDRboKoCtp8V6vZeZPd";
        auto[teamCount, teamData] = request.DbConnection()->WebRepoInst->GetLastAddressContent(teamAddress, blockNumber, 99);
        for (size_t i = 0; i < teamData.size(); i++)
        {
            teamData.At(i).pushKV("msg", "sharepocketnet");
            result.push_back(teamData[i]);
        }

        // ---------------------------------------------------------------------

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
                result.push_back(subData[0]);
            }
        }

        // ---------------------------------------------------------------------

        // Reposts
        result.push_backV(request.DbConnection()->WebRepoInst->GetMissedRelayedContent(address, blockNumber));

        // ---------------------------------------------------------------------

        // TODO (brangr): Oo
        // Get new subscribers
        // reindexer::QueryResults subscribers;
        // g_pocketdb->DB()->Select(
        //     reindexer::Query("SubscribesView").Where("address_to", CondEq, address)
        //     .Where("block", CondGt, blockNumber).Sort("time", true).Limit(
        //         cntResult), subscribers);
        //
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

        // ---------------------------------------------------------------------

        // Scores to contents
        result.push_backV(request.DbConnection()->WebRepoInst->GetMissedContentsScores(address, blockNumber, cntResult));

        // ---------------------------------------------------------------------

        // Scores to comments
        result.push_backV(request.DbConnection()->WebRepoInst->GetMissedCommentsScores(address, blockNumber, cntResult));

        // ---------------------------------------------------------------------

        // New incoming transactions
        result.push_backV(request.DbConnection()->WebRepoInst->GetMissedTransactions(address, blockNumber, cntResult));

        // ---------------------------------------------------------------------

        // Comments answers
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

        // ---------------------------------------------------------------------

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