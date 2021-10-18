// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include "pocketdb/web/PocketContentRpc.h"

namespace PocketWeb::PocketWebRpc
{
    void ParseRequestContentType(const UniValue& value, vector<int>& types)
    {
        if (value.isNum())
        {
            types.push_back(value.get_int());
        }
        else if (value.isStr())
        {
            if (TransactionHelper::TxIntType(value.get_str()) != TxType::NOT_SUPPORTED)
                types.push_back(TransactionHelper::TxIntType(value.get_str()));
        }
        else if (value.isArray())
        {
            UniValue cntntTps = value.get_array();
            for (unsigned int idx = 0; idx < cntntTps.size(); idx++)
                ParseRequestContentType(cntntTps[idx], types);
        }
    }

    UniValue GetContents(const JSONRPCRequest& request)
    {
        if (request.fHelp || request.params.size() < 1)
        {
            throw runtime_error(
                "getcontents address\n"
                "\nReturns contents for address.\n"
                "\nArguments:\n"
                "1. address            (string) A pocketcoin addresses to filter\n"
                "\nResult\n"
                "[                     (array of contents)\n"
                "  ...\n"
                "]");
        }

        string address;
        if (request.params[0].isStr())
            address = request.params[0].get_str();

        return request.DbConnection()->WebRpcRepoInst->GetContentsForAddress(address);
    }

    UniValue GetHotPosts(const JSONRPCRequest& request)
    {
        if (request.fHelp)
            throw runtime_error(
                "GetHotPosts\n");

        int count = 30;
        if (request.params.size() > 0)
        {
            if (request.params[0].isNum())
                count = request.params[0].get_int();
            else if (request.params[0].isStr())
                ParseInt32(request.params[0].get_str(), &count);
        }

        // Depth in blocks (default about 3 days)
        int dayInBlocks = 24 * 60;
        int depthBlocks = 3 * dayInBlocks;
        if (request.params.size() > 1)
        {
            if (request.params[1].isNum())
                depthBlocks = request.params[1].get_int();
            else if (request.params[1].isStr())
                ParseInt32(request.params[1].get_str(), &depthBlocks);

            if (depthBlocks == 259200)
            { // for old version electron
                depthBlocks = 3 * dayInBlocks;
            }

            depthBlocks = min(depthBlocks, 365 * dayInBlocks);
        }

        int nHeightOffset = chainActive.Height();
        int nOffset = 0;
        if (request.params.size() > 2)
        {
            if (request.params[2].isNum())
            {
                if (request.params[2].get_int() > 0)
                    nOffset = request.params[2].get_int();
            }
            else if (request.params[2].isStr())
            {
                ParseInt32(request.params[2].get_str(), &nOffset);
            }
            nHeightOffset -= nOffset;
        }

        string lang = "";
        if (request.params.size() > 3)
            lang = request.params[3].get_str();

        vector<int> contentTypes{CONTENT_POST, CONTENT_VIDEO};
        if (request.params.size() > 4)
        {
            contentTypes.clear();
            ParseRequestContentType(request.params[4], contentTypes);
        }

        string address = "";
        if (request.params.size() > 5)
            address = request.params[5].get_str();

        return request.DbConnection()->WebRpcRepoInst->GetHotPosts(count, depthBlocks, nHeightOffset, lang, contentTypes, address);
    }

