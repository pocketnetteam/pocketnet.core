// Copyright (c) 2019 The Pocketcoin Core developers

#include <rpc/pocketrpc.h>

#include <pos.h>
#include <validation.h>
//#include <logging.h>

UniValue getcomments(const JSONRPCRequest& request)
{
    if (request.fHelp)
        throw std::runtime_error(
            "getcomments (\"postid\", \"parentid\", \"address\", [\"commend_id\",\"commend_id\",...])\n"
            "\nGet Pocketnet comments.\n");

    std::string postid = "";
    if (request.params.size() > 0) {
        postid = request.params[0].get_str();
        if (postid.length() == 0 && request.params.size() < 3)
            throw JSONRPCError(RPC_INVALID_PARAMS, "Invalid postid");
    }

    std::string parentid = "";
    if (request.params.size() > 1) {
        parentid = request.params[1].get_str();
    }

    std::string address = "";
    if (request.params.size() > 2) {
        address = request.params[2].get_str();
    }

    vector<string> cmnids;
    if (request.params.size() > 3) {
        if (request.params[3].isArray()) {
            UniValue cmntid = request.params[3].get_array();
            for (unsigned int id = 0; id < cmntid.size(); id++) {
                cmnids.push_back(cmntid[id].get_str());
            }
        } else {
            throw JSONRPCError(RPC_INVALID_PARAMS, "Invalid inputs params");
        }
    }

    // For joined tables
    // it.GetJoined()[1][0]
    // [1] - table
    // [0] - item in table
    reindexer::QueryResults commRes;
    if (cmnids.size() > 0)
        g_pocketdb->Select(
            Query("Comment")
                .Where("otxid", CondSet, cmnids)
                .Where("last", CondEq, true)
                .Where("time", CondLe, GetAdjustedTime())
                .InnerJoin("otxid", "txid", CondEq, Query("Comment").Where("txid", CondSet, cmnids).Limit(1))
                .LeftJoin("otxid", "commentid", CondEq, Query("CommentScores").Where("address", CondEq, address).Limit(1)),
            commRes);
    else
        g_pocketdb->Select(
            Query("Comment")
                .Where("postid", CondEq, postid)
                .Where("parentid", CondEq, parentid)
                .Where("last", CondEq, true)
                .Where("time", CondLe, GetAdjustedTime())
                .InnerJoin("otxid", "txid", CondEq, Query("Comment").Limit(1))
                .LeftJoin("otxid", "commentid", CondEq, Query("CommentScores").Where("address", CondEq, address).Limit(1)),
            commRes);

    UniValue aResult(UniValue::VARR);
    for (auto& it : commRes) {
        reindexer::Item cmntItm = it.GetItem();
        reindexer::Item ocmntItm = it.GetJoined()[0][0].GetItem();

        int myScore = 0;
        if (it.GetJoined().size() > 1 && it.GetJoined()[1].Count() > 0) {
            reindexer::Item ocmntScoreItm = it.GetJoined()[1][0].GetItem();
            myScore = ocmntScoreItm["value"].As<int>();
        }

        UniValue oCmnt(UniValue::VOBJ);
        oCmnt.pushKV("id", cmntItm["otxid"].As<string>());
        oCmnt.pushKV("postid", cmntItm["postid"].As<string>());
        oCmnt.pushKV("address", cmntItm["address"].As<string>());
        oCmnt.pushKV("time", ocmntItm["time"].As<string>());
        oCmnt.pushKV("timeUpd", cmntItm["time"].As<string>());
        oCmnt.pushKV("block", cmntItm["block"].As<string>());
        oCmnt.pushKV("msg", cmntItm["msg"].As<string>());
        oCmnt.pushKV("parentid", cmntItm["parentid"].As<string>());
        oCmnt.pushKV("answerid", cmntItm["answerid"].As<string>());
        oCmnt.pushKV("scoreUp", cmntItm["scoreUp"].As<string>());
        oCmnt.pushKV("scoreDown", cmntItm["scoreDown"].As<string>());
        oCmnt.pushKV("reputation", cmntItm["reputation"].As<string>());
        oCmnt.pushKV("edit", cmntItm["otxid"].As<string>() != cmntItm["txid"].As<string>());
        oCmnt.pushKV("deleted", cmntItm["msg"].As<string>() == "");
        oCmnt.pushKV("myScore", myScore);
        oCmnt.pushKV("children", std::to_string(g_pocketdb->SelectCount(Query("Comment").Where("parentid", CondEq, cmntItm["otxid"].As<string>()).Where("last", CondEq, true))));

        aResult.push_back(oCmnt);
    }

    return aResult;
}

