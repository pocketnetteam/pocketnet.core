// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include "pocketdb/web/WebSocketRpc.h"
#include "rpc/util.h"

namespace PocketWeb::PocketWebRpc
{
    RPCHelpMan GetMissedInfo()
    {
        return RPCHelpMan{
                "getmissedinfo",
                "\nGet missed info.\n",
                {
                    {"address", RPCArg::Type::STR, RPCArg::Optional::NO, ""},
                    {"block_number", RPCArg::Type::NUM, RPCArg::Optional::NO, ""}
                },
                {
                    // TODO (team)
                },
                RPCExamples{
                    // TODO (team)
                    ""
                },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
    {
        RPCTypeCheck(request.params, {UniValue::VSTR, UniValue::VNUM});

        // Get address from arguments
        RPCTypeCheckArgument(request.params[0], UniValue::VSTR);
        CTxDestination dest = DecodeDestination(request.params[0].get_str());
        if (!IsValidDestination(dest))
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY,
                std::string("Invalid address: ") + request.params[0].get_str());
        string address = request.params[0].get_str();

        // Get initial block number
        int blockNumber = request.params[1].get_int();
        if (ChainActive().Height() - blockNumber > 10000)
            blockNumber = ChainActive().Height() - 10000;

        // Get count of result records
        int cntResult = 30;
        if (!request.params[2].isNull() && request.params[2].isNum())
            cntResult = request.params[2].get_int();

        // Build result array
        UniValue result(UniValue::VARR);

        // ---------------------------------------------------------------------

        // Language statistic
        auto[contentCount, contentLangCount] = request.DbConnection()->WebRpcRepoInst->GetContentLanguages(blockNumber);
        UniValue fullStat(UniValue::VOBJ);
        fullStat.pushKV("block", ::ChainActive().Height());
        fullStat.pushKV("cntposts", contentCount);
        fullStat.pushKV("contentsLang", contentLangCount);
        result.push_back(fullStat);

        // ---------------------------------------------------------------------

        // Pocketnet Team content
        std::string teamAddress = (Params().NetworkIDString() == CBaseChainParams::MAIN) ? "PEj7QNjKdDPqE9kMDRboKoCtp8V6vZeZPd" : "TAqR1ncH95eq9XKSDRR18DtpXqktxh74UU";
        auto[teamCount, teamData] = request.DbConnection()->WebRpcRepoInst->GetLastAddressContent(teamAddress, blockNumber, 99);
        for (size_t i = 0; i < teamData.size(); i++)
        {
            teamData.At(i).pushKV("msg", "sharepocketnet");
            teamData.At(i).pushKV("addrFrom", teamAddress);
            result.push_back(teamData[i]);
        }

        // ---------------------------------------------------------------------

        // Private subscribers news
        auto privateSubscribes = request.DbConnection()->WebRpcRepoInst->GetSubscribesAddresses({ address }, { ACTION_SUBSCRIBE_PRIVATE });
        UniValue subs = privateSubscribes[address];
        for (size_t i = 0; i < subs.size(); i++)
        {
            UniValue sub = subs[i];
            string subAddress = sub["adddress"].get_str();
            if (auto[subCount, subData] = request.DbConnection()->WebRpcRepoInst->GetLastAddressContent(subAddress, blockNumber, 1); subCount > 0)
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
        result.push_backV(request.DbConnection()->WebRpcRepoInst->GetMissedRelayedContent(address, blockNumber));

        // ---------------------------------------------------------------------

        // Scores to contents
        result.push_backV(
            request.DbConnection()->WebRpcRepoInst->GetMissedContentsScores(address, blockNumber, cntResult));

        // ---------------------------------------------------------------------

        // Scores to comments
        result.push_backV(request.DbConnection()->WebRpcRepoInst->GetMissedCommentsScores(address, blockNumber, cntResult));

        // ---------------------------------------------------------------------

        // New incoming missed transactions
        auto missedTransactions = request.DbConnection()->WebRpcRepoInst->GetMissedTransactions(address, blockNumber, cntResult);

        // Restore transactions details
        vector<string> txIds;
        for (const auto& tx: missedTransactions)
            txIds.push_back(tx.first);
        auto transactionsInfo = request.DbConnection()->ExplorerRepoInst->GetTransactions(txIds, 1, (int) txIds.size());

        // Extend missed transactions
        for (size_t i = 0; i < transactionsInfo.size(); i++)
        {
            auto txInfo = transactionsInfo.At(i);
            missedTransactions[txInfo["txid"].get_str()].pushKV("txinfo", txInfo);
            result.push_back(transactionsInfo[i]);
        }

        // ---------------------------------------------------------------------

        // Comments answers
        auto answers = request.DbConnection()->WebRpcRepoInst->GetMissedCommentAnswers(address, blockNumber, cntResult);
        vector<string> answersPosts;
        for (const auto& answer: answers)
            answersPosts.push_back(answer["posttxid"].get_str());
        result.push_backV(answers);

        // ---------------------------------------------------------------------

        // New comments to posts
        result.push_backV(request.DbConnection()->WebRpcRepoInst->GetMissedPostComments(address, answersPosts, blockNumber, cntResult));

        // ---------------------------------------------------------------------

        return result;
    },
        };
    }
}