    UniValue GetHistoricalStrip(const JSONRPCRequest& request)
    {
        if (request.fHelp)
            throw runtime_error(
                "GetHistoricalStrip\n"
                "\n.\n");

        int nHeight = chainActive.Height();
        if (!request.params.empty())
        {
            if (request.params[0].isNum())
                if (request.params[0].get_int() > 0)
                    nHeight = request.params[0].get_int();
        }

        string start_txid;
        if (request.params.size() > 1)
        {
            start_txid = request.params[1].get_str();
        }

        int countOut = 10;
        if (request.params.size() > 2)
            if (request.params[2].isNum())
                countOut = request.params[2].get_int();

        string lang;
        if (request.params.size() > 3)
            lang = request.params[3].get_str();

        vector<string> tags;
        if (request.params.size() > 4)
        {
            if (request.params[4].isStr())
            {
                string tag = boost::trim_copy(request.params[4].get_str());
                if (!tag.empty())
                    tags.push_back(tag);
            }
            else if (request.params[4].isArray())
            {
                UniValue tgs = request.params[4].get_array();
                if (tgs.size() > 1000)
                    throw JSONRPCError(RPC_INVALID_PARAMS, "Too large array tags");

                if (!tgs.empty())
                {
                    for (unsigned int idx = 0; idx < tgs.size(); idx++)
                    {
                        string tag = boost::trim_copy(tgs[idx].get_str());
                        if (!tag.empty())
                            tags.push_back(tag);
                    }
                }
            }
        }

        vector<int> contentTypes;
        if (request.params.size() > 5)
        {
            if (request.params[5].isNum())
            {
                contentTypes.push_back(request.params[5].get_int());
            }
            else if (request.params[5].isStr())
            {
                if (TransactionHelper::TxIntType(request.params[5].get_str()) != TxType::NOT_SUPPORTED)
                    contentTypes.push_back(TransactionHelper::TxIntType(request.params[5].get_str()));
            }
            else if (request.params[5].isArray())
            {
                UniValue cntntTps = request.params[5].get_array();
                if (cntntTps.size() > 10)
                    throw JSONRPCError(RPC_INVALID_PARAMS, "Too large array content types");

                if (!cntntTps.empty())
                {
                    for (unsigned int idx = 0; idx < cntntTps.size(); idx++)
                    {
                        if (cntntTps[idx].isNum())
                        {
                            contentTypes.push_back(cntntTps[idx].get_int());
                        }
                        else if (cntntTps[idx].isStr())
                        {
                            if (TransactionHelper::TxIntType(cntntTps[idx].get_str()) != TxType::NOT_SUPPORTED)
                                contentTypes.push_back(TransactionHelper::TxIntType(cntntTps[idx].get_str()));
                        }
                    }
                }
            }
        }

        vector<string> txidsExcluded;
        if (request.params.size() > 6)
        {
            if (request.params[6].isStr())
            {
                txidsExcluded.push_back(request.params[6].get_str());
            }
            else if (request.params[6].isArray())
            {
                UniValue txids = request.params[6].get_array();
                if (txids.size() > 1000)
                    throw JSONRPCError(RPC_INVALID_PARAMS, "Too large array txids");

                if (!txids.empty())
                {
                    for (unsigned int idx = 0; idx < txids.size(); idx++)
                    {
                        string txidEx = boost::trim_copy(txids[idx].get_str());
                        if (!txidEx.empty())
                            txidsExcluded.push_back(txidEx);
                    }
                }
            }
        }

        vector<string> adrsExcluded;
        if (request.params.size() > 7)
        {
            if (request.params[7].isStr())
            {
                adrsExcluded.push_back(request.params[7].get_str());
            }
            else if (request.params[7].isArray())
            {
                UniValue adrs = request.params[7].get_array();
                if (adrs.size() > 1000)
                {
                    throw JSONRPCError(RPC_INVALID_PARAMS, "Too large array addresses");
                }
                if (!adrs.empty())
                {
                    for (unsigned int idx = 0; idx < adrs.size(); idx++)
                    {
                        string adrEx = boost::trim_copy(adrs[idx].get_str());
                        if (!adrEx.empty())
                        {
                            adrsExcluded.push_back(adrEx);
                        }
                    }
                }
            }
        }

        vector<string> tagsExcluded;
        if (request.params.size() > 8)
        {
            if (request.params[8].isStr())
            {
                tagsExcluded.push_back(request.params[8].get_str());
            }
            else if (request.params[8].isArray())
            {
                UniValue tagsEx = request.params[8].get_array();
                if (tagsEx.size() > 1000)
                {
                    throw JSONRPCError(RPC_INVALID_PARAMS, "Too large array tags");
                }
                if (!tagsEx.empty())
                {
                    for (unsigned int idx = 0; idx < tagsEx.size(); idx++)
                    {
                        string tgsEx = boost::trim_copy(tagsEx[idx].get_str());
                        if (!tgsEx.empty())
                        {
                            tagsExcluded.push_back(tgsEx);
                        }
                    }
                }
            }
        }

        string address;
        if (request.params.size() > 9)
        {
            RPCTypeCheckArgument(request.params[9], UniValue::VSTR);
            address = request.params[9].get_str();
            CTxDestination dest = DecodeDestination(address);

            if (!IsValidDestination(dest))
                throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, string("Invalid Pocketcoin address: ") + address);
        }