UniValue getlastcomments(const JSONRPCRequest& request)
{
    if (request.fHelp)
        throw std::runtime_error(
            "getlastcomments (count)\n"
            "\nGet Pocketnet last comments.\n");

    int resultCount = 10;
    if (request.params.size() > 0) {
        ParseInt32(request.params[0].get_str(), &resultCount);
    }

    std::string address = "";
    if (request.params.size() > 1) {
        address = request.params[1].get_str();
    }

    reindexer::QueryResults commRes;
    g_pocketdb->Select(
        Query("Comment")
            .Where("last", CondEq, true)
            .Where("time", CondLe, GetAdjustedTime())
            .Sort("time", true)
            .Limit(resultCount)
            .InnerJoin("otxid", "txid", CondEq, Query("Comment").Limit(1))
            .LeftJoin("otxid", "commentid", CondEq, Query("CommentScores").Where("address", CondEq, address).Limit(1)),
        commRes);

    UniValue aResult(UniValue::VARR);
    for (auto& it : commRes) {
        reindexer::Item cmntItm = it.GetItem();
        reindexer::Item ocmntItm = it.GetJoined()[0][0].GetItem();

        int myScore = 0;
        if (it.GetJoined().size() > 1 && it.GetJoined()[1].Count() > 0) {
            reindexer::Item ocmntScoreItm = it.GetJoined()[1][0].GetItem();
            myScore = ocmntScoreItm["value"].As<int>();
        }

        UniValue oCmnt(UniValue::VOBJ);
        oCmnt.pushKV("id", cmntItm["otxid"].As<string>());
        oCmnt.pushKV("postid", cmntItm["postid"].As<string>());
        oCmnt.pushKV("address", cmntItm["address"].As<string>());
        oCmnt.pushKV("time", ocmntItm["time"].As<string>());
        oCmnt.pushKV("timeUpd", cmntItm["time"].As<string>());
        oCmnt.pushKV("block", cmntItm["block"].As<string>());
        oCmnt.pushKV("msg", cmntItm["msg"].As<string>());
        oCmnt.pushKV("parentid", cmntItm["parentid"].As<string>());
        oCmnt.pushKV("answerid", cmntItm["answerid"].As<string>());
        oCmnt.pushKV("scoreUp", cmntItm["scoreUp"].As<string>());
        oCmnt.pushKV("scoreDown", cmntItm["scoreDown"].As<string>());
        oCmnt.pushKV("reputation", cmntItm["reputation"].As<string>());
        oCmnt.pushKV("edit", cmntItm["otxid"].As<string>() != cmntItm["txid"].As<string>());
        oCmnt.pushKV("deleted", cmntItm["msg"].As<string>() == "");
        oCmnt.pushKV("myScore", myScore);

        aResult.push_back(oCmnt);
    }

    return aResult;
}

UniValue getaddressscores(const JSONRPCRequest& request)
{
    if (request.fHelp)
        throw std::runtime_error(
            "getaddressscores\n"
            "\nGet scores from address.\n");

    std::string address = "";
    if (request.params.size() > 0 && request.params[0].isStr()) {
        CTxDestination dest = DecodeDestination(request.params[0].get_str());

        if (!IsValidDestination(dest)) {
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, std::string("Invalid address: ") + request.params[0].get_str());
        }

        address = request.params[0].get_str();
    } else {
        throw JSONRPCError(RPC_INVALID_PARAMS, "There is no address.");
    }

    vector<string> TxIds;
    if (request.params.size() > 1) {
        if (request.params[1].isArray()) {
            UniValue txid = request.params[1].get_array();
            for (unsigned int idx = 0; idx < txid.size(); idx++) {
                TxIds.push_back(txid[idx].get_str());
            }
        } else if (request.params[1].isStr()) {
            TxIds.push_back(request.params[1].get_str());
        } else {
            throw JSONRPCError(RPC_INVALID_PARAMS, "Invalid txs format");
        }
    }

    reindexer::QueryResults queryRes;
    if (TxIds.empty()) {
        g_pocketdb->DB()->Select(
            reindexer::Query("Scores").Where("address", CondEq, address).InnerJoin("address", "address", CondEq, Query("UsersView").Where("address", CondEq, address)).Sort("time", true),
            queryRes);
    } else {
        g_pocketdb->DB()->Select(
            reindexer::Query("Scores").Where("address", CondEq, address).Where("posttxid", CondSet, TxIds).InnerJoin("address", "address", CondEq, Query("UsersView").Where("address", CondEq, address)).Sort("time", true),
            queryRes);
    }

    UniValue result(UniValue::VARR);
    for (auto it : queryRes) {
        reindexer::Item itm(it.GetItem());
        reindexer::Item itmj(it.GetJoined()[0][0].GetItem());

        UniValue postscore(UniValue::VOBJ);
        postscore.pushKV("posttxid", itm["posttxid"].As<string>());
        postscore.pushKV("address", itm["address"].As<string>());
        postscore.pushKV("name", itmj["name"].As<string>());
        postscore.pushKV("avatar", itmj["avatar"].As<string>());
        postscore.pushKV("reputation", itmj["reputation"].As<string>());
        postscore.pushKV("value", itm["value"].As<string>());
        result.push_back(postscore);
    }
    return result;
}

