#include <index/addrindex.h>
#include <antibot/antibot.h>
#include <rpc/server.h>
#include <timedata.h>
#include <univalue.h>
#include <utilstrencodings.h>
#include <validation.h>
#include "html.h"

UniValue sendcomment(const JSONRPCRequest& request)
{ 
    if (request.fHelp)
        throw std::runtime_error(
            "sendcomment\n"
            "\nCreate new Pocketnet comment.\n"
        );

    if (request.params.size() < 5)
        throw JSONRPCError(RPC_INVALID_PARAMS, "Not enough params");

    std::string id = "";
    id = request.params[0].get_str();
    if (id.length() == 0)
        throw JSONRPCError(RPC_INVALID_PARAMS, "Id is empty");

    reindexer::Item itmCmnt;
    reindexer::Error errCmnt = g_pocketdb->SelectOne(Query("Comments").Where("id", CondEq, id), itmCmnt);

    std::string postid = "";
    postid = request.params[1].get_str();
    if (postid.length() == 0)
        throw JSONRPCError(RPC_INVALID_PARAMS, "Postid is empty");
    reindexer::Item itmPost;
    reindexer::Error errPost = g_pocketdb->SelectOne(Query("Posts").Where("txid", CondEq, postid), itmPost);
    if (!errPost.ok())
        throw JSONRPCError(RPC_INVALID_PARAMS, "Invalid postid");

    std::string address = "";
    address = request.params[2].get_str();
    if (!IsValidDestination(DecodeDestination(address)) || (errCmnt.ok() && itmCmnt["address"].As<string>() != address))
        throw JSONRPCError(RPC_INVALID_PARAMS, "Invalid address");
    
    if (!g_antibot->CheckRegistration(address)) {
        throw JSONRPCError(RPC_INVALID_PARAMS, "Address not registered");
    }

	reindexer::Item itmBlock;
    reindexer::Error errBlock = g_pocketdb->SelectOne(Query("BlockingView").Where("address", CondEq, itmPost["address"].As<string>()).Where("address_to", CondEq, address), itmBlock);
	if (errBlock.ok())
        throw JSONRPCError(RPC_INVALID_REQUEST, "Blocking");

    std::string pubkey = "";
    pubkey = request.params[3].get_str();
    if (pubkey.length() == 0)
        throw JSONRPCError(RPC_INVALID_PARAMS, "Public key is empty");

    std::vector<unsigned char> vpubkey(ParseHex(pubkey));
    CPubKey cpubkey(vpubkey.begin(), vpubkey.end());
    if (!cpubkey.IsFullyValid())
        throw JSONRPCError(RPC_INVALID_PARAMS, "Invalid public key");

    CScript cspubkey(vpubkey);
    CTxDestination destAddress;
    if (ExtractDestination(GetScriptForRawPubKey(cpubkey), destAddress)) {
        if (EncodeDestination(destAddress) != address)
            throw JSONRPCError(RPC_INVALID_PARAMS, "Invalid public key or address");
    } else
        throw JSONRPCError(RPC_INVALID_PARAMS, "Public key is invalid");

    std::string signature = "";
    signature = request.params[4].get_str();
    if (signature.length() == 0)
        throw JSONRPCError(RPC_INVALID_PARAMS, "Signature is empty");

    std::string msg = "";
    if (request.params.size() > 5) {
        msg = request.params[5].get_str();
        if (UrlDecode(msg).length() > 2000)
            throw JSONRPCError(RPC_INVALID_PARAMS, "Message is too long");
    }
    
    UniValue omsg(UniValue::VOBJ);
    omsg.read(msg);
    std::string imgs = "";
    if (omsg["i"].isArray()) {
        if (omsg["i"].size() > 0)
            imgs = omsg["i"][0].get_str();
        for (unsigned int i = 1; i < omsg["i"].size(); i++)
            imgs = imgs + "," + omsg["i"][i].get_str();
    } else
        imgs = omsg["i"].get_str();
    std::string smsg = omsg["m"].get_str() + imgs + omsg["u"].get_str();

    unsigned char _hash[32] = {};
    CSHA256().Write((unsigned char*)smsg.data(), smsg.size()).Finalize(_hash);
    CSHA256().Write(_hash, sizeof(_hash)).Finalize(_hash);
    std::vector<unsigned char> vhash(_hash, _hash + sizeof(_hash));
    std::string hashhex = HexStr(vhash);
    uint256 hash(vhash);
    std::vector<unsigned char> vsig(ParseHex(signature));
    std::vector<unsigned char> vsig_der;
    ConvertToDER(vsig, vsig_der);
    if (!cpubkey.Verify(hash, vsig_der))
        throw JSONRPCError(RPC_INVALID_PARAMS, "Signature is invalid");
    
    std::string parentid = "";
    if (request.params.size() > 6) {
        parentid = request.params[6].get_str();
    }
    reindexer::Item itmCmntPrnt;
    reindexer::Error errCmntPrnt(13);
    if (parentid.length() > 0) {
        errCmntPrnt = g_pocketdb->SelectOne(Query("Comments").Where("id", CondEq, parentid), itmCmntPrnt);
        if (!errCmntPrnt.ok())
            throw JSONRPCError(RPC_INVALID_PARAMS, "Invalid parentid");
    }

    std::string answerid = "";
    if (request.params.size() > 7) {
        answerid = request.params[7].get_str();
    }
    reindexer::Item itmCmntAnswr;
    reindexer::Error errCmntAnswr(13);
    if (answerid.length() > 0) {
        errCmntAnswr = g_pocketdb->SelectOne(Query("Comments").Where("id", CondEq, answerid), itmCmntAnswr);
        if (!errCmntAnswr.ok())
            throw JSONRPCError(RPC_INVALID_PARAMS, "Invalid answerid");
    }

    int64_t time = GetAdjustedTime();

    reindexer::Item cmntItm = g_pocketdb->DB()->NewItem("Comments");

    cmntItm["id"] = id;
    cmntItm["postid"] = postid;
    cmntItm["address"] = address;
    cmntItm["pubkey"] = pubkey;
    cmntItm["signature"] = signature;
    cmntItm["time"] = errCmnt.ok() ? itmCmnt["time"].As<int64_t>() : time;
    cmntItm["block"] = chainActive.Height() + 1;
    cmntItm["msg"] = msg;
    cmntItm["parentid"] = parentid;
    cmntItm["answerid"] = answerid;
    cmntItm["timeupd"] = errCmnt.ok() ? time : 0;
    g_pocketdb->UpsertWithCommit("Comments", cmntItm);

    for (const auto& connWS : WSConnections) {
        std::string addr_to = "";
        std::string msgSubType = "comment";

        if (errCmntAnswr.ok() && itmCmntAnswr["address"].As<string>() == connWS.second.Address) {
            addr_to = itmCmntAnswr["address"].As<string>();
            msgSubType = "answer";
        } else if (errPost.ok() && itmPost["address"].As<string>() == connWS.second.Address) {
            addr_to = itmPost["address"].As<string>();
            msgSubType = "post";
        }

        if (addr_to != "" && addr_to != address) {
            UniValue msg(UniValue::VOBJ);
            msg.pushKV("addr", addr_to);
            msg.pushKV("addrFrom", address);
            msg.pushKV("msg", "comment");
            msg.pushKV("mesType", msgSubType);
            msg.pushKV("commentid", id);
            msg.pushKV("posttxid", postid);
            if (parentid != "") msg.pushKV("parentid", parentid);
            if (answerid != "") msg.pushKV("answerid", answerid);
            if (errCmnt.ok()) msg.pushKV("updated", "true");
            connWS.second.Connection->send(msg.write(), [](const SimpleWeb::error_code& ec) {});
        }
    }

    UniValue a(UniValue::VOBJ);
    a.pushKV("id", id);
    a.pushKV("time", time);
    return a;
}