        UniValue oResult(UniValue::VOBJ);
        UniValue aContents(UniValue::VARR);

        // int totalcount = 0;
        map<string, UniValue> contents = request.DbConnection()->WebRpcRepoInst->GetContents(
            countOut, nHeight, 0, start_txid, lang, tags, contentTypes, txidsExcluded, adrsExcluded, tagsExcluded, address);

        for (auto& c: contents)
        {
            aContents.push_back(c.second);
        }

        oResult.pushKV("height", nHeight);
        oResult.pushKV("contents", aContents);
        // oResult.pushKV("contentsTotal", totalcount);
        return oResult;
    }

    UniValue GetHierarchicalStrip(const JSONRPCRequest& request)
    {
        if (request.fHelp)
            throw runtime_error(
                "GetHierarchicalStrip\n"
                "\n.\n");

        int nHeight = chainActive.Height();
        if (request.params.size() > 0)
        {
            if (request.params[0].isNum())
            {
                if (request.params[0].get_int() > 0)
                {
                    nHeight = request.params[0].get_int();
                }
            }
        }

        string start_txid = "";
        if (request.params.size() > 1)
        {
            start_txid = request.params[1].get_str();
        }

        int countOut = 10;
        if (request.params.size() > 2)
        {
            if (request.params[2].isNum())
            {
                countOut = request.params[2].get_int();
            }
        }

        string lang = "";
        if (request.params.size() > 3)
        {
            lang = request.params[3].get_str();
        }

        vector<string> tags;
        UniValue uvTags(UniValue::VARR);
        if (request.params.size() > 4)
        {
            if (request.params[4].isStr())
            {
                string tag = boost::trim_copy(request.params[4].get_str());
                if (!tag.empty())
                {
                    tags.push_back(tag);
                    uvTags.push_back(tag);
                }
            }
            else if (request.params[4].isArray())
            {
                UniValue tgs = request.params[4].get_array();
                if (tgs.size() > 1000)
                {
                    throw JSONRPCError(RPC_INVALID_PARAMS, "Too large array tags");
                }
                if (tgs.size() > 0)
                {
                    uvTags = tgs;
                    for (unsigned int idx = 0; idx < tgs.size(); idx++)
                    {
                        string tag = boost::trim_copy(tgs[idx].get_str());
                        if (!tag.empty())
                        {
                            tags.push_back(tag);
                        }
                    }
                }
            }
        }

        vector<int> contentTypes;
        UniValue uvContentTypes(UniValue::VARR);
        if (request.params.size() > 5)
        {
            if (request.params[5].isNum())
            {
                contentTypes.push_back(request.params[5].get_int());
                uvContentTypes.push_back(request.params[5].get_int());
            }
            else if (request.params[5].isStr())
            {
                if (TransactionHelper::TxIntType(request.params[5].get_str()) != TxType::NOT_SUPPORTED)
                    contentTypes.push_back(TransactionHelper::TxIntType(request.params[5].get_str()));
            }
            else if (request.params[5].isArray())
            {
                UniValue cntntTps = request.params[5].get_array();
                if (cntntTps.size() > 10)
                {
                    throw JSONRPCError(RPC_INVALID_PARAMS, "Too large array content types");
                }
                if (cntntTps.size() > 0)
                {
                    uvContentTypes = cntntTps;
                    for (unsigned int idx = 0; idx < cntntTps.size(); idx++)
                    {
                        if (cntntTps[idx].isNum())
                        {
                            contentTypes.push_back(cntntTps[idx].get_int());
                        }
                        else if (cntntTps[idx].isStr())
                        {
                            if (TransactionHelper::TxIntType(cntntTps[idx].get_str()) != TxType::NOT_SUPPORTED)
                                contentTypes.push_back(TransactionHelper::TxIntType(cntntTps[idx].get_str()));
                        }
                    }
                }
            }
        }

        vector<string> txidsExcluded;
        UniValue uvTxidsExcluded(UniValue::VARR);
        if (request.params.size() > 6)
        {
            if (request.params[6].isStr())
            {
                txidsExcluded.push_back(request.params[6].get_str());
                uvTxidsExcluded.push_back(request.params[6].get_str());
            }
            else if (request.params[6].isArray())
            {
                UniValue txids = request.params[6].get_array();
                if (txids.size() > 1000)
                {
                    throw JSONRPCError(RPC_INVALID_PARAMS, "Too large array txids");
                }
                if (txids.size() > 0)
                {
                    uvTxidsExcluded = txids;
                    for (unsigned int idx = 0; idx < txids.size(); idx++)
                    {
                        string txidEx = boost::trim_copy(txids[idx].get_str());
                        if (!txidEx.empty())
                        {
                            txidsExcluded.push_back(txidEx);
                        }
                    }
                }
            }
        }

        vector<string> adrsExcluded;
        UniValue uvAdrsExcluded(UniValue::VARR);
        if (request.params.size() > 7)
        {
            if (request.params[7].isStr())
            {
                adrsExcluded.push_back(request.params[7].get_str());
                uvAdrsExcluded.push_back(request.params[7].get_str());
            }
            else if (request.params[7].isArray())
            {
                UniValue adrs = request.params[7].get_array();
                if (adrs.size() > 1000)
                {
                    throw JSONRPCError(RPC_INVALID_PARAMS, "Too large array addresses");
                }
                if (adrs.size() > 0)
                {
                    uvAdrsExcluded = adrs;
                    for (unsigned int idx = 0; idx < adrs.size(); idx++)
                    {
                        string adrEx = boost::trim_copy(adrs[idx].get_str());
                        if (!adrEx.empty())
                        {
                            adrsExcluded.push_back(adrEx);
                        }
                    }
                }
            }
        }

        vector<string> tagsExcluded;
        UniValue uvTagsExcluded(UniValue::VARR);
        if (request.params.size() > 8)
        {
            if (request.params[8].isStr())
            {
                tagsExcluded.push_back(request.params[8].get_str());
                uvTagsExcluded.push_back(request.params[8].get_str());
            }
            else if (request.params[8].isArray())
            {
                UniValue tagsEx = request.params[8].get_array();
                if (tagsEx.size() > 1000)
                {
                    throw JSONRPCError(RPC_INVALID_PARAMS, "Too large array tags");
                }
                if (tagsEx.size() > 0)
                {
                    for (unsigned int idx = 0; idx < tagsEx.size(); idx++)
                    {
                        string tgsEx = boost::trim_copy(tagsEx[idx].get_str());
                        if (!tgsEx.empty())
                        {
                            tagsExcluded.push_back(tgsEx);
                            uvTagsExcluded.push_back(tgsEx);
                        }
                    }
                }
            }
        }

        // TODO (o1q): Do not show posts from users with reputation < Limit::bad_reputation

        enum PostRanks
        {
            LAST5, LAST5R, BOOST, UREP, UREPR, DREP, PREP, PREPR, DPOST, POSTRF
        };
        int cntBlocksForResult = 300;
        int cntPrevPosts = 5;
        int durationBlocksForPrevPosts = 30 * 24 * 60; // about 1 month
        double dekayRep = 0.82;//0.7;
        double dekayPost = 0.96;
        if (contentTypes.size() == 1 && contentTypes[0] == TxType::CONTENT_VIDEO)
            dekayPost = 0.99;

        vector<string> txidsOut;
        vector<string> txidsHierarchical;

        map<string, map<PostRanks, double>> postsRanks;

        // map<string, UniValue> contents = request.DbConnection()->WebRpcRepoInst->GetContents(
        //     0, nHeight, nHeight - cntBlocksForResult, "", lang, tags, contentTypes, txidsExcluded, adrsExcluded, tagsExcluded, "");

            
        //for (auto& c : contents)

        /*err = g_pocketdb->DB()->Select(query, queryResults);
        if (err.ok()) {
            for (auto it : queryResults) {
                reindexer::Item itm(it.GetItem());
                double postRep = 0.0;
                double userRep = 0.0;
                if (it.GetJoined().size() > 0 && it.GetJoined()[0].Count() > 0) {
                    postRep = it.GetJoined()[0][0].GetItem()["reputation"].As<int>();
                }
                if (it.GetJoined().size() > 1 && it.GetJoined()[1].Count() > 0) {
                    userRep = it.GetJoined()[1][0].GetItem()["reputation"].As<int>() / 10.0;
                }

                reindexer::Query queryPrevsPosts;
                reindexer::QueryResults queryPrevsPostsResult;

                int cntPositiveScores = 0;
                string posttxid = itm["txid"].As<string>();
                string postaddress = itm["address"].As<string>();
                int postblock = itm["block"].As<int>();
                int postblockOrig = postblock;
                if (it.GetJoined().size() > 2 && it.GetJoined()[2].Count() > 0) {
                    postblockOrig = it.GetJoined()[2][0].GetItem()["block"].As<int>();
                }
                uvTxidsExcluded.push_back(posttxid);

                *//*
                // DEBUGINFO
                if (postRep != 0) {
                    reindexer::QueryResults postRepQueryResult;
                    if (g_pocketdb->DB()->Select(reindexer::Query("Scores").Where("posttxid", CondEq, posttxid).Where("block", CondLe, nHeight), postRepQueryResult).ok()) {
                        for (auto itPostRep : postRepQueryResult) {
                            reindexer::Item itmPR(itPostRep.GetItem());
                            if (g_antibot->AllowModifyReputationOverPost(itmPR["address"].As<string>(), postaddress, itmPR["block"].As<int>(), itmPR["time"].As<int64_t>(), posttxid, false)) {
                                map<scoreDetail, string> postScoresDetail;           // DEBUGINFO
                                postScoresDetail[ADDRESS] = itmPR["address"].As<string>(); // DEBUGINFO
                                postScoresDetail[VALUE] = itmPR["value"].As<string>();     // DEBUGINFO
                                postScoresDetails[posttxid].push_back(postScoresDetail);       // DEBUGINFO
                            }
                        }
                    }
                }
                 *//*

                queryPrevsPosts = reindexer::Query("Posts", 0, cntPrevPosts)
                    .Where("address", CondEq, postaddress)
                    .Where("block", CondLt, postblockOrig)
                    .Where("block", CondGe, postblockOrig - durationBlocksForPrevPosts)
                    .Not().Where("type", CondEq, (int)ContentType::ContentDelete);
                err = g_pocketdb->DB()->Select(queryPrevsPosts, queryPrevsPostsResult);
                if (err.ok()) {
                    vector<string> prevPostsIds;
                    for (auto itPrevPost : queryPrevsPostsResult) {
                        reindexer::Item itmPP(itPrevPost.GetItem());
                        prevPostsIds.push_back(itmPP["txid"].As<string>());
                    }
                    vector<int> scores = {1, 5};
                    reindexer::Query queryScores;
                    reindexer::QueryResults queryScoresResult;
                    queryScores = reindexer::Query("Scores");
                    queryScores = queryScores.Where("posttxid", CondSet, prevPostsIds);
                    queryScores = queryScores.Where("block", CondLe, postblock);
                    queryScores = queryScores.Where("value", CondSet, scores);
                    reindexer::Error errSc = g_pocketdb->DB()->Select(queryScores, queryScoresResult);
                    if (errSc.ok()) {
                        vector<string> addressesRated;
                        for (auto itScore : queryScoresResult) {
                            reindexer::Item itmScore(itScore.GetItem());
                            if (g_antibot->AllowModifyReputationOverPost(itmScore["address"].As<string>(), postaddress, itmScore["block"].As<int>(), itmScore["time"].As<int64_t>(), itmScore["txid"].As<string>(), false)) {
                                if(find(addressesRated.begin(), addressesRated.end(), itmScore["address"].As<string>()) == addressesRated.end()) {
                                    addressesRated.push_back(itmScore["address"].As<string>());
                                    cntPositiveScores += itmScore["value"].As<int>() == 5 ? 1 : -1;
                                    *//*
                                    map<scoreDetail, string> postPrevScoresDetail; // DEBUGINFO
                                    postPrevScoresDetail[ADDRESS] = itmScore["address"].As<string>(); // DEBUGINFO
                                    postPrevScoresDetail[VALUE] = itmScore["value"].As<string>(); // DEBUGINFO
                                    postPrevScoresDetail[TXID] = itmScore["posttxid"].As<string>(); // DEBUGINFO
                                    postPrevScoresDetails[posttxid].push_back(postPrevScoresDetail); // DEBUGINFO
                                     *//*
                                }
                            }
                        }
                    }
                }
                postsRanks[posttxid][LAST5] = 1.0 * cntPositiveScores;
                postsRanks[posttxid][UREP] = userRep;
                postsRanks[posttxid][PREP] = postRep;
                postsRanks[posttxid][DREP] = pow(dekayRep, (nHeight - postblockOrig));
                postsRanks[posttxid][DPOST] = pow(dekayPost, (nHeight - postblockOrig));
            }

            map<PostRanks, int> count;
            int nElements = postsRanks.size();
            for (auto iPostRank : postsRanks) {
                count[LAST5R] = 0;
                count[UREPR] = 0;
                count[PREPR] = 0;
                double boost = 0;
                if (nElements > 1) {
                    for (auto jPostRank : postsRanks) {
                        if (iPostRank.second[LAST5] > jPostRank.second[LAST5]) {
                            count[LAST5R] += 1;
                        }
                        if (iPostRank.second[UREP] > jPostRank.second[UREP]) {
                            count[UREPR] += 1;
                        }
                        if (iPostRank.second[PREP] > jPostRank.second[PREP]) {
                            count[PREPR] += 1;
                        }
                    }
                    postsRanks[iPostRank.first][LAST5R] = 1.0 * (count[LAST5R] * 100) / (nElements - 1);
                    postsRanks[iPostRank.first][UREPR] = min(postsRanks[iPostRank.first][UREP], 1.0 * (count[UREPR] * 100) / (nElements - 1)) * (postsRanks[iPostRank.first][UREP] < 0 ? 2.0 : 1.0);
                    postsRanks[iPostRank.first][PREPR] = min(postsRanks[iPostRank.first][PREP], 1.0 * (count[PREPR] * 100) / (nElements - 1)) * (postsRanks[iPostRank.first][PREP] < 0 ? 2.0 : 1.0);
                }
                else {
                    postsRanks[iPostRank.first][LAST5R] = 100;
                    postsRanks[iPostRank.first][UREPR] = 100;
                    postsRanks[iPostRank.first][PREPR] = 100;
                }
                postsRanks[iPostRank.first][POSTRF] = 0.4 *
                                                      (0.75 *
                                                       (postsRanks[iPostRank.first][LAST5R] + boost) +
                                                       0.25 * postsRanks[iPostRank.first][UREPR]) *
                                                      postsRanks[iPostRank.first][DREP] +
                                                      0.6 * postsRanks[iPostRank.first][PREPR] * postsRanks[iPostRank.first][DPOST];
            }

            vector<pair<double, string>> postsRaited;

            UniValue oPost(UniValue::VOBJ);
            for (auto iPostRank : postsRanks) {
                postsRaited.push_back(make_pair(iPostRank.second[POSTRF], iPostRank.first));
            }

            sort(postsRaited.begin(), postsRaited.end(), greater{});

            for (auto v : postsRaited) {
                txidsHierarchical.push_back(v.second);
            }
        }

        UniValue contents(UniValue::VARR);*/

        if (txidsHierarchical.empty() || countOut > 0)
        {

        }

        return GetHistoricalStrip(request);
    }

}