UniValue getpostscores(const JSONRPCRequest& request)
{
    if (request.fHelp)
        throw std::runtime_error(
            "getpostscores\n"
            "\nGet scores from address.\n");

    vector<string> TxIds;
    if (request.params.size() > 0) {
        if (request.params[0].isArray()) {
            UniValue txid = request.params[0].get_array();
            for (unsigned int idx = 0; idx < txid.size(); idx++) {
                TxIds.push_back(txid[idx].get_str());
            }
        } else if (request.params[0].isStr()) {
            TxIds.push_back(request.params[0].get_str());
        } else {
            throw JSONRPCError(RPC_INVALID_PARAMS, "Invalid txs format");
        }
    } else {
        throw JSONRPCError(RPC_INVALID_PARAMS, "There are no txid");
    }

    std::string address = "";
    if (request.params.size() > 1 && request.params[1].isStr()) {
        CTxDestination dest = DecodeDestination(request.params[1].get_str());

        if (!IsValidDestination(dest)) {
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, std::string("Invalid address: ") + request.params[1].get_str());
        }

        address = request.params[1].get_str();
    }

    UniValue result(UniValue::VARR);

    reindexer::QueryResults queryRes1;
    g_pocketdb->DB()->Select(
        reindexer::Query("Scores").Where("posttxid", CondSet, TxIds).InnerJoin("address", "address_to", CondEq, Query("SubscribesView").Where("address", CondEq, address)).InnerJoin("address", "address", CondEq, Query("UsersView").Where("address", CondEq, address))
        //.Sort("txid", true).Sort("private", true).Sort("reputation", true)
        ,
        queryRes1);

    std::vector<std::string> subscribeadrs;
    for (auto it : queryRes1) {
        reindexer::Item itm(it.GetItem());
        reindexer::Item itmj(it.GetJoined()[0][0].GetItem());
        reindexer::Item itmj2(it.GetJoined()[1][0].GetItem());
        UniValue postscore(UniValue::VOBJ);
        postscore.pushKV("posttxid", itm["posttxid"].As<string>());
        postscore.pushKV("address", itm["address"].As<string>());
        postscore.pushKV("name", itmj2["name"].As<string>());
        postscore.pushKV("avatar", itmj2["avatar"].As<string>());
        postscore.pushKV("reputation", itmj2["reputation"].As<string>());
        postscore.pushKV("value", itm["value"].As<string>());
        postscore.pushKV("isprivate", itmj["private"].As<string>());
        result.push_back(postscore);

        subscribeadrs.push_back(itm["address"].As<string>());
    }

    reindexer::QueryResults queryRes2;
    g_pocketdb->DB()->Select(
        reindexer::Query("Scores").Where("posttxid", CondSet, TxIds).Not().Where("address", CondSet, subscribeadrs).InnerJoin("address", "address", CondEq, Query("UsersView").Not().Where("address", CondSet, subscribeadrs))
        //.Sort("txid", true).Sort("reputation", true)
        ,
        queryRes2);

    for (auto it : queryRes2) {
        reindexer::Item itm(it.GetItem());
        reindexer::Item itmj(it.GetJoined()[0][0].GetItem());
        UniValue postscore(UniValue::VOBJ);
        postscore.pushKV("posttxid", itm["posttxid"].As<string>());
        postscore.pushKV("address", itm["address"].As<string>());
        postscore.pushKV("name", itmj["name"].As<string>());
        postscore.pushKV("avatar", itmj["avatar"].As<string>());
        postscore.pushKV("reputation", itmj["reputation"].As<string>());
        postscore.pushKV("value", itm["value"].As<string>());
        result.push_back(postscore);
    }

    return result;
}

