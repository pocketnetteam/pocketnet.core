// Copyright (c) 2019 The Pocketcoin Core developers

#include <rpc/pocketrpc.h>

UniValue getcommentsV2(const JSONRPCRequest& request)
{
    if (request.fHelp)
        throw std::runtime_error(
            "getcomments (\"postid\", \"parentid\", \"address\", [\"commend_id\",\"commend_id\",...])\n"
            "\nGet Pocketnet comments.\n"
        );

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
    if (cmnids.size()>0)
        g_pocketdb->Select(
            Query("Comment")
                .Where("otxid", CondSet, cmnids)
                .Where("last", CondEq, true)
                .InnerJoin("otxid", "txid", CondEq, Query("Comment").Where("txid", CondSet, cmnids).Limit(1))
                .LeftJoin("otxid", "commentid", CondEq, Query("CommentScores").Where("address", CondSet, address).Limit(1))
        ,commRes);
    else 
        g_pocketdb->Select(
            Query("Comment")
                .Where("postid", CondEq, postid)
                .Where("parentid", CondEq, parentid)
                .Where("last", CondEq, true)
                .InnerJoin("otxid", "txid", CondEq, Query("Comment").Limit(1))
                .LeftJoin("otxid", "commentid", CondEq, Query("CommentScores").Where("address", CondSet, address).Limit(1))
        ,commRes);

    UniValue aResult(UniValue::VARR);
    for (auto& it : commRes) {
        reindexer::Item cmntItm = it.GetItem();
        reindexer::Item ocmntItm = it.GetJoined()[0][0].GetItem();
        
        int myScore = 0;
        if (it.GetJoined().size() > 1 &&it.GetJoined()[1].Count() > 0) {
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

UniValue getlastcommentsV2(const JSONRPCRequest& request)
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
            .Sort("time", true)
            .Limit(resultCount)
            .InnerJoin("otxid", "txid", CondEq, Query("Comment").Limit(1))
            .LeftJoin("otxid", "commentid", CondEq, Query("CommentScores").Where("address", CondSet, address).Limit(1))
    ,commRes);

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

static const CRPCCommand commands[] =
    {
        {"pocketnetrpc",   "getlastcomments2",    &getlastcommentsV2,      {"count","address"}},
        {"pocketnetrpc",   "getcomments2",        &getcommentsV2,          {"postid","parentid","address","ids"}},
};

void RegisterPocketnetRPCCommands(CRPCTable& t)
{
    for (unsigned int vcidx = 0; vcidx < ARRAYLEN(commands); vcidx++)
        t.appendCommand(commands[vcidx].name, &commands[vcidx]);
}