UniValue getcomments(const JSONRPCRequest& request)
{
    if (request.fHelp)
        throw std::runtime_error(
            "getcomments\n"
            "\nGet Pocketnet comment.\n"
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
    
    vector<string> cmnids;
    if (request.params.size() > 2) {
        if (request.params[2].isArray()) {
            UniValue cmntid = request.params[2].get_array();
            for (unsigned int id = 0; id < cmntid.size(); id++) {
                cmnids.push_back(cmntid[id].get_str());
            }
        } else {
            throw JSONRPCError(RPC_INVALID_PARAMS, "Invalid inputs params");
        }
    }
    
    reindexer::QueryResults commRes;
    if (cmnids.size()>0)
        g_pocketdb->Select(Query("Comments").Where("id", CondSet, cmnids), commRes);
    else 
        g_pocketdb->Select(Query("Comments").Where("postid", CondEq, postid).Where("parentid", CondEq, parentid), commRes);

    UniValue aResult(UniValue::VARR);
    for (auto& c : commRes) {
        reindexer::Item cmntItm = c.GetItem();

        UniValue oCmnt(UniValue::VOBJ);
        oCmnt.pushKV("id", cmntItm["id"].As<string>());
        oCmnt.pushKV("postid", cmntItm["postid"].As<string>());
        oCmnt.pushKV("address", cmntItm["address"].As<string>());
        oCmnt.pushKV("pubkey", cmntItm["pubkey"].As<string>());
        oCmnt.pushKV("signature", cmntItm["signature"].As<string>());
        oCmnt.pushKV("time", cmntItm["time"].As<string>());
        oCmnt.pushKV("block", cmntItm["block"].As<string>());
        oCmnt.pushKV("msg", cmntItm["msg"].As<string>());
        oCmnt.pushKV("parentid", cmntItm["parentid"].As<string>());
        oCmnt.pushKV("answerid", cmntItm["answerid"].As<string>());
        oCmnt.pushKV("timeupd", cmntItm["timeupd"].As<string>());
        if (parentid == "")
            oCmnt.pushKV("children", std::to_string(g_pocketdb->SelectCount(Query("Comments").Where("parentid", CondEq, cmntItm["id"].As<string>()))));

        aResult.push_back(oCmnt);
    }

    return aResult;
}

UniValue getlastcomments(const JSONRPCRequest& request)
{
    if (request.fHelp)
        throw std::runtime_error(
            "getcomments\n"
            "\nGet Pocketnet comment.\n");

    int resulCount = 10;
    if (request.params.size() > 0) {
        ParseInt32(request.params[0].get_str(), &resulCount);
    }

    reindexer::QueryResults commRes;
	g_pocketdb->Select(Query("Comments").Sort("time", true).Limit(resulCount), commRes);

    UniValue aResult(UniValue::VARR);
    for (auto& c : commRes) {
        reindexer::Item cmntItm = c.GetItem();

        UniValue oCmnt(UniValue::VOBJ);
        oCmnt.pushKV("id", cmntItm["id"].As<string>());
        oCmnt.pushKV("postid", cmntItm["postid"].As<string>());
        oCmnt.pushKV("address", cmntItm["address"].As<string>());
        oCmnt.pushKV("pubkey", cmntItm["pubkey"].As<string>());
        oCmnt.pushKV("signature", cmntItm["signature"].As<string>());
        oCmnt.pushKV("time", cmntItm["time"].As<string>());
        oCmnt.pushKV("block", cmntItm["block"].As<string>());
        oCmnt.pushKV("msg", cmntItm["msg"].As<string>());
        oCmnt.pushKV("parentid", cmntItm["parentid"].As<string>());
        oCmnt.pushKV("answerid", cmntItm["answerid"].As<string>());
        oCmnt.pushKV("timeupd", cmntItm["timeupd"].As<string>());

        aResult.push_back(oCmnt);
    }

    return aResult;
}

static const CRPCCommand commands[] =
    {
        //  category              name                            actor (function)            argNames
        //  --------------------- ------------------------        -----------------------     ----------
        {"rawtransactions", "sendcomment",     &sendcomment,       {"id", "postid", "address", "pubkey", "signature", "msg", "parentid", "answerid"}},
        {"rawtransactions", "getcomments",     &getcomments,       {"postid", "parentid"}},
        {"rawtransactions", "getlastcomments", &getlastcomments,   {"count"}},
};

void RegisterCommentsRPCCommands(CRPCTable& t)
{
    for (unsigned int vcidx = 0; vcidx < ARRAYLEN(commands); vcidx++)
        t.appendCommand(commands[vcidx].name, &commands[vcidx]);
}