UniValue getpagescores(const JSONRPCRequest& request)
{
    if (request.fHelp)
        throw std::runtime_error(
            "getpagescores\n"
            "\nGet scores for post(s) from address.\n");

    vector<string> TxIds;
    if (request.params.size() > 0) {
        if (request.params[0].isArray()) {
            UniValue txid = request.params[0].get_array();
            for (unsigned int idx = 0; idx < txid.size(); idx++) {
                TxIds.push_back(txid[idx].get_str());
            }
        } else if (request.params[0].isStr()) {
            TxIds.push_back(request.params[0].get_str());
        } else {
            throw JSONRPCError(RPC_INVALID_PARAMS, "Invalid txs format");
        }
    } else {
        throw JSONRPCError(RPC_INVALID_PARAMS, "There are no txid");
    }

    std::string address = "";
    if (request.params.size() > 1 && request.params[1].isStr()) {
        CTxDestination dest = DecodeDestination(request.params[1].get_str());

        if (!IsValidDestination(dest)) {
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, std::string("Invalid address: ") + request.params[1].get_str());
        }

        address = request.params[1].get_str();
    } else {
        throw JSONRPCError(RPC_INVALID_PARAMS, "There is no address.");
    }

    vector<string> CmntIds;
    if (request.params.size() > 2) {
        if (request.params[2].isArray()) {
            UniValue cmntid = request.params[2].get_array();
            for (unsigned int id = 0; id < cmntid.size(); id++) {
                CmntIds.push_back(cmntid[id].get_str());
            }
        } else if (request.params[2].isStr()) {
            CmntIds.push_back(request.params[2].get_str());
        } else {
            throw JSONRPCError(RPC_INVALID_PARAMS, "Invalid cmntids format");
        }
    }

    reindexer::QueryResults queryRes;
    g_pocketdb->DB()->Select(
        reindexer::Query("Posts").Where("txid", CondSet, TxIds).LeftJoin("txid", "posttxid", CondEq, Query("Scores").Where("address", CondEq, address).Where("value", CondGt, 3)), queryRes);

    UniValue result(UniValue::VARR);
    for (auto it : queryRes) {
        reindexer::Item itm(it.GetItem());
        reindexer::Item itmj;

        //1. Add postlikers from private subscribes
        reindexer::QueryResults queryResLikers1;
        g_pocketdb->DB()->Select(
            reindexer::Query("UsersView")
                .InnerJoin("address", "address", CondEq, Query("Scores").Where("posttxid", CondEq, itm["txid"].As<string>()).Where("value", CondGt, 3))
                .InnerJoin("address", "address_to", CondEq, Query("SubscribesView").Where("address", CondEq, address).Where("private", CondEq, true))
                .Sort("reputation", true)
                .Limit(3),
            queryResLikers1);

        UniValue postlikers(UniValue::VARR);
        std::vector<std::string> postlikersadrs;
        for (auto itl : queryResLikers1) {
            reindexer::Item itmL(itl.GetItem());

            UniValue postliker(UniValue::VOBJ);
            postliker.pushKV("address", itmL["address"].As<string>());
            postliker.pushKV("name", itmL["name"].As<string>());
            postliker.pushKV("avatar", itmL["avatar"].As<string>());
            postlikers.push_back(postliker);
            postlikersadrs.push_back(itmL["address"].As<string>());
        }

        //2. Add postlikers from not private subscribes
        if (postlikers.size() < 3) {
            reindexer::QueryResults queryResLikers2;
            g_pocketdb->DB()->Select(
                reindexer::Query("UsersView")
                    .InnerJoin("address", "address", CondEq, Query("Scores").Where("posttxid", CondEq, itm["txid"].As<string>()).Where("value", CondGt, 3))
                    .InnerJoin("address", "address_to", CondEq, Query("SubscribesView").Where("address", CondEq, address).Where("private", CondEq, false))
                    .Sort("reputation", true)
                    .Limit(3 - postlikers.size()),
                queryResLikers2);

            for (auto itl : queryResLikers2) {
                reindexer::Item itmL(itl.GetItem());

                UniValue postliker(UniValue::VOBJ);
                postliker.pushKV("address", itmL["address"].As<string>());
                postliker.pushKV("name", itmL["name"].As<string>());
                postliker.pushKV("avatar", itmL["avatar"].As<string>());
                postlikers.push_back(postliker);
                postlikersadrs.push_back(itmL["address"].As<string>());
            }
        }

        //3. Add postlikers from not subscribes
        if (postlikers.size() < 3) {
            postlikersadrs.push_back(address);
            reindexer::QueryResults queryResLikers3;
            g_pocketdb->DB()->Select(
                reindexer::Query("UsersView")
                    .InnerJoin("address", "address", CondEq, Query("Scores").Where("posttxid", CondEq, itm["txid"].As<string>()).Where("value", CondGt, 3).Not().Where("address", CondSet, postlikersadrs))
                    .Sort("reputation", true)
                    .Limit(3 - postlikers.size()),
                queryResLikers3);

            for (auto itl : queryResLikers3) {
                reindexer::Item itmL(itl.GetItem());

                UniValue postliker(UniValue::VOBJ);
                postliker.pushKV("address", itmL["address"].As<string>());
                postliker.pushKV("name", itmL["name"].As<string>());
                postliker.pushKV("avatar", itmL["avatar"].As<string>());
                postlikers.push_back(postliker);
            }
        }

        UniValue postscore(UniValue::VOBJ);
        postscore.pushKV("posttxid", itm["txid"].As<string>());
        if (it.GetJoined().size() > 0) postscore.pushKV("value", it.GetJoined()[0][0].GetItem()["value"].As<string>());
        postscore.pushKV("postlikers", postlikers);
        result.push_back(postscore);
    }

    reindexer::QueryResults commRes;
    g_pocketdb->Select(
        Query("Comment")
            .Where("otxid", CondSet, CmntIds)
            .Where("last", CondEq, true)
            .Where("time", CondLe, GetAdjustedTime())
            .LeftJoin("otxid", "commentid", CondEq, Query("CommentScores").Where("address", CondEq, address).Limit(1)),
        commRes);

    for (auto cit : commRes) {
        reindexer::Item cmntItm = cit.GetItem();

        UniValue cmntscore(UniValue::VOBJ);
        cmntscore.pushKV("cmntid", cmntItm["otxid"].As<string>());
        cmntscore.pushKV("scoreUp", cmntItm["scoreUp"].As<string>());
        cmntscore.pushKV("scoreDown", cmntItm["scoreDown"].As<string>());
        cmntscore.pushKV("reputation", cmntItm["reputation"].As<string>());
        if (cit.GetJoined().size() > 0) cmntscore.pushKV("myscore", cit.GetJoined()[0][0].GetItem()["value"].As<string>());
        result.push_back(cmntscore);
    }

    return result;
}

UniValue debug(const JSONRPCRequest& request)
{
    if (request.fHelp)
        throw std::runtime_error(
            "debug\n"
            "\nFor debugging purposes.\n");

    UniValue result(UniValue::VOBJ);

    return result;
}

static const CRPCCommand commands[] =
    {
        {"pocketnetrpc", "getlastcomments2", &getlastcomments, {"count", "address"}},
        {"pocketnetrpc", "getlastcomments", &getlastcomments, {"count", "address"}},
        {"pocketnetrpc", "getcomments2", &getcomments, {"postid", "parentid", "address", "ids"}},
        {"pocketnetrpc", "getcomments", &getcomments, {"postid", "parentid", "address", "ids"}},
        {"pocketnetrpc", "getaddressscores", &getaddressscores, {"address", "txs"}},
        {"pocketnetrpc", "getpostscores", &getpostscores, {"txs", "address"}},
        {"pocketnetrpc", "getpagescores", &getpagescores, {"txs", "address", "cmntids"}},

        {"hidden", "debug", &debug, {}},
};

void RegisterPocketnetRPCCommands(CRPCTable& t)
{
    for (unsigned int vcidx = 0; vcidx < ARRAYLEN(commands); vcidx++)
        t.appendCommand(commands[vcidx].name, &commands[vcidx]);
}
