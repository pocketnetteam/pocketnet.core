// Copyright (c) 2019-2021 The Pocketcoin Core developers

#include <rpc/pocketrpc.h>

#include <pos.h>
#include <validation.h>
//#include <logging.h>

//repetition in rawtransaction.cpp
static void TxToJSON(const CTransaction& tx, const uint256 hashBlock, UniValue& entry)
{
    // Call into TxToUniv() in pocketcoin-common to decode the transaction hex.
    //
    // Blockchain contextual information (confirmations and blocktime) is not
    // available to code in pocketcoin-common, so we query them here and push the
    // data into the returned UniValue.
    TxToUniv(tx, uint256(), entry, true, RPCSerializationFlags());

    entry.pushKV("pockettx", g_addrindex->IsPocketnetTransaction(tx));

    if (!hashBlock.IsNull()) {
        entry.pushKV("blockhash", hashBlock.GetHex());
        CBlockIndex* pindex = LookupBlockIndexWithoutAssert(hashBlock);
        if (pindex) {
            if (chainActive.Contains(pindex)) {
                entry.pushKV("confirmations", 1 + chainActive.Height() - pindex->nHeight);
                entry.pushKV("time", pindex->GetBlockTime());
                entry.pushKV("blocktime", pindex->GetBlockTime());
            } else
                entry.pushKV("confirmations", 0);
        }
    }
}
static UniValue Sendrawtransaction(RTransaction& rtx)
{
    const uint256& hashTx = rtx->GetHash();

    std::promise<void> promise;
    CAmount nMaxRawTxFee = maxTxFee;

    { // cs_main scope
        LOCK(cs_main);
        CCoinsViewCache& view = *pcoinsTip;
        bool fHaveChain = false;
        for (size_t o = 0; !fHaveChain && o < rtx->vout.size(); o++) {
            const Coin& existingCoin = view.AccessCoin(COutPoint(hashTx, o));
            fHaveChain = !existingCoin.IsSpent();
        }
        bool fHaveMempool = mempool.exists(hashTx);
        if (!fHaveMempool && !fHaveChain) {
            // push to local node and sync with wallets
            CValidationState state;
            bool fMissingInputs;
            if (!AcceptToMemoryPool(mempool, state, rtx, &fMissingInputs, nullptr /* plTxnReplaced */, false /* bypass_limits */, nMaxRawTxFee)) {
                if (state.IsInvalid()) {
                    throw JSONRPCError(RPC_TRANSACTION_REJECTED, FormatStateMessage(state));
                } else {
                    if (state.GetRejectCode() == RPC_POCKETTX_MATURITY) {
                        throw JSONRPCError(RPC_POCKETTX_MATURITY, FormatStateMessage(state));
                    } else {
                        if (fMissingInputs) {
                            throw JSONRPCError(RPC_TRANSACTION_ERROR, "Missing inputs");
                        }
                        throw JSONRPCError(RPC_TRANSACTION_ERROR, FormatStateMessage(state));
                    }
                }
            } else {
                // If wallet is enabled, ensure that the wallet has been made aware
                // of the new transaction prior to returning. This prevents a race
                // where a user might call sendrawtransaction with a transaction
                // to/from their wallet, immediately call some wallet RPC, and get
                // a stale result because callbacks have not yet been processed.
                CallFunctionInValidationInterfaceQueue([&promise] {
                  promise.set_value();
                });
            }
        } else if (fHaveChain) {
            throw JSONRPCError(RPC_TRANSACTION_ALREADY_IN_CHAIN, "transaction already in block chain");
        } else {
            // Make sure we don't block forever if re-sending
            // a transaction already in mempool.
            promise.set_value();
        }

    } // cs_main

    promise.get_future().wait();

    if (!g_connman)
        throw JSONRPCError(RPC_CLIENT_P2P_DISABLED, "Error: Peer-to-peer functionality missing or disabled");

    CInv inv(MSG_TX, hashTx);
    g_connman->ForEachNode([&inv](CNode* pnode) {
      pnode->PushInventory(inv);
    });

    return hashTx.GetHex();
}
//----------------------------------------------------------
std::map<std::string, UniValue> getUsersProfiles(std::vector<std::string> addresses, bool shortForm = true, int option = 0)
{
    std::map<std::string, UniValue> result;

    // Get users
    reindexer::QueryResults _users_res;
    g_pocketdb->DB()->Select(reindexer::Query("UsersView").Where("address", CondSet, addresses), _users_res);

    // Get count of posts by addresses
    reindexer::AggregationResult aggRes;
    std::map<std::string, int> _posts_cnt;
    if (g_pocketdb->SelectAggr(reindexer::Query("Posts").Where("address", CondSet, addresses).Aggregate("address", AggFacet), "address", aggRes).ok()) {
        for (const auto& f : aggRes.facets) {
            _posts_cnt.insert_or_assign(f.value, f.count);
        }
    }

    // Build return object array
    for (auto& it : _users_res) {
        UniValue entry(UniValue::VOBJ);
        reindexer::Item itm = it.GetItem();
        std::string _address = itm["address"].As<string>();

        // Minimal fields for short form
        entry.pushKV("address", _address);
        entry.pushKV("name", itm["name"].As<string>());
        entry.pushKV("id", itm["id"].As<int>() + 1);
        entry.pushKV("i", itm["avatar"].As<string>());
        entry.pushKV("b", itm["donations"].As<string>());
        entry.pushKV("r", itm["referrer"].As<string>());
        entry.pushKV("reputation", itm["reputation"].As<int>() / 10.0);

        if (_posts_cnt.find(_address) != _posts_cnt.end()) {
            entry.pushKV("postcnt", _posts_cnt[_address]);
        }

        // Count of referrals
        size_t _referrals_count = g_pocketdb->SelectCount(reindexer::Query("UsersView").Where("referrer", CondEq, _address));
        entry.pushKV("rc", (int)_referrals_count);

        if (option == 1)
            entry.pushKV("a", itm["about"].As<string>());

        // In full form add other fields
        if (!shortForm) {
            entry.pushKV("regdate", itm["regdate"].As<int64_t>());
            if (option != 1)
                entry.pushKV("a", itm["about"].As<string>());
            entry.pushKV("l", itm["lang"].As<string>());
            entry.pushKV("s", itm["url"].As<string>());
            entry.pushKV("update", itm["time"].As<int64_t>());
            entry.pushKV("k", itm["pubkey"].As<string>());
            //entry.pushKV("birthday", itm["birthday"].As<int>());
            //entry.pushKV("gender", itm["gender"].As<int>());

            // Subscribes
            reindexer::QueryResults queryResSubscribes;
            reindexer::Error errS = g_pocketdb->DB()->Select(
                reindexer::Query("SubscribesView")
                    .Where("address", CondEq, _address),
                queryResSubscribes);

            UniValue aS(UniValue::VARR);
            if (errS.ok() && queryResSubscribes.Count() > 0) {
                for (auto itS : queryResSubscribes) {
                    UniValue entryS(UniValue::VOBJ);
                    reindexer::Item curSbscrbItm(itS.GetItem());
                    entryS.pushKV("adddress", curSbscrbItm["address_to"].As<string>());
                    entryS.pushKV("private", curSbscrbItm["private"].As<string>());
                    aS.push_back(entryS);
                }
            }
            entry.pushKV("subscribes", aS);

            // Subscribers
            reindexer::QueryResults queryResSubscribers;
            reindexer::Error errRS = g_pocketdb->DB()->Select(
                reindexer::Query("SubscribesView")
                    .Where("address_to", CondEq, _address),
                //.Where("private", CondEq, false),
                queryResSubscribers);

            UniValue arS(UniValue::VARR);
            if (errRS.ok() && queryResSubscribers.Count() > 0) {
                for (auto itS : queryResSubscribers) {
                    reindexer::Item curSbscrbItm(itS.GetItem());
                    arS.push_back(curSbscrbItm["address"].As<string>());
                }
            }
            entry.pushKV("subscribers", arS);

            // Blockings
            reindexer::QueryResults queryResBlockings;
            reindexer::Error errRB = g_pocketdb->DB()->Select(
                reindexer::Query("BlockingView")
                    .Where("address", CondEq, _address),
                queryResBlockings);

            UniValue arB(UniValue::VARR);
            if (errRB.ok() && queryResBlockings.Count() > 0) {
                for (auto itB : queryResBlockings) {
                    reindexer::Item curBlckItm(itB.GetItem());
                    arB.push_back(curBlckItm["address_to"].As<string>());
                }
            }
            entry.pushKV("blocking", arB);

            // Recommendations subscribtions
            /*
            std::vector<string> recomendedSubscriptions;
            g_addrindex->GetRecomendedSubscriptions(_address, 10, recomendedSubscriptions);

            UniValue rs(UniValue::VARR);
            for (std::string r : recomendedSubscriptions) {
                rs.push_back(r);
            }
            entry.pushKV("recomendedSubscribes", rs);
            */
        }

        result.insert_or_assign(_address, entry);
    }

    return result;
}
//----------------------------------------------------------
UniValue getPostData(reindexer::Item& itm, std::string address)
{
    UniValue entry(UniValue::VOBJ);

    entry.pushKV("txid", itm["txid"].As<string>());
    if (itm["txidEdit"].As<string>() != "") entry.pushKV("edit", "true");
    if (itm["txidRepost"].As<string>() != "") entry.pushKV("repost", itm["txidRepost"].As<string>());
    entry.pushKV("address", itm["address"].As<string>());
    entry.pushKV("time", itm["time"].As<string>());
    entry.pushKV("l", itm["lang"].As<string>());
    entry.pushKV("c", itm["caption"].As<string>());
    entry.pushKV("m", itm["message"].As<string>());
    entry.pushKV("u", itm["url"].As<string>());
    entry.pushKV("type", getcontenttype(itm["type"].As<int>()));

    entry.pushKV("scoreSum", itm["scoreSum"].As<string>());
    entry.pushKV("scoreCnt", itm["scoreCnt"].As<string>());

    try {
        UniValue t(UniValue::VARR);
        reindexer::VariantArray va = itm["tags"];
        for (unsigned int idx = 0; idx < va.size(); idx++) {
            t.push_back(va[idx].As<string>());
        }
        entry.pushKV("t", t);
    } catch (...) {
    }

    try {
        UniValue i(UniValue::VARR);
        reindexer::VariantArray va = itm["images"];
        for (unsigned int idx = 0; idx < va.size(); idx++) {
            i.push_back(va[idx].As<string>());
        }
        entry.pushKV("i", i);
    } catch (...) {
    }

    UniValue ss(UniValue::VOBJ);
    ss.read(itm["settings"].As<string>());
    entry.pushKV("s", ss);

    if (address != "") {
        reindexer::Item scoreMyItm;
        reindexer::Error errS = g_pocketdb->SelectOne(
            reindexer::Query("Scores").Where("address", CondEq, address).Where("posttxid", CondEq, itm["txid"].As<string>()),
            scoreMyItm);

        entry.pushKV("myVal", errS.ok() ? scoreMyItm["value"].As<string>() : "0");
    }

    int totalComments = g_pocketdb->SelectCount(Query("Comment").Where("postid", CondEq, itm["txid"].As<string>()).Where("last", CondEq, true));
    entry.pushKV("comments", totalComments);

    reindexer::QueryResults cmntRes;
    g_pocketdb->Select(
        Query("Comment", 0, 1)
            .Where("postid", CondEq, itm["txid"].As<string>())
            .Where("parentid", CondEq, "")
            .Where("last", CondEq, true)
            .Sort("time", true)
            .InnerJoin("otxid", "txid", CondEq, Query("Comment").Limit(1))
            .LeftJoin("otxid", "commentid", CondEq, Query("CommentScores").Where("address", CondSet, address).Limit(1)),
        cmntRes);

    if (totalComments > 0 && cmntRes.Count() > 0) {
        UniValue oCmnt(UniValue::VOBJ);

        reindexer::Item cmntItm = cmntRes[0].GetItem();
        reindexer::Item ocmntItm = cmntRes[0].GetJoined()[0][0].GetItem();

        int myScore = 0;
        if (cmntRes[0].GetJoined().size() > 1 && cmntRes[0].GetJoined()[1].Count() > 0) {
            reindexer::Item ocmntScoreItm = cmntRes[0].GetJoined()[1][0].GetItem();
            myScore = ocmntScoreItm["value"].As<int>();
        }

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

        entry.pushKV("lastComment", oCmnt);
    }

    int totalReposted = g_pocketdb->SelectCount(Query("Posts").Where("txidRepost", CondEq, itm["txid"].As<string>()));
    if (totalReposted > 0)
        entry.pushKV("reposted", totalReposted);

    std::map<std::string, UniValue> profile = getUsersProfiles(std::vector<std::string>{itm["address"].As<string>()}, true);
    if (profile.size() > 0)
        entry.pushKV("userprofile", profile.begin()->second);


    return entry;
}
//----------------------------------------------------------
void getFastSearchString(std::string search, std::string str, std::map<std::string, int>& mFastSearch)
{
    std::size_t found = str.find(search);
    int cntfound = 0;
    if (found != std::string::npos && found + search.length() < str.length()) {
        std::string subst = str.substr(found + search.length());
        std::string runningstr = "";
        for (char& c : subst) {
            if (c == ' ' || c == ',' || c == '.' || c == '!' || c == ')' || c == '(' || c == '"') {
                if (runningstr != "") {
                    if (mFastSearch.find(runningstr) == mFastSearch.end()) {
                        mFastSearch.insert_or_assign(runningstr, 1);
                    } else {
                        mFastSearch[runningstr]++;
                    }
                }
                cntfound++;
                if (cntfound == 2) {
                    runningstr = "";
                    break;
                }
            }
            runningstr = runningstr + c;
        }
        if (runningstr != "") {
            if (mFastSearch.find(runningstr) == mFastSearch.end()) {
                mFastSearch.insert_or_assign(runningstr, 1);
            } else {
                mFastSearch[runningstr]++;
            }
        }
    }
}
//----------------------------------------------------------
UniValue PostPocketTransaction(RTransaction& tx, std::string txType) {
    // Check transaction with antibot
    ANTIBOTRESULT ab_result;
    g_antibot->CheckTransactionRIItem(g_addrindex->GetUniValue(tx, tx.pTransaction, tx.pTable), chainActive.Height() + 1, ab_result);
    if (ab_result != ANTIBOTRESULT::Success) {
        throw JSONRPCError(ab_result, txType);
    }

    // Antibot checked -> create transaction in blockchain
    return Sendrawtransaction(tx);
}
//----------------------------------------------------------
UniValue setContent(const JSONRPCRequest& request, RTransaction& tx, ContentType contentType, std::string logType) {
    int64_t txTime = tx->nTime;

    tx.pTransaction["type"] = contentType;

    std::string postlang = "en";
    if (request.params[1].exists("l"))
        postlang = request.params[1]["l"].get_str().length() != 2 ? "en" : request.params[1]["l"].get_str();

    // Posts:
    //   txid - txid of original post transaction
    //   txidEdit - txid of post transaction
    std::string _txid_edit = "";
    if (request.params[1].exists("txidEdit")) _txid_edit = request.params[1]["txidEdit"].get_str();
    if (_txid_edit != "") {
        reindexer::Item _itmP;
        reindexer::Error _err = g_pocketdb->SelectOne(reindexer::Query("Posts").Where("txid", CondEq, _txid_edit), _itmP);
        if (!_err.ok()) throw JSONRPCError(RPC_INVALID_PARAMS, "Invalid txidEdit. Post not found.");
        txTime = _itmP["time"].As<int64_t>();
    }

    std::string _txid_repost = "";
    if (request.params[1].exists("txidRepost")) _txid_repost = request.params[1]["txidRepost"].get_str();
    if (_txid_repost != "") {
        reindexer::Item _itmP;
        reindexer::Error _err = g_pocketdb->SelectOne(reindexer::Query("Posts").Where("txid", CondEq, _txid_repost), _itmP);
        if (!_err.ok()) throw JSONRPCError(RPC_INVALID_PARAMS, "Invalid txidRepost. Post not found.");
    }

    tx.pTransaction["txid"] = _txid_edit == "" ? tx->GetHash().GetHex() : _txid_edit;
    tx.pTransaction["txidEdit"] = _txid_edit == "" ? "" : tx->GetHash().GetHex();
    tx.pTransaction["txidRepost"] = _txid_repost;
    tx.pTransaction["block"] = -1;
    tx.pTransaction["address"] = tx.Address;
    tx.pTransaction["time"] = txTime;
    tx.pTransaction["lang"] = postlang;
    tx.pTransaction["message"] = request.params[1]["m"].get_str();

    if (request.params[1].exists("c")) tx.pTransaction["caption"] = request.params[1]["c"].get_str();
    if (request.params[1].exists("u")) tx.pTransaction["url"] = request.params[1]["u"].get_str();
    if (request.params[1].exists("s")) tx.pTransaction["settings"] = request.params[1]["s"].get_obj().write();

    vector<string> _tags;
    if (request.params[1].exists("t")) {
        UniValue tags = request.params[1]["t"].get_array();
        for (unsigned int idx = 0; idx < tags.size(); idx++) {
            _tags.push_back(tags[idx].get_str());
        }
    }
    tx.pTransaction["tags"] = _tags;


    vector<string> _images;
    if (request.params[1].exists("i")) {
        UniValue images = request.params[1]["i"].get_array();
        for (unsigned int idx = 0; idx < images.size(); idx++) {
            _images.push_back(images[idx].get_str());
        }
    }
    tx.pTransaction["images"] = _images;

    return PostPocketTransaction(tx, logType);
}

UniValue SetShare(const JSONRPCRequest& request, RTransaction& tx) {
    if (request.fHelp)
        throw std::runtime_error("share\n\nCreate new pocketnet post.\n");

    return setContent(request, tx, ContentType::ContentPost, "Share");
}

UniValue SetVideo(const JSONRPCRequest& request, RTransaction& tx) {
    if (request.fHelp)
        throw std::runtime_error("video\n\nCreate new pocketnet video post.\n");

    return setContent(request, tx, ContentType::ContentVideo, "Video");
}

UniValue SetTranslate(const JSONRPCRequest& request, RTransaction& tx) {
    if (request.fHelp)
        throw std::runtime_error("translate\n\nCreate new pocketnet translate post.\n");

    return setContent(request, tx, ContentType::ContentTranslate, "Translate");
}

UniValue SetServerPing(const JSONRPCRequest& request, RTransaction& tx) {
    if (request.fHelp)
        throw std::runtime_error("serverPing\n\nCreate new pocketnet server ping post.\n");

    return setContent(request, tx, ContentType::ContentServerPing, "ServerPing");
}

UniValue SetUpvoteShare(const JSONRPCRequest& request, RTransaction& tx) {
    if (request.fHelp)
        throw std::runtime_error("upvoteShare\n\nCreate new pocketnet like to post.\n");

    tx.pTransaction["txid"] = tx->GetHash().GetHex();
    tx.pTransaction["block"] = -1;
    tx.pTransaction["posttxid"] = request.params[1]["share"].get_str();
    tx.pTransaction["address"] = tx.Address;
    tx.pTransaction["time"] = (int64_t)tx->nTime;

    int _val;
    ParseInt32(request.params[1]["value"].get_str(), &_val);
    tx.pTransaction["value"] = _val;

    return PostPocketTransaction(tx, "UpvoteShare");
}

UniValue SetSubscribe(const JSONRPCRequest& request, RTransaction& tx) {
    if (request.fHelp)
        throw std::runtime_error("subscribe\n\nCreate new pocketnet subscribe.\n");

    tx.pTransaction["txid"] = tx->GetHash().GetHex();
    tx.pTransaction["block"] = -1;
    tx.pTransaction["time"] = (int64_t)tx->nTime;
    tx.pTransaction["address"] = tx.Address;
    tx.pTransaction["address_to"] = request.params[1]["address"].get_str();
    tx.pTransaction["private"] = false;
    tx.pTransaction["unsubscribe"] = false;

    return PostPocketTransaction(tx, "Subscribe");
}

UniValue SetSubscribePrivate(const JSONRPCRequest& request, RTransaction& tx) {
    if (request.fHelp)
        throw std::runtime_error("subscribePrivate\n\nCreate new pocketnet subscribe private.\n");

    tx.pTransaction["txid"] = tx->GetHash().GetHex();
    tx.pTransaction["block"] = -1;
    tx.pTransaction["time"] = (int64_t)tx->nTime;
    tx.pTransaction["address"] = tx.Address;
    tx.pTransaction["address_to"] = request.params[1]["address"].get_str();
    tx.pTransaction["private"] = true;
    tx.pTransaction["unsubscribe"] = false;

    return PostPocketTransaction(tx, "SubscribePrivate");
}

UniValue SetUnsubscribe(const JSONRPCRequest& request, RTransaction& tx) {
    if (request.fHelp)
        throw std::runtime_error("unsubscribe\n\nCreate new pocketnet unsubscribe.\n");

    reindexer::Item _itm;
    reindexer::Error _err = g_pocketdb->SelectOne(
        reindexer::Query("SubscribesView")
            .Where("address", CondEq, tx.Address)
            .Where("address_to", CondEq, request.params[1]["address"].get_str()),
        _itm);
    if (!_err.ok()) throw JSONRPCError(RPC_INVALID_PARAMS, "Invalid transaction");
    tx.pTransaction["private"] = _itm["private"].As<bool>();

    tx.pTransaction["txid"] = tx->GetHash().GetHex();
    tx.pTransaction["block"] = -1;
    tx.pTransaction["time"] = (int64_t)tx->nTime;
    tx.pTransaction["address"] = tx.Address;
    tx.pTransaction["address_to"] = request.params[1]["address"].get_str();
    tx.pTransaction["unsubscribe"] = true;

    return PostPocketTransaction(tx, "Unsubscribe");
}

UniValue SetComplainShare(const JSONRPCRequest& request, RTransaction& tx) {
    if (request.fHelp)
        throw std::runtime_error("complainShare\n\nCreate new pocketnet complain for post.\n");

    tx.pTransaction["txid"] = tx->GetHash().GetHex();
    tx.pTransaction["block"] = -1;
    tx.pTransaction["posttxid"] = request.params[1]["share"].get_str();
    tx.pTransaction["address"] = tx.Address;
    tx.pTransaction["time"] = (int64_t)tx->nTime;

    int _val;
    ParseInt32(request.params[1]["reason"].get_str(), &_val);
    tx.pTransaction["reason"] = _val;

    return PostPocketTransaction(tx, "ComplainShare");
}

UniValue SetBlocking(const JSONRPCRequest& request, RTransaction& tx) {
    if (request.fHelp)
        throw std::runtime_error("blocking\n\nCreate new pocketnet block post.\n");

    tx.pTransaction["txid"] = tx->GetHash().GetHex();
    tx.pTransaction["block"] = -1;
    tx.pTransaction["time"] = (int64_t)tx->nTime;
    tx.pTransaction["address"] = tx.Address;
    tx.pTransaction["address_to"] = request.params[1]["address"].get_str();
    tx.pTransaction["unblocking"] = false;

    return PostPocketTransaction(tx, "Blocking");
}

UniValue SetUnblocking(const JSONRPCRequest& request, RTransaction& tx) {
    if (request.fHelp)
        throw std::runtime_error("unblocking\n\nCreate new pocketnet unblock post.\n");

    // reindexer::Item _itm;
    // reindexer::Error _err = g_pocketdb->SelectOne(
    //     reindexer::Query("BlockingView")
    //         .Where("address", CondEq, address)
    //         .Where("address_to", CondEq, request.params[1]["address"].get_str()),
    //     _itm);

    // if (!_err.ok()) throw JSONRPCError(RPC_INVALID_PARAMS, "Invalid transaction");

    tx.pTransaction["txid"] = tx->GetHash().GetHex();
    tx.pTransaction["block"] = -1;
    tx.pTransaction["time"] = (int64_t)tx->nTime;
    tx.pTransaction["address"] = tx.Address;
    tx.pTransaction["address_to"] = request.params[1]["address"].get_str();
    tx.pTransaction["unblocking"] = true;

    return PostPocketTransaction(tx, "Unblocking");
}

void FillComment(const JSONRPCRequest& request, RTransaction& tx, bool& valid) {
    valid = valid & request.params[1].exists("postid");
    valid = valid & request.params[1].exists("parentid");
    valid = valid & request.params[1].exists("answerid");
    if (!valid) throw JSONRPCError(RPC_INVALID_PARAMS, "Invalid parameters");

    std::string _otxid = tx->GetHash().GetHex();
    if (request.params[1].exists("id")) _otxid = request.params[1]["id"].get_str();
    tx.pTransaction["otxid"] = _otxid;

    tx.pTransaction["txid"] = tx->GetHash().GetHex();
    tx.pTransaction["block"] = -1;
    tx.pTransaction["address"] = tx.Address;
    tx.pTransaction["time"] = (int64_t)tx->nTime;
    tx.pTransaction["last"] = true;
    tx.pTransaction["postid"] = request.params[1]["postid"].get_str();
    tx.pTransaction["parentid"] = request.params[1]["parentid"].get_str();
    tx.pTransaction["answerid"] = request.params[1]["answerid"].get_str();
}

UniValue SetComment(const JSONRPCRequest& request, RTransaction& tx) {
    if (request.fHelp)
        throw std::runtime_error("comment\n\nCreate new pocketnet comment for post.\n");

    bool valid = true;
    valid = valid & request.params[1].exists("msg");

    FillComment(request, tx, valid);
    tx.pTransaction["msg"] = request.params[1]["msg"].get_str();

    return PostPocketTransaction(tx, "Comment");
}

UniValue SetCommentEdit(const JSONRPCRequest& request, RTransaction& tx) {
    if (request.fHelp)
        throw std::runtime_error("commentEdit\n\nCreate new pocketnet comment edit.\n");

    bool valid = true;
    valid = valid & request.params[1].exists("id");
    valid = valid & request.params[1].exists("msg");

    FillComment(request, tx, valid);
    tx.pTransaction["msg"] = request.params[1]["msg"].get_str();

    return PostPocketTransaction(tx, "CommentDelete");
}

UniValue SetCommentDelete(const JSONRPCRequest& request, RTransaction& tx) {
    if (request.fHelp)
        throw std::runtime_error("commentDelete\n\nDelete pocketnet comment.\n");

    bool valid = true;
    valid = valid & request.params[1].exists("id");

    FillComment(request, tx, valid);
    tx.pTransaction["msg"] = "";

    return PostPocketTransaction(tx, "CommentDelete");
}

UniValue SetCommentScore(const JSONRPCRequest& request, RTransaction& tx) {
    if (request.fHelp)
        throw std::runtime_error("cScore\nCreate new pocketnet comment like.\n");

    bool valid = true;
    valid = valid & request.params[1].exists("commentid");
    valid = valid & request.params[1].exists("value");
    if (!valid) throw JSONRPCError(RPC_INVALID_PARAMS, "Invalid parameters");

    tx.pTransaction["txid"] = tx->GetHash().GetHex();
    tx.pTransaction["address"] = tx.Address;
    tx.pTransaction["time"] = (int64_t)tx->nTime;
    tx.pTransaction["block"] = -1;

    tx.pTransaction["commentid"] = request.params[1]["commentid"].get_str();

    int _val;
    ParseInt32(request.params[1]["value"].get_str(), &_val);
    tx.pTransaction["value"] = _val;

    return PostPocketTransaction(tx, "UpvoteComment");
}

UniValue setAccount(const JSONRPCRequest& request, RTransaction& tx, AccountType accountType, std::string logType) {
    tx.pTransaction["gender"] = accountType;

    tx.pTransaction["txid"] = tx->GetHash().GetHex();
    tx.pTransaction["block"] = -1;
    tx.pTransaction["id"] = -1;
    tx.pTransaction["address"] = tx.Address;
    tx.pTransaction["name"] = request.params[1]["n"].get_str();
    tx.pTransaction["avatar"] = request.params[1]["i"].get_str();
    tx.pTransaction["lang"] = request.params[1]["l"].get_str() == "" ? "en" : request.params[1]["l"].get_str();

    tx.pTransaction["time"] = (int64_t)tx->nTime;
    tx.pTransaction["regdate"] = (int64_t)tx->nTime;
    tx.pTransaction["referrer"] = "";

    reindexer::Item user_cur;
    if (g_pocketdb->SelectOne(reindexer::Query("UsersView").Where("address", CondEq, tx.Address), user_cur).ok()) {
        tx.pTransaction["regdate"] = user_cur["regdate"].As<int64_t>();
    } else if (request.params[1].exists("r")) {
        tx.pTransaction["referrer"] = request.params[1]["r"].get_str();
    }

    if (request.params[1].exists("a")) tx.pTransaction["about"] = request.params[1]["a"].get_str();
    if (request.params[1].exists("s")) tx.pTransaction["url"] = request.params[1]["s"].get_str();
    if (request.params[1].exists("b")) tx.pTransaction["donations"] = request.params[1]["b"].get_str();
    if (request.params[1].exists("k")) tx.pTransaction["pubkey"] = request.params[1]["k"].get_str();

    return PostPocketTransaction(tx, logType);
}

UniValue SetVideoServer(const JSONRPCRequest& request, RTransaction& tx) {
    if (request.fHelp)
        throw std::runtime_error("videoServer\n\nCreate new pocketnet video server registration record.\n");

    return setAccount(request, tx, AccountType::AccountVideoServer, "VideoServer");
}

UniValue SetMessageServer(const JSONRPCRequest& request, RTransaction& tx) {
    if (request.fHelp)
        throw std::runtime_error("messageServer\n\nCreate new pocketnet messaging server registration record.\n");

    return setAccount(request, tx, AccountType::AccountMessageServer, "MessageServer");
}

UniValue SetUserInfo(const JSONRPCRequest& request, RTransaction& tx) {
    if (request.fHelp)
        throw std::runtime_error("userInfo\n\nCreate new pocketnet user.\n");

    return setAccount(request, tx, AccountType::AccountUser, "Account");
}

//----------------------------------------------------------
UniValue sendrawtransactionwithmessage(const JSONRPCRequest& request)
{
    if (request.fHelp)
        throw std::runtime_error("sendrawtransactionwithmessage\n\nCreate new pocketnet transaction.\n");

    RPCTypeCheck(request.params, {UniValue::VSTR, UniValue::VOBJ, UniValue::VSTR});

    std::string address;
    CMutableTransaction mTx;
    if (!DecodeHexTx(mTx, request.params[0].get_str()))
        throw JSONRPCError(RPC_DESERIALIZATION_ERROR, "TX decode failed");

    RTransaction tx(mTx);

    if (!g_addrindex->GetPocketnetTXType(tx, tx.TxType, tx.pTable))
        throw JSONRPCError(RPC_DESERIALIZATION_ERROR, "Pocket tx type bad");

    if (!GetInputAddress(tx->vin[0].prevout.hash, tx->vin[0].prevout.n, address)) {
        throw JSONRPCError(RPC_INVALID_PARAMS, "Invalid address");
    }

    tx.Address = address;
    tx.pTransaction = g_pocketdb->DB()->NewItem(tx.pTable);

    std::string mesType = request.params[2].get_str();
    if (mesType == "share") return SetShare(request, tx);
    if (mesType == "upvoteShare") return SetUpvoteShare(request, tx);
    if (mesType == "subscribe") return SetSubscribe(request, tx);
    if (mesType == "subscribePrivate") return SetSubscribePrivate(request, tx);
    if (mesType == "unsubscribe") return SetUnsubscribe(request, tx);
    if (mesType == "userInfo") return SetUserInfo(request, tx);
    if (mesType == "complainShare") return SetComplainShare(request, tx);
    if (mesType == "blocking") return SetBlocking(request, tx);
    if (mesType == "unblocking") return SetUnblocking(request, tx);
    if (mesType == "comment") return SetComment(request, tx);
    if (mesType == "commentEdit") return SetCommentEdit(request, tx);
    if (mesType == "commentDelete") return SetCommentDelete(request, tx);
    if (mesType == "cScore") return SetCommentScore(request, tx);
    if (mesType == "video") return SetVideo(request, tx);

    // TODO (brangr): only for pre-release tests
    if (Params().NetworkIDString() == CBaseChainParams::TESTNET) {
        if (mesType == "serverPing") return SetServerPing(request, tx);
        if (mesType == "translate") return SetTranslate(request, tx);
        if (mesType == "videoServer") return SetVideoServer(request, tx);
        if (mesType == "messageServer") return SetMessageServer(request, tx);
    }

    throw JSONRPCError(RPC_INVALID_PARAMS, "Invalid transaction type");
}
//----------------------------------------------------------
UniValue getrawtransactionwithmessage(const JSONRPCRequest& request)
{
    if (request.fHelp)
        throw std::runtime_error(
            "getrawtransactionwithmessage\n"
            "\nReturn Pocketnet posts.\n");

    UniValue a(UniValue::VARR);

    reindexer::QueryResults queryRes;
    reindexer::Error err;

    int64_t resultStart = 0;
    int resultCount = 50;

    std::string address_from = "";
    if (request.params.size() > 0 && request.params[0].get_str().length() > 0) {
        address_from = request.params[0].get_str();
        if (address_from.length() < 34)
            throw JSONRPCError(RPC_INVALID_PARAMS, "Invalid address in HEX transaction");
    }

    std::string address_to = "";
    if (request.params.size() > 1 && request.params[1].get_str().length() > 0) {
        address_to = request.params[1].get_str();
        if (address_to != "1" && address_to.length() < 34)
            throw JSONRPCError(RPC_INVALID_PARAMS, "Invalid address in HEX transaction");
    }

    if (request.params.size() > 2) {
        reindexer::Item postItm;
        reindexer::Error errT = g_pocketdb->SelectOne(
            reindexer::Query("Posts").Where("txid", CondEq, request.params[2].get_str()),
            postItm);
        if (errT.ok()) resultStart = postItm["time"].As<int64_t>();
    }

    if (request.params.size() > 3) {
        resultCount = request.params[3].get_int();
    }

    std::string lang = "";
    if (request.params.size() > 4) {
        lang = request.params[4].get_str();
    }

    std::vector<string> tags;
    if (request.params.size() > 5) {
        if (request.params[5].isStr()) {
            std::string tag = boost::trim_copy(request.params[4].get_str());
            if (!tag.empty()) {
                tags.push_back(tag);
            }
        } else if (request.params[5].isArray()) {
            UniValue tgs = request.params[5].get_array();
            if (tgs.size() > 1000) {
                throw JSONRPCError(RPC_INVALID_PARAMS, "Too large array tags");
            }
            if(tgs.size() > 0) {
                for (unsigned int idx = 0; idx < tgs.size(); idx++) {
                    std::string tag = boost::trim_copy(tgs[idx].get_str());
                    if (!tag.empty()) {
                        tags.push_back(tag);
                    }
                }
            }
        }
    }

    // TODO (brangr): contentTypes
    /*
    std::vector<int> contentTypes = { 0, 1, 2, 4, 5 };
    if (request.params.size() > 6) {
        if (request.params[6].isNum()) {
            contentTypes = { request.params[6].get_int() };
        } else if (request.params[6].isArray()) {
            contentTypes.clear();
            UniValue tps = request.params[6].get_array();
            for (unsigned int idx = 0; idx < tps.size(); idx++) {
                contentTypes.push_back(tps[idx].get_int());
            }
        }
    }
    */

    vector<string> addrsblock;
    if (address_from != "" && (address_to == "" || address_to == "1")) {
        reindexer::QueryResults queryResBlocking;
        err = g_pocketdb->DB()->Select(reindexer::Query("BlockingView").Where("address", CondEq, address_from), queryResBlocking);

        for (auto it : queryResBlocking) {
            reindexer::Item itm(it.GetItem());
            addrsblock.push_back(itm["address_to"].As<string>());
        }
    }

    // Do not show posts from users with reputation < Limit::bad_reputation
    if (address_to == "") {
        int64_t _bad_reputation_limit = GetActualLimit(Limit::bad_reputation, chainActive.Height());
        reindexer::QueryResults queryResBadReputation;
        err = g_pocketdb->DB()->Select(reindexer::Query("UsersView").Where("reputation", CondLe, _bad_reputation_limit), queryResBadReputation);

        for (auto it : queryResBadReputation) {
            reindexer::Item itm(it.GetItem());
            addrsblock.push_back(itm["address"].As<string>());
        }
    }

    vector<string> addrs;
    if (address_to != "") {
        if (address_to == "1") {
            if (address_from.length() < 34)
                throw JSONRPCError(RPC_INVALID_PARAMS, "Invalid address in HEX transaction");
            reindexer::QueryResults queryResSubscribes;
            err = g_pocketdb->DB()->Select(reindexer::Query("SubscribesView").Where("address", CondEq, address_from), queryResSubscribes);

            for (auto it : queryResSubscribes) {
                reindexer::Item itm(it.GetItem());
                addrs.push_back(itm["address_to"].As<string>());
            }
        } else {
            addrs.push_back(address_to);
        }
        //err = g_pocketdb->DB()->Select(
        //    reindexer::Query("Posts" /*, 0, resultCount*/).Where("tags", (tags[0] != "" ? CondSet : CondEmpty), tags).Where("lang", (lang == "" ? CondGe : CondEq), lang).Where("address", CondSet, addrs).Not().Where("address", CondSet, addrsblock).Where("time", ((resultCount > 0 && resultStart > 0) ? CondLt : CondGt), resultStart).Where("time", CondLe, GetAdjustedTime()).Sort("time", (resultCount > 0 ? true : false)),
        //    queryRes);
    } else {
        //err = g_pocketdb->DB()->Select(
        //    reindexer::Query("Posts" /*, 0, resultCount*/).Where("tags", (tags[0] != "" ? CondSet : CondGe), tags).Where("lang", (lang == "" ? CondGe : CondEq), lang).Where("txidRepost", CondEq, "").Not().Where("address", CondSet, addrsblock).Where("time", ((resultCount > 0 && resultStart > 0) ? CondLt : CondGt), resultStart).Where("time", CondLe, GetAdjustedTime()).Sort("time", (resultCount > 0 ? true : false)),
        //    queryRes);
    }

    reindexer::Query query;
    query = reindexer::Query("Posts");

    query = query.Not().Where("address", CondSet, addrsblock);
    query = query.Where("time", CondLe, GetAdjustedTime());
    // TODO (brangr): contentTypes
    //query = query.Where("type", CondSet, contentTypes);

    if (resultCount > 0 && resultStart > 0) {
        query = query.Where("time", CondLt, resultStart);
    } else {
        query = query.Where("time", CondGt, resultStart);
    }
    if (address_to != "") {
        query = query.Where("address", CondSet, addrs);
    } else {
        query = query.Where("txidRepost", CondEq, "");
    }
    if (!lang.empty()) {
        query = query.Where("lang", CondEq, lang);
    }
    if (!tags.empty()) {
        query = query.Where("tags", CondSet, tags);
    }
    if (resultCount > 0) {
        query = query.Sort("time", true);
    } else {
        query = query.Sort("time", false);
    }
    err = g_pocketdb->DB()->Select(query, queryRes);

    int iQuery = 0;
    reindexer::QueryResults::Iterator it = queryRes.begin();
    while (resultCount > 0 && it != queryRes.end()) {
        reindexer::Item itm(it.GetItem());

        reindexer::QueryResults queryResComp;
        err = g_pocketdb->DB()->Select(reindexer::Query("Complains").Where("posttxid", CondEq, itm["txid"].As<string>()), queryResComp);
        reindexer::QueryResults queryResUpv;
        err = g_pocketdb->DB()->Select(reindexer::Query("Scores").Where("posttxid", CondEq, itm["txid"].As<string>()).Where("value", CondGt, 3), queryResUpv);

        if (queryResComp.Count() <= 7 || queryResComp.Count() / (queryResUpv.Count() == 0 ? 1 : queryResUpv.Count() == 0 ? 1 : queryResUpv.Count()) <= 0.1) {
            a.push_back(getPostData(itm, address_from));
            resultCount -= 1;
        }
        iQuery += 1;
        it = queryRes[iQuery];
    }

    return a;
}
UniValue getrawtransactionwithmessage2(const JSONRPCRequest& request) { return getrawtransactionwithmessage(request); }
//----------------------------------------------------------
UniValue getrawtransactionwithmessagebyid(const JSONRPCRequest& request)
{
    if (request.fHelp)
        throw std::runtime_error(
            "getrawtransactionwithmessagebyid\n"
            "\nReturn Pocketnet posts.\n");

    if (request.params.size() < 1)
        throw JSONRPCError(RPC_INVALID_PARAMETER, "There is no TxId");

    vector<string> TxIds;
    if (request.params[0].isArray()) {
        UniValue txid = request.params[0].get_array();
        for (unsigned int idx = 0; idx < txid.size(); idx++) {
            TxIds.push_back(txid[idx].get_str());
        }
    } else if (request.params[0].isStr()) {
        TxIds.push_back(request.params[0].get_str());
    } else {
        throw JSONRPCError(RPC_INVALID_PARAMS, "Invalid inputs params");
    }

    std::string address = "";
    if (request.params.size() > 1 && request.params[1].get_str().length() > 0) {
        address = request.params[1].get_str();
        if (address.length() < 34)
            throw JSONRPCError(RPC_INVALID_PARAMS, "Invalid address in HEX transaction");
    }

    UniValue a(UniValue::VARR);

    reindexer::QueryResults queryRes;
    reindexer::Error err;

    err = g_pocketdb->DB()->Select(
        reindexer::Query("Posts").Where("txid", CondSet, TxIds).Sort("time", true),
        queryRes);

    for (auto it : queryRes) {
        reindexer::Item itm(it.GetItem());
        a.push_back(getPostData(itm, address));
    }
    return a;
}
UniValue getrawtransactionwithmessagebyid2(const JSONRPCRequest& request) { return getrawtransactionwithmessagebyid(request); }
//----------------------------------------------------------
UniValue getuserprofile(const JSONRPCRequest& request)
{
    if (request.fHelp)
        throw std::runtime_error(
            "getuserprofile \"address\" ( shortForm )\n"
            "\nReturn Pocketnet user profile.\n");

    if (request.params.size() < 1)
        throw JSONRPCError(RPC_INVALID_PARAMETER, "There is no address");

    std::vector<string> addresses;
    if (request.params[0].isStr())
        addresses.push_back(request.params[0].get_str());
    else if (request.params[0].isArray()) {
        UniValue addr = request.params[0].get_array();
        for (unsigned int idx = 0; idx < addr.size(); idx++) {
            addresses.push_back(addr[idx].get_str());
        }
    } else
        throw JSONRPCError(RPC_INVALID_PARAMS, "Invalid inputs params");

    // Short profile form is: address, b, i, name
    bool shortForm = false;
    if (request.params.size() >= 2) {
        shortForm = request.params[1].get_str() == "1";
    }

    UniValue aResult(UniValue::VARR);

    std::map<std::string, UniValue> profiles = getUsersProfiles(addresses, shortForm);
    for (auto& p : profiles) {
        aResult.push_back(p.second);
    }

    return aResult;
}
//----------------------------------------------------------
UniValue getmissedinfo(const JSONRPCRequest& request)
{
    if (request.fHelp)
        throw std::runtime_error(
            "getmissedinfo \"address\" block_number\n"
            "\nGet missed info.\n");

    if (request.params.size() < 1)
        throw JSONRPCError(RPC_INVALID_PARAMETER, "There is no address");
    if (request.params.size() < 2)
        throw JSONRPCError(RPC_INVALID_PARAMETER, "There is no block number");

    std::string address = "";
    int blockNumber = 0;
    int cntResult = 30;

    if (!request.params[0].isNull()) {
        RPCTypeCheckArgument(request.params[0], UniValue::VSTR);
        CTxDestination dest = DecodeDestination(request.params[0].get_str());

        if (!IsValidDestination(dest)) {
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, std::string("Invalid address: ") + request.params[0].get_str());
        }

        address = request.params[0].get_str();
    }

    if (!request.params[1].isNull()) {
        blockNumber = request.params[1].isNum() ? request.params[1].get_int() : std::stoi(request.params[1].get_str());
    }

    if (!request.params[2].isNull()) {
        cntResult = request.params[2].isNum() ? request.params[2].get_int() : std::stoi(request.params[2].get_str());
    }

    UniValue a(UniValue::VARR);

    reindexer::QueryResults posts;
    g_pocketdb->DB()->Select(reindexer::Query("Posts").Where("block", CondGt, blockNumber), posts);
    std::map<std::string, int> postsCntLang;
    for (auto& p : posts) {
        reindexer::Item postItm = p.GetItem();

        UniValue t(UniValue::VARR);
        std::string lang = postItm["lang"].As<string>();
        if (postsCntLang.count(lang) == 0)
            postsCntLang[lang] = 1;
        else
            postsCntLang[lang] += 1;
    }
    UniValue cntpostslang(UniValue::VOBJ);
    for (std::map<std::string, int>::iterator itl = postsCntLang.begin(); itl != postsCntLang.end(); ++itl) {
        cntpostslang.pushKV(itl->first, itl->second);
    }

    UniValue msg(UniValue::VOBJ);
    msg.pushKV("block", (int)chainActive.Height());
    msg.pushKV("cntposts", (int)posts.Count());
    msg.pushKV("cntpostslang", cntpostslang);
    a.push_back(msg);

    std::string addrespocketnet = "PEj7QNjKdDPqE9kMDRboKoCtp8V6vZeZPd";
    reindexer::QueryResults postspocketnet;
    g_pocketdb->DB()->Select(reindexer::Query("Posts").Where("block", CondGt, blockNumber).Where("address", CondEq, addrespocketnet), postspocketnet);
    for (auto it : postspocketnet) {
        reindexer::Item itm(it.GetItem());
        UniValue msg(UniValue::VOBJ);
        msg.pushKV("msg", "sharepocketnet");
        msg.pushKV("txid", itm["txid"].As<string>());
        msg.pushKV("time", itm["time"].As<string>());
        msg.pushKV("nblock", itm["block"].As<int>());
        a.push_back(msg);
    }

    reindexer::QueryResults subscribespriv;
    g_pocketdb->DB()->Select(reindexer::Query("SubscribesView").Where("address", CondEq, address).Where("private", CondEq, "true"), subscribespriv);
    for (auto it : subscribespriv) {
        reindexer::Item itm(it.GetItem());

        reindexer::QueryResults postfromprivate;
        g_pocketdb->Select(reindexer::Query("Posts", 0, 1).Where("block", CondGt, blockNumber).Where("address", CondEq, itm["address_to"].As<string>()).Sort("time", true).ReqTotal(), postfromprivate);
        if (postfromprivate.totalCount > 0) {
            reindexer::Item itm2(postfromprivate[0].GetItem());

            UniValue msg(UniValue::VOBJ);
            msg.pushKV("msg", "event");
            msg.pushKV("mesType", "postfromprivate");
            msg.pushKV("txid", itm2["txid"].As<string>());
            msg.pushKV("addrFrom", itm2["address"].As<string>());
            msg.pushKV("postsCnt", postfromprivate.totalCount);
            msg.pushKV("time", itm2["time"].As<string>());
            msg.pushKV("nblock", itm2["block"].As<int>());
            a.push_back(msg);
        }
    }

    reindexer::QueryResults reposts;
    g_pocketdb->DB()->Select(reindexer::Query("Posts").Where("block", CondGt, blockNumber).InnerJoin("txidRepost", "txid", CondEq, reindexer::Query("Posts").Where("address", CondEq, address)), reposts);
    for (auto it : reposts) {
        reindexer::Item itm(it.GetItem());
        UniValue msg(UniValue::VOBJ);
        msg.pushKV("msg", "reshare");
        msg.pushKV("txid", itm["txid"].As<string>());
        msg.pushKV("txidRepost", itm["txidRepost"].As<string>());
        msg.pushKV("addrFrom", itm["address"].As<string>());
        msg.pushKV("time", itm["time"].As<string>());
        msg.pushKV("nblock", itm["block"].As<int>());

        a.push_back(msg);
    }

    reindexer::QueryResults subscribers;
    g_pocketdb->DB()->Select(reindexer::Query("SubscribesView").Where("address_to", CondEq, address).Where("block", CondGt, blockNumber).Sort("time", true).Limit(cntResult), subscribers);
    for (auto it : subscribers) {
        reindexer::Item itm(it.GetItem());

        reindexer::QueryResults resubscribe;
        reindexer::Error errResubscr = g_pocketdb->DB()->Select(reindexer::Query("Subscribes", 0, 1).Where("address", CondEq, itm["address"].As<string>()).Where("address_to", CondEq, itm["address_to"].As<string>()).Where("time", CondLt, itm["time"].As<int64_t>()).Sort("time", true), resubscribe);
        if (errResubscr.ok() && resubscribe.Count() > 0) {
            reindexer::Item reitm(resubscribe[0].GetItem());
            if (reitm["unsubscribe"].As<bool>()) {
                UniValue msg(UniValue::VOBJ);
                msg.pushKV("addr", itm["address_to"].As<string>());
                msg.pushKV("addrFrom", itm["address"].As<string>());
                msg.pushKV("msg", "event");
                msg.pushKV("txid", itm["txid"].As<string>());
                msg.pushKV("time", itm["time"].As<string>());
                msg.pushKV("mesType", "subscribe");
                msg.pushKV("nblock", itm["block"].As<int>());

                a.push_back(msg);
            }
        }
    }

    reindexer::QueryResults scores;
    g_pocketdb->DB()->Select(reindexer::Query("Scores").Where("block", CondGt, blockNumber).InnerJoin("posttxid", "txid", CondEq, reindexer::Query("Posts").Where("address", CondEq, address)).Sort("time", true).Limit(cntResult), scores);
    for (auto it : scores) {
        reindexer::Item itm(it.GetItem());
        UniValue msg(UniValue::VOBJ);
        msg.pushKV("addr", address);
        msg.pushKV("addrFrom", itm["address"].As<string>());
        msg.pushKV("msg", "event");
        msg.pushKV("txid", itm["txid"].As<string>());
        msg.pushKV("time", itm["time"].As<string>());
        msg.pushKV("posttxid", itm["posttxid"].As<string>());
        msg.pushKV("upvoteVal", itm["value"].As<int>());
        msg.pushKV("mesType", "upvoteShare");
        msg.pushKV("nblock", itm["block"].As<int>());
        a.push_back(msg);
    }

    reindexer::QueryResults commentScores;
    g_pocketdb->DB()->Select(
        reindexer::Query("CommentScores")
            .Where("block", CondGt, blockNumber)
            .InnerJoin("commentid", "txid", CondEq, reindexer::Query("Comment").Where("address", CondEq, address))
            .Sort("time", true)
            .Limit(cntResult),
        commentScores);
    for (auto it : commentScores) {
        reindexer::Item itm(it.GetItem());
        UniValue msg(UniValue::VOBJ);
        msg.pushKV("addr", address);
        msg.pushKV("addrFrom", itm["address"].As<string>());
        msg.pushKV("msg", "event");
        msg.pushKV("txid", itm["txid"].As<string>());
        msg.pushKV("time", itm["time"].As<string>());
        msg.pushKV("commentid", itm["commentid"].As<string>());
        msg.pushKV("upvoteVal", itm["value"].As<int>());
        msg.pushKV("mesType", "cScore");
        msg.pushKV("nblock", itm["block"].As<int>());
        a.push_back(msg);
    }

    std::vector<std::string> txSent;
    reindexer::QueryResults transactions;
    g_pocketdb->DB()->Select(reindexer::Query("UTXO").Where("address", CondEq, address).Where("block", CondGt, blockNumber).Sort("time", true).Limit(cntResult), transactions);
    for (auto it : transactions) {
        reindexer::Item itm(it.GetItem());

        // Double transaction notify not allowed
        if (std::find(txSent.begin(), txSent.end(), itm["txid"].As<string>()) != txSent.end()) continue;

        UniValue msg(UniValue::VOBJ);
        msg.pushKV("addr", itm["address"].As<string>());
        msg.pushKV("msg", "transaction");
        msg.pushKV("txid", itm["txid"].As<string>());
        msg.pushKV("time", itm["time"].As<string>());
        msg.pushKV("amount", itm["amount"].As<int64_t>());
        msg.pushKV("nblock", itm["block"].As<int>());

        uint256 hash = ParseHashV(itm["txid"].As<string>(), "txid");
        CTransactionRef tx;
        uint256 hash_block;
        CBlockIndex* blockindex = nullptr;
        if (GetTransaction(hash, tx, Params().GetConsensus(), hash_block, true, blockindex)) {
            const CTxOut& txout = tx->vout[itm["txout"].As<int>()];
            std::string optype = "";
            if (txout.scriptPubKey[0] == OP_RETURN) {
                std::string asmstr = ScriptToAsmStr(txout.scriptPubKey);
                std::vector<std::string> spl;
                boost::split(spl, asmstr, boost::is_any_of("\t "));
                if (spl.size() == 3) {
                    if (spl[1] == OR_POST || spl[1] == OR_POSTEDIT)
                        optype = "share";
                    else if (spl[1] == OR_VIDEO)
                        optype = "video";
                    else if (spl[1] == OR_SCORE)
                        optype = "upvoteShare";
                    else if (spl[1] == OR_SUBSCRIBE)
                        optype = "subscribe";
                    else if (spl[1] == OR_SUBSCRIBEPRIVATE)
                        optype = "subscribePrivate";
                    else if (spl[1] == OR_USERINFO)
                        optype = "userInfo";
                    else if (spl[1] == OR_UNSUBSCRIBE)
                        optype = "unsubscribe";
                    else if (spl[1] == OR_COMMENT)
                        optype = "comment";
                    else if (spl[1] == OR_COMMENT_EDIT)
                        optype = "commentEdit";
                    else if (spl[1] == OR_COMMENT_DELETE)
                        optype = "commentDelete";
                    else if (spl[1] == OR_COMMENT_SCORE)
                        optype = "cScore";
                }
            }

            if (optype != "") msg.pushKV("type", optype);

            UniValue txinfo(UniValue::VOBJ);
            TxToJSON(*tx, hash_block, txinfo);
            msg.pushKV("txinfo", txinfo);
        }

        a.push_back(msg);
    }

    vector<string> answerpostids;
    reindexer::QueryResults commentsAnswer;
    g_pocketdb->DB()->Select(
        reindexer::Query("Comment")
            .Where("block", CondGt, blockNumber)
            .Where("last", CondEq, true)
            .InnerJoin("answerid", "otxid", CondEq, reindexer::Query("Comment").Where("address", CondEq, address).Where("last", CondEq, true))
            .Sort("time", true)
            .Limit(cntResult),
        commentsAnswer);

    for (auto it : commentsAnswer) {
        reindexer::Item itm(it.GetItem());
        if (address != itm["address"].As<string>()) {
            if (itm["msg"].As<string>() == "") continue;
            if (itm["otxid"].As<string>() != itm["txid"].As<string>()) continue;

            UniValue msg(UniValue::VOBJ);
            msg.pushKV("addr", address);
            msg.pushKV("addrFrom", itm["address"].As<string>());
            msg.pushKV("nblock", itm["block"].As<int>());
            msg.pushKV("msg", "comment");
            msg.pushKV("mesType", "answer");
            msg.pushKV("txid", itm["otxid"].As<string>());
            msg.pushKV("posttxid", itm["postid"].As<string>());
            msg.pushKV("reason", "answer");
            msg.pushKV("time", itm["time"].As<string>());
            if (itm["parentid"].As<string>() != "") msg.pushKV("parentid", itm["parentid"].As<string>());
            if (itm["answerid"].As<string>() != "") msg.pushKV("answerid", itm["answerid"].As<string>());

            a.push_back(msg);

            answerpostids.push_back(itm["postid"].As<string>());
        }
    }

    reindexer::QueryResults commentsPost;
    g_pocketdb->DB()->Select(reindexer::Query("Comment").Where("block", CondGt, blockNumber).Where("last", CondEq, true).InnerJoin("postid", "txid", CondEq, reindexer::Query("Posts").Where("address", CondEq, address).Not().Where("txid", CondSet, answerpostids)).Sort("time", true).Limit(cntResult), commentsPost);

    for (auto it : commentsPost) {
        reindexer::Item itm(it.GetItem());
        if (address != itm["address"].As<string>()) {
            if (itm["msg"].As<string>() == "") continue;
            if (itm["otxid"].As<string>() != itm["txid"].As<string>()) continue;

            UniValue msg(UniValue::VOBJ);
            msg.pushKV("addr", address);
            msg.pushKV("addrFrom", itm["address"].As<string>());
            msg.pushKV("nblock", itm["block"].As<int>());
            msg.pushKV("msg", "comment");
            msg.pushKV("mesType", "post");
            msg.pushKV("txid", itm["otxid"].As<string>());
            msg.pushKV("posttxid", itm["postid"].As<string>());
            msg.pushKV("reason", "post");
            msg.pushKV("time", itm["time"].As<string>());
            if (itm["parentid"].As<string>() != "") msg.pushKV("parentid", itm["parentid"].As<string>());
            if (itm["answerid"].As<string>() != "") msg.pushKV("answerid", itm["answerid"].As<string>());

            a.push_back(msg);
        }
    }

    reindexer::QueryResults likedPosts;
    g_pocketdb->DB()->Select(reindexer::Query("Post"), likedPosts);

    return a;
}
UniValue getmissedinfo2(const JSONRPCRequest& request) { return getmissedinfo(request); }
//----------------------------------------------------------
UniValue txunspent(const JSONRPCRequest& request)
{
    if (request.fHelp || request.params.size() < 1) {
        throw std::runtime_error(
            "txunspent ( minconf maxconf  [\"addresses\",...] [include_unsafe] [query_options])\n"
            "\nReturns array of unspent transaction outputs\n"
            "with between minconf and maxconf (inclusive) confirmations.\n"
            "Optionally filter to only include txouts paid to specified addresses.\n"
            "\nArguments:\n"
            "1. \"addresses\"      (string) A json array of pocketcoin addresses to filter\n"
            "    [\n"
            "      \"address\"     (string) pocketcoin address\n"
            "      ,...\n"
            "    ]\n"
            "2. minconf          (numeric, optional, default=1) The minimum confirmations to filter\n"
            "3. maxconf          (numeric, optional, default=9999999) The maximum confirmations to filter\n"
            "4. include_unsafe (bool, optional, default=true) Include outputs that are not safe to spend\n"
            "                  See description of \"safe\" attribute below.\n"
            "5. query_options    (json, optional) JSON with query options\n"
            "    {\n"
            "      \"minimumAmount\"    (numeric or string, default=0) Minimum value of each UTXO in " +
            CURRENCY_UNIT + "\n"
                            "      \"maximumAmount\"    (numeric or string, default=unlimited) Maximum value of each UTXO in " +
            CURRENCY_UNIT + "\n"
                            "      \"maximumCount\"     (numeric or string, default=unlimited) Maximum number of UTXOs\n"
                            "      \"minimumSumAmount\" (numeric or string, default=unlimited) Minimum sum value of all UTXOs in " +
            CURRENCY_UNIT + "\n"
                            "    }\n"
                            "\nResult\n"
                            "[                   (array of json object)\n"
                            "  {\n"
                            "    \"txid\" : \"txid\",          (string) the transaction id \n"
                            "    \"vout\" : n,               (numeric) the vout value\n"
                            "    \"address\" : \"address\",    (string) the pocketcoin address\n"
                            "    \"label\" : \"label\",        (string) The associated label, or \"\" for the default label\n"
                            "    \"scriptPubKey\" : \"key\",   (string) the script key\n"
                            "    \"amount\" : x.xxx,         (numeric) the transaction output amount in " +
            CURRENCY_UNIT + "\n"
                            "    \"confirmations\" : n,      (numeric) The number of confirmations\n"
                            "    \"redeemScript\" : n        (string) The redeemScript if scriptPubKey is P2SH\n"
                            "    \"spendable\" : xxx,        (bool) Whether we have the private keys to spend this output\n"
                            "    \"solvable\" : xxx,         (bool) Whether we know how to spend this output, ignoring the lack of keys\n"
                            "    \"safe\" : xxx              (bool) Whether this output is considered safe to spend. Unconfirmed transactions\n"
                            "                              from outside keys and unconfirmed replacement transactions are considered unsafe\n"
                            "                              and are not eligible for spending by fundrawtransaction and sendtoaddress.\n"
                            "  }\n"
                            "  ,...\n"
                            "]\n"

                            "\nExamples\n" +
            HelpExampleCli("txunspent", "") +
            HelpExampleCli("txunspent", "\"[\\\"1PGFqEzfmQch1gKD3ra4k18PNj3tTUUSqg\\\",\\\"1LtvqCaApEdUGFkpKMM4MstjcaL4dKg8SP\\\"]\" 6 9999999") +
            HelpExampleRpc("txunspent", "\"[\\\"1PGFqEzfmQch1gKD3ra4k18PNj3tTUUSqg\\\",\\\"1LtvqCaApEdUGFkpKMM4MstjcaL4dKg8SP\\\"]\" 6 9999999") +
            HelpExampleCli("txunspent", "'[]' 6 9999999 true '{ \"minimumAmount\": 0.005 }'") +
            HelpExampleRpc("txunspent", "[], 6, 9999999, true, { \"minimumAmount\": 0.005 } "));
    }

    std::vector<std::string> destinations;
    if (request.params.size() > 0) {
        RPCTypeCheckArgument(request.params[0], UniValue::VARR);
        UniValue inputs = request.params[0].get_array();
        for (unsigned int idx = 0; idx < inputs.size(); idx++) {
            const UniValue& input = inputs[idx];
            CTxDestination dest = DecodeDestination(input.get_str());

            if (!IsValidDestination(dest)) {
                throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, std::string("Invalid Pocketcoin address: ") + input.get_str());
            }

            if (std::find(destinations.begin(), destinations.end(), input.get_str()) == destinations.end()) {
                destinations.push_back(input.get_str());
            }
        }
    }

    int nMinDepth = 1;
    if (request.params.size() > 1) {
        RPCTypeCheckArgument(request.params[1], UniValue::VNUM);
        nMinDepth = request.params[1].get_int();
    }

    int nMaxDepth = 9999999;
    if (request.params.size() > 2) {
        RPCTypeCheckArgument(request.params[2], UniValue::VNUM);
        nMaxDepth = request.params[2].get_int();
    }

    bool include_unsafe = true;
    if (request.params.size() > 3) {
        RPCTypeCheckArgument(request.params[3], UniValue::VBOOL);
        include_unsafe = request.params[3].get_bool();
    }

    CAmount nMinimumAmount = 0;
    CAmount nMaximumAmount = MAX_MONEY;
    CAmount nMinimumSumAmount = MAX_MONEY;
    uint64_t nMaximumCount = UINTMAX_MAX;

    if (request.params.size() > 4) {
        const UniValue& options = request.params[4].get_obj();

        if (options.exists("minimumAmount"))
            nMinimumAmount = AmountFromValue(options["minimumAmount"]);

        if (options.exists("maximumAmount"))
            nMaximumAmount = AmountFromValue(options["maximumAmount"]);

        if (options.exists("minimumSumAmount"))
            nMinimumSumAmount = AmountFromValue(options["minimumSumAmount"]);

        if (options.exists("maximumCount"))
            nMaximumCount = options["maximumCount"].get_int64();
    }
    // TODO: check txindex and sync

    UniValue results(UniValue::VARR);

    // Get transaction ids from UTXO index
    std::vector<AddressUnspentTransactionItem> unspentTransactions;
    if (!g_addrindex->GetUnspentTransactions(destinations, unspentTransactions)) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Error get from address index");
    }

    // Check exists TX in mempool
    // TODO: LOCK(mempool.cs);
    for (const auto& e : mempool.mapTx) {
        const CTransaction& tx = e.GetTx();
        for (const CTxIn& txin : tx.vin) {
            AddressUnspentTransactionItem mempoolItm = {"", txin.prevout.hash.ToString(), (int)txin.prevout.n};

            if (std::find_if(
                unspentTransactions.begin(),
                unspentTransactions.end(),
                [&](const AddressUnspentTransactionItem& itm) { return itm == mempoolItm; }) != unspentTransactions.end()) {
                unspentTransactions.erase(
                    remove(unspentTransactions.begin(), unspentTransactions.end(), mempoolItm),
                    unspentTransactions.end());
            }
        }
    }

    for (const auto& unsTx : unspentTransactions) {
        CBlockIndex* blockindex = nullptr;
        uint256 hash_block;
        CTransactionRef tx;
        uint256 txhash;
        txhash.SetHex(unsTx.txid);

        if (!GetTransaction(txhash, tx, Params().GetConsensus(), hash_block, true, blockindex)) {
            continue;
        }

        if (!blockindex) {
            blockindex = LookupBlockIndexWithoutLock(hash_block);
            if (!blockindex) {
                continue;
            }
        }

        const CTxOut& txout = tx->vout[unsTx.txout];

        // TODO: filter by amount

        CTxDestination destAddress;
        const CScript& scriptPubKey = txout.scriptPubKey;
        bool fValidAddress = ExtractDestination(scriptPubKey, destAddress);
        std::string encoded_address = EncodeDestination(destAddress);

        // TODO: filter by depth

        int confirmations = chainActive.Height() - blockindex->nHeight + 1;
        if (confirmations < nMinDepth || confirmations > nMaxDepth) continue;

        UniValue entry(UniValue::VOBJ);
        entry.pushKV("txid", unsTx.txid);
        entry.pushKV("vout", unsTx.txout);
        entry.pushKV("address", unsTx.address);
        entry.pushKV("scriptPubKey", HexStr(scriptPubKey.begin(), scriptPubKey.end()));
        entry.pushKV("amount", ValueFromAmount(txout.nValue));
        entry.pushKV("confirmations", confirmations);
        entry.pushKV("coinbase", tx->IsCoinBase() || tx->IsCoinStake());
        entry.pushKV("pockettx", g_addrindex->IsPocketnetTransaction(tx));
        results.push_back(entry);
    }

    return results;
}
//----------------------------------------------------------
UniValue getaddressregistration(const JSONRPCRequest& request)
{
    if (request.fHelp || request.params.size() > 1) {
        throw std::runtime_error(
            "getaddressregistration [\"addresses\",...]\n"
            "\nReturns array of registration dates.\n"
            "\nArguments:\n"
            "1. \"addresses\"      (string) A json array of pocketcoin addresses to filter\n"
            "    [\n"
            "      \"address\"     (string) pocketcoin address\n"
            "      ,...\n"
            "    ]\n"
            "\nResult\n"
            "[                             (array of json objects)\n"
            "  {\n"
            "    \"address\" : \"1PGFqEzfmQch1gKD3ra4k18PNj3tTUUSqg\",     (string) the pocketcoin address\n"
            "    \"date\" : \"1544596205\",                                (int64) date in Unix time format\n"
            "    \"date\" : \"2378659...\"                                 (string) id of first transaction with this address\n"
            "  },\n"
            "  ,...\n"
            "]");
    }

    std::vector<std::string> addresses;
    if (!request.params[0].isNull()) {
        RPCTypeCheckArgument(request.params[0], UniValue::VARR);
        UniValue inputs = request.params[0].get_array();
        for (unsigned int idx = 0; idx < inputs.size(); idx++) {
            const UniValue& input = inputs[idx];
            CTxDestination dest = DecodeDestination(input.get_str());

            if (!IsValidDestination(dest)) {
                throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, std::string("Invalid Pocketcoin address: ") + input.get_str());
            }

            if (std::find(addresses.begin(), addresses.end(), input.get_str()) == addresses.end()) {
                addresses.push_back(input.get_str());
            }
        }
    }
    //-------------------------
    UniValue results(UniValue::VARR);
    //-------------------------
    // Get transaction ids from UTXO index
    std::vector<AddressRegistrationItem> addrRegItems;
    if (!g_addrindex->GetAddressRegistrationDate(addresses, addrRegItems)) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Error get from address index");
    }
    //-------------------------
    for (const auto& addrReg : addrRegItems) {
        UniValue entry(UniValue::VOBJ);
        entry.pushKV("address", addrReg.address);
        entry.pushKV("date", (int64_t)addrReg.time);
        entry.pushKV("txid", addrReg.txid);
        results.push_back(entry);
    }
    //-------------------------
    return results;
}
//----------------------------------------------------------
UniValue getuserstate(const JSONRPCRequest& request)
{
    if (request.fHelp || request.params.size() < 1) {
        throw std::runtime_error(
            "getuserstate \"address\"\n"
            "\nReturns array of limits.\n"
            "\nArguments:\n"
            "1. \"address\"        (string) A pocketcoin addresses to filter\n"
            "\nResult\n"
            "[                             (array of json objects)\n"
            "  {\n"
            "    \"address\"       : \"1PGFqE..\",     (string) the pocketcoin address\n"
            "    \"reputation\"    : \"205\",          (int) reputation of user\n"
            "    \"balance\"       : \"20500000\",     (int64) sum of unspent transactions\n"
            "    \"trial\"         : \"true\",         (bool) trial mode?\n"
            "    \"post_unspent\"  : \"4\",            (int) unspent posts count\n"
            "    \"post_spent\"    : \"3\",            (int) spent posts count\n"
            "    \"score_unspent\" : \"3\",            (int) unspent scores count\n"
            "    \"score_spent\"   : \"3\",            (int) spent scores count\n"
            "  },\n"
            "  ,...\n"
            "]");
    }

    std::string address;
    if (request.params.size() > 0) {
        RPCTypeCheckArgument(request.params[0], UniValue::VSTR);
        CTxDestination dest = DecodeDestination(request.params[0].get_str());

        if (!IsValidDestination(dest)) {
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, std::string("Invalid Pocketcoin address: ") + request.params[0].get_str());
        }

        address = request.params[0].get_str();
    }

    // Get transaction ids from UTXO index
    UserStateItem userStateItm(address);
    if (!g_antibot->GetUserState(address, userStateItm)) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Error get from address index");
    }

    return userStateItm.Serialize();
}
//----------------------------------------------------------
UniValue gettime(const JSONRPCRequest& request)
{
    if (request.fHelp)
        throw std::runtime_error(
            "gettime\n"
            "\nReturn node time.\n");

    UniValue entry(UniValue::VOBJ);
    entry.pushKV("time", GetAdjustedTime());

    return entry;
}
//----------------------------------------------------------
UniValue getrecommendedposts(const JSONRPCRequest& request)
{
    if (request.fHelp || request.params.size() < 1) {
        throw std::runtime_error(
            "getrecommendedposts address count\n"
            "\nReturns array of recommended posts.\n"
            "\nArguments:\n"
            "1. address            (string) A pocketcoin addresses to filter\n"
            "2. count              (int) Max count of posts\n"
            "\nResult\n"
            "[                     (array of posts)\n"
            "  ...\n"
            "]");
    }

    std::string address;
    if (!request.params[0].isNull()) {
        RPCTypeCheckArgument(request.params[0], UniValue::VSTR);
        CTxDestination dest = DecodeDestination(request.params[0].get_str());

        if (!IsValidDestination(dest)) {
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, std::string("Invalid Pocketcoin address: ") + request.params[0].get_str());
        }

        address = request.params[0].get_str();
    }

    int count = 30;
    if (request.params.size() >= 2) {
        ParseInt32(request.params[1].get_str(), &count);
    }

    std::set<string> recommendedPosts;
    g_addrindex->GetRecommendedPostsByScores(address, count, recommendedPosts);
    g_addrindex->GetRecommendedPostsBySubscriptions(address, count, recommendedPosts);

    UniValue a(UniValue::VARR);
    for (string p : recommendedPosts) {
        a.push_back(p);
    }

    JSONRPCRequest jreq;
    jreq.params = UniValue(UniValue::VARR);
    jreq.params.push_back(a);
    return getrawtransactionwithmessagebyid(jreq);
}
UniValue getrecommendedposts2(const JSONRPCRequest& request) { return getrecommendedposts(request); }
//----------------------------------------------------------
UniValue searchtags(const JSONRPCRequest& request)
{
    if (request.fHelp || request.params.size() < 1) {
        throw std::runtime_error(
            "searchtags search_string count\n"
            "\nReturns array of found tags.\n"
            "\nArguments:\n"
            "1. search_string      (string) Symbols for search (minimum 3 symbols)\n"
            "2. count              (int) Max count results\n"
            "\nResult\n"
            "[                     (array of tags with frequency usage)\n"
            "  ...\n"
            "]");
    }

    std::string search_string;
    if (!request.params[0].isNull()) {
        RPCTypeCheckArgument(request.params[0], UniValue::VSTR);
        search_string = request.params[0].get_str();
    }

    int count = 10;
    if (request.params.size() >= 2) {
        ParseInt32(request.params[1].get_str(), &count);
    }

    int totalCount = 0;
    std::map<std::string, int> foundTags;
    g_pocketdb->SearchTags(search_string, count, foundTags, totalCount);

    UniValue a(UniValue::VOBJ);
    for (auto& p : foundTags) {
        a.pushKV(p.first, p.second);
    }

    return a;
}
//----------------------------------------------------------
UniValue search(const JSONRPCRequest& request)
{
    if (request.fHelp)
        throw std::runtime_error(
            "search ...\n"
            "\nSearch in Pocketnet DB.\n");

    std::string fulltext_search_string;
    std::string fulltext_search_string_strict;

    std::string search_string = "";
    if (request.params.size() > 0) {
        RPCTypeCheckArgument(request.params[0], UniValue::VSTR);
        search_string = UrlDecode(request.params[0].get_str());
    }

    std::string type = "";
    if (request.params.size() > 1) {
        RPCTypeCheckArgument(request.params[1], UniValue::VSTR);
        type = lower(request.params[1].get_str());
    }

    if (type != "all" && type != "posts" && type != "tags" && type != "users") {
        type = "fs";
    }

    bool fs = (type == "fs");
    bool all = (type == "all");

    int blockNumber = 0;
    if (request.params.size() > 2) {
        ParseInt32(request.params[2].get_str(), &blockNumber);
    }

    int resultStart = 0;
    if (request.params.size() > 3) {
        ParseInt32(request.params[3].get_str(), &resultStart);
    }

    int resulCount = 10;
    if (request.params.size() > 4) {
        ParseInt32(request.params[4].get_str(), &resulCount);
    }

    std::string address = "";
    if (request.params.size() > 5) {
        RPCTypeCheckArgument(request.params[5], UniValue::VSTR);
        CTxDestination dest = DecodeDestination(request.params[5].get_str());

        if (!IsValidDestination(dest)) {
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, std::string("Invalid Pocketcoin address: ") + request.params[5].get_str());
        }

        address = request.params[5].get_str();
    }

    int fsresultCount = 10;

    // --- Return object -----------------------------------------------
    UniValue result(UniValue::VOBJ);

    // --- First search tags -----------------------------------------------
    int totalCount;
    std::map<std::string, int> foundTagsRatings;
    std::vector<std::string> foundTags;
    std::map<std::string, std::set<std::string>> mTagsUsers;
    std::map<std::string, int> mFastSearch;

    if(type == "tags"){

    }

    // --- Search posts by Search String -----------------------------------------------
    if (fs || all || type == "posts") {
        //LogPrintf("--- Search: %s\n", fulltext_search_string);
        reindexer::QueryResults resPostsBySearchString;
        if (g_pocketdb->Select(
                reindexer::Query("Posts", resultStart, resulCount)
                    .Where("block", blockNumber ? CondLe : CondGe, blockNumber)
                    .Where(search_string.at(0) == '#' ? "tags" : "caption+message", CondEq, search_string.at(0) == '#' ? search_string.substr(1) : "\"" + search_string + "\"")
                    .Where("address", address == "" ? CondGt : CondEq, address)
                    .Sort("time", true)
                    .ReqTotal(),
                resPostsBySearchString)
            .ok()) {
            UniValue aPosts(UniValue::VARR);

            for (auto& it : resPostsBySearchString) {
                Item _itm = it.GetItem();
                std::string _txid = _itm["txid"].As<string>();
                std::string _caption = _itm["caption_"].As<string>();
                std::string _message = _itm["message_"].As<string>();

                if (fs) getFastSearchString(search_string, _caption, mFastSearch);
                if (fs) getFastSearchString(search_string, _message, mFastSearch);

                if (all || type == "posts") aPosts.push_back(getPostData(_itm, ""));
            }

            if (all || type == "posts") {
                UniValue oPosts(UniValue::VOBJ);
                oPosts.pushKV("count", resPostsBySearchString.totalCount);
                oPosts.pushKV("data", aPosts);
                result.pushKV("posts", oPosts);
            }
        }
    }

    // --- Search Users by Search String -----------------------------------------------
    if (all || type == "users") {
        reindexer::QueryResults resUsersBySearchString;
        if (g_pocketdb->Select(
                reindexer::Query("UsersView", resultStart, resulCount)
                    .Where("block", blockNumber ? CondLe : CondGe, blockNumber)
                    .Where("name_text", CondEq, "*" + UrlEncode(search_string) + "*")
                        //.Sort("time", false)  // Do not sort or think about full-text match first
                    .ReqTotal(),
                resUsersBySearchString)
            .ok()) {
            std::vector<std::string> vUserAdresses;

            for (auto& it : resUsersBySearchString) {
                Item _itm = it.GetItem();
                std::string _address = _itm["address"].As<string>();
                vUserAdresses.push_back(_address);
            }

            auto mUsers = getUsersProfiles(vUserAdresses, true, 1);

            UniValue aUsers(UniValue::VARR);
            for (const auto& item : vUserAdresses) {
                aUsers.push_back(mUsers[item]);
            }
            //for (auto& u : mUsers) {
            //    aUsers.push_back(u.second);
            //}

            UniValue oUsers(UniValue::VOBJ);
            oUsers.pushKV("count", resUsersBySearchString.totalCount);
            oUsers.pushKV("data", aUsers);

            result.pushKV("users", oUsers);
        }

        fsresultCount = resUsersBySearchString.Count() < fsresultCount ? fsresultCount - resUsersBySearchString.Count() : 0;
    }

    // --- Autocomplete for search string
    if (fs) {
        struct IntCmp {
            bool operator()(const std::pair<std::string, int>& lhs, const std::pair<std::string, int>& rhs)
            {
                return lhs.second > rhs.second; // Changed  < to > since we need DESC order
            }
        };

        UniValue fastsearch(UniValue::VARR);
        std::vector<std::pair<std::string, int>> vFastSearch;
        for (const auto& f : mFastSearch) {
            vFastSearch.push_back(std::pair<std::string, int>(f.first, f.second));
        }

        std::sort(vFastSearch.begin(), vFastSearch.end(), IntCmp());
        int _count = fsresultCount;
        for (auto& t : vFastSearch) {
            fastsearch.push_back(t.first);
            _count -= 1;
            if (_count <= 0) break;
        }
        result.pushKV("fastsearch", fastsearch);
    }

    return result;
}
UniValue search2(const JSONRPCRequest& request) { return search(request); }
//----------------------------------------------------------
UniValue gethotposts(const JSONRPCRequest& request)
{
    if (request.fHelp)
        throw std::runtime_error(
            "gethotposts\n"
            "\nReturn Pocketnet top posts.\n");

    int count = 30;
    if (request.params.size() > 0) {
        if (request.params[0].isNum()) {
            count = request.params[0].get_int();
        } else if (request.params[0].isStr()) {
            ParseInt32(request.params[0].get_str(), &count);
        }
    }

    // Depth in blocks (default about 3 days)
    int dayInBlocks = 24 * 60;
    int depthBlocks = 3 * dayInBlocks;
    if (request.params.size() > 1) {
        if (request.params[1].isNum()) {
            depthBlocks = request.params[1].get_int();
        } else if (request.params[1].isStr()) {
            ParseInt32(request.params[1].get_str(), &depthBlocks);
        }
        if (depthBlocks == 259200) { // for old version electron
            depthBlocks = 3 * dayInBlocks;
        }
        depthBlocks = std::min(depthBlocks, 365 * dayInBlocks);
    }

    int nHeightOffset = chainActive.Height();
    int nOffset = 0;
    if (request.params.size() > 2) {
        if (request.params[2].isNum()) {
            if (request.params[2].get_int() > 0) {
                nOffset = request.params[2].get_int();
            }
        } else if (request.params[2].isStr()) {
            ParseInt32(request.params[2].get_str(), &nOffset);
        }
        nHeightOffset -= nOffset;
    }

    std::string lang = "";
    if (request.params.size() > 3) {
        lang = request.params[3].get_str();
    }

    std::vector<int> contentTypes;
    if (request.params.size() > 4) {
        if (request.params[4].isNum()) {
            contentTypes.push_back(request.params[4].get_int());
        } else if (request.params[4].isStr()) {
            if (getcontenttype(request.params[4].get_str()) >= 0) {
                contentTypes.push_back(getcontenttype(request.params[5].get_str()));
            }
        } else if (request.params[4].isArray()) {
            UniValue cntntTps = request.params[4].get_array();
            if (cntntTps.size() > 10) {
                throw JSONRPCError(RPC_INVALID_PARAMS, "Too large array content types");
            }
            if(cntntTps.size() > 0) {
                for (unsigned int idx = 0; idx < cntntTps.size(); idx++) {
                    if (cntntTps[idx].isNum()) {
                        contentTypes.push_back(cntntTps[idx].get_int());
                    } else if (cntntTps[idx].isStr()) {
                        if (getcontenttype(cntntTps[idx].get_str()) >= 0) {
                            contentTypes.push_back(getcontenttype(cntntTps[idx].get_str()));
                        }
                    }
                }
            }
        }
    }

    // Excluded posts
    vector<string> addrsblock;

    // Do not show posts from users with reputation < Limit::bad_reputation
    int64_t _bad_reputation_limit = GetActualLimit(Limit::bad_reputation, chainActive.Height());
    reindexer::QueryResults queryResBadReputation;
    g_pocketdb->DB()->Select(reindexer::Query("UsersView").Where("reputation", CondLe, _bad_reputation_limit), queryResBadReputation);

    for (auto it : queryResBadReputation) {
        reindexer::Item itm(it.GetItem());
        addrsblock.push_back(itm["address"].As<string>());
    }

    reindexer::QueryResults postsRes;
    reindexer::Query query;

    query = reindexer::Query("Posts", 0, count);
    query = query.Where("block", CondLe, nHeightOffset);
    query = query.Where("block", CondGt, nHeightOffset - depthBlocks);
    query = query.Where("time", CondLe, GetAdjustedTime());
    query = query.Not().Where("address", CondSet, addrsblock);
    if (!lang.empty()) {
        query = query.Where("lang", CondEq, lang);
    }
    if (!contentTypes.empty()) {
        query = query.Where("type", CondSet, contentTypes);
    }

    query = query.Sort("reputation", true);
    query = query.Sort("scoreSum", true);

    g_pocketdb->Select(query, postsRes);

    UniValue result(UniValue::VARR);
    for (auto& p : postsRes) {
        reindexer::Item postItm = p.GetItem();

        if (postItm["reputation"].As<int>() > 0) {
            result.push_back(getPostData(postItm, ""));
        }
    }

    return result;
}
UniValue gethotposts2(const JSONRPCRequest& request) { return gethotposts(request); }
//----------------------------------------------------------
UniValue getuseraddress(const JSONRPCRequest& request)
{
    if (request.fHelp || request.params.size() > 2)
        throw std::runtime_error(
            "getuseraddress \"user_name\" ( count )\n"
            "\nGet list addresses of user.\n");

    std::string userName;
    if (!request.params[0].isNull()) {
        RPCTypeCheckArgument(request.params[0], UniValue::VSTR);
        userName = request.params[0].get_str();
    }

    int count = 7;
    if (request.params.size() >= 2) {
        ParseInt32(request.params[1].get_str(), &count);
    }

    reindexer::QueryResults users;
    g_pocketdb->Select(reindexer::Query("UsersView", 0, count).Where("name", CondEq, userName), users);

    UniValue aResult(UniValue::VARR);
    for (auto& u : users) {
        reindexer::Item userItm = u.GetItem();

        UniValue oUser(UniValue::VOBJ);
        oUser.pushKV("name", userItm["name"].As<string>());
        oUser.pushKV("address", userItm["address"].As<string>());

        aResult.push_back(oUser);
    }

    return aResult;
}
//----------------------------------------------------------
UniValue getreputations(const JSONRPCRequest& request)
{
    if (request.fHelp)
        throw std::runtime_error(
            "getreputations\n"
            "\nGet list repuatations of users.\n");

    reindexer::QueryResults users;
    g_pocketdb->Select(reindexer::Query("UsersView"), users);

    UniValue aResult(UniValue::VARR);
    for (auto& u : users) {
        reindexer::Item userItm = u.GetItem();

        UniValue oUser(UniValue::VOBJ);
        oUser.pushKV("address", userItm["address"].As<string>());
        oUser.pushKV("referrer", userItm["referrer"].As<string>());
        oUser.pushKV("reputation", userItm["reputation"].As<string>());

        aResult.push_back(oUser);
    }

    return aResult;
}
//----------------------------------------------------------
UniValue getcontents(const JSONRPCRequest& request)
{
    if (request.fHelp || request.params.size() < 1) {
        throw std::runtime_error(
            "getcontents address\n"
            "\nReturns contents for address.\n"
            "\nArguments:\n"
            "1. address            (string) A pocketcoin addresses to filter\n"
            "\nResult\n"
            "[                     (array of contents)\n"
            "  ...\n"
            "]");
    }

    std::string address;
    if (!request.params[0].isNull()) {
        RPCTypeCheckArgument(request.params[0], UniValue::VSTR);
        CTxDestination dest = DecodeDestination(request.params[0].get_str());

        if (!IsValidDestination(dest)) {
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, std::string("Invalid Pocketcoin address: ") + request.params[0].get_str());
        }

        address = request.params[0].get_str();
    }

    reindexer::QueryResults posts;
    g_pocketdb->Select(reindexer::Query("Posts").Where("address", CondEq, address), posts);

    UniValue aResult(UniValue::VARR);
    for (auto& p : posts) {
        reindexer::Item postItm = p.GetItem();

        UniValue oPost(UniValue::VOBJ);
        oPost.pushKV("content", UrlDecode(postItm["caption"].As<string>()) == "" ? UrlDecode(postItm["message"].As<string>()).substr(0, 100) : UrlDecode(postItm["caption"].As<string>()));
        oPost.pushKV("txid", postItm["txid"].As<string>());
        oPost.pushKV("time", postItm["time"].As<string>());
        oPost.pushKV("reputation", postItm["reputation"].As<string>());
        oPost.pushKV("settings", postItm["settings"].As<string>());
        oPost.pushKV("scoreSum", postItm["scoreSum"].As<string>());
        oPost.pushKV("scoreCnt", postItm["scoreCnt"].As<string>());

        aResult.push_back(oPost);
    }
    return aResult;
}
//----------------------------------------------------------
UniValue gettags(const JSONRPCRequest& request)
{
    std::string address = "";
    if (!request.params[0].isNull() && request.params[0].get_str().length() > 0) {
        RPCTypeCheckArgument(request.params[0], UniValue::VSTR);
        CTxDestination dest = DecodeDestination(request.params[0].get_str());

        if (!IsValidDestination(dest)) {
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, std::string("Invalid Pocketcoin address: ") + request.params[0].get_str());
        }

        address = request.params[0].get_str();
    }

    int count = 50;
    if (request.params.size() > 1) {
        ParseInt32(request.params[1].get_str(), &count);
    }

    int from = 0;
    if (request.params.size() > 2) {
        ParseInt32(request.params[2].get_str(), &from);
    }

    std::string lang = "";
    if (request.params.size() > 3) {
        lang = request.params[3].get_str();
    }

    std::map<std::string, int> mapTags;
    reindexer::QueryResults posts;
    g_pocketdb->Select(reindexer::Query("Posts").Where("block", CondGe, from).Where("lang", (lang == "" ? CondGe : CondEq), lang).Where("address", address == "" ? CondGt : CondEq, address), posts);
    for (auto& p : posts) {
        reindexer::Item postItm = p.GetItem();

        UniValue t(UniValue::VARR);
        reindexer::VariantArray va = postItm["tags"];
        for (unsigned int idx = 0; idx < va.size(); idx++) {
            std::string sTag = lower(va[idx].As<string>());
            if (std::all_of(sTag.begin(), sTag.end(), [](unsigned char ch) { return ::isdigit(ch) || ::isalpha(ch); })) {
                if (mapTags.count(sTag) == 0)
                    mapTags[sTag] = 1;
                else
                    mapTags[sTag] = mapTags[sTag] + 1;
            }
        }
    }

    typedef std::function<bool(std::pair<std::string, int>, std::pair<std::string, int>)> Comparator;
    Comparator compFunctor = [](std::pair<std::string, int> elem1, std::pair<std::string, int> elem2) { return elem1.second > elem2.second; };
    std::set<std::pair<std::string, int>, Comparator> setTags(mapTags.begin(), mapTags.end(), compFunctor);

    UniValue aResult(UniValue::VARR);
    for (std::set<std::pair<std::string, int>, Comparator>::iterator it = setTags.begin(); it != setTags.end() && count; ++it, --count) {
        UniValue oTag(UniValue::VOBJ);
        oTag.pushKV("tag", it->first);
        oTag.pushKV("count", std::to_string(it->second));

        aResult.push_back(oTag);
    }

    return aResult;
}
//----------------------------------------------------------
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
//----------------------------------------------------------
UniValue getlastcomments(const JSONRPCRequest& request)
{
    if (request.fHelp)
        throw std::runtime_error(
            "getlastcomments (count)\n"
            "\nGet Pocketnet last comments.\n");

    int resultCount = 10;
    if (request.params.size() > 0) {
        if (request.params[2].isNum()) {
            resultCount = request.params[0].get_int();
        }
    }

    std::string address = "";
    if (request.params.size() > 1) {
        address = request.params[1].get_str();
    }

    std::string lang = "";
    if (request.params.size() > 2) {
        lang = request.params[2].get_str();
    }

    int nHeight = chainActive.Height();
    reindexer::Query query;
    reindexer::QueryResults queryResults;

    query = reindexer::Query("Comment");
    query = query.Where("last", CondEq, true);
    query = query.Where("block", CondLe, nHeight);
    query = query.Where("block", CondGt, nHeight - 600);
    query = query.Where("time", CondLe, GetAdjustedTime());
    query = query.Sort("block", true);
    query = query.Sort("time", true);
    query = query.InnerJoin("postid", "txid", CondEq, Query("Posts").Limit(1));

    g_pocketdb->Select(query, queryResults);

    UniValue aResult(UniValue::VARR);
    for (auto& it : queryResults) {
        reindexer::Item cmntItm = it.GetItem();

        string cmntLang = it.GetJoined()[0][0].GetItem()["lang"].As<string>();

        if(!lang.empty() && cmntLang == lang) {
            UniValue oCmnt(UniValue::VOBJ);
            oCmnt.pushKV("id", cmntItm["otxid"].As<string>());
            oCmnt.pushKV("postid", cmntItm["postid"].As<string>());
            oCmnt.pushKV("address", cmntItm["address"].As<string>());
            oCmnt.pushKV("time", cmntItm["time"].As<string>());
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

            aResult.push_back(oCmnt);
        }
        if (aResult.size() >= resultCount) {
            break;
        }
    }

    return aResult;
}
//----------------------------------------------------------
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
//----------------------------------------------------------
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
//----------------------------------------------------------
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

    UniValue result(UniValue::VARR);

/*
    reindexer::QueryResults queryRes;
    g_pocketdb->DB()->Select(
        reindexer::Query("Posts").Where("txid", CondSet, TxIds).LeftJoin("txid", "posttxid", CondEq, Query("Scores").Where("address", CondEq, address).Where("value", CondGt, 3)), queryRes);

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
*/

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
//----------------------------------------------------------
//----------------------------------------------------------
//----------------------------------------------------------
//--METHODS 2.0
//----------------------------------------------------------
//----------------------------------------------------------
UniValue converttxidaddress(const JSONRPCRequest& request)
{
    if (request.params.size() == 0 || request.fHelp)
        throw std::runtime_error(
            "converttxidaddress\n"
            "\n.\n");

    std::string txid = "";
    std::string txidshort = "";
    std::string txhex = "";
    int nblock = 0;
    int ntx = 0;
    std::string address = "";
    std::string addressshort = "";
    int userid = 0;

    if (request.params.size() > 0) {
        if (request.params[0].isStr()) {
            txhex = request.params[0].get_str();
            if (txhex.length() == 64) {
                txid = txhex;

                CTransactionRef tx;
                uint256 hash_block;
                uint256 hash_tx;
                hash_tx.SetHex(txid);
                if (g_txindex->FindTx(hash_tx , hash_block, tx)) {
                    CBlock block;
                    if (!g_addrindex->IsPocketnetTransaction(tx)) {
                        throw JSONRPCError(RPC_DESERIALIZATION_ERROR, "Not Pocketnet transaction");
                    }
                    const CBlockIndex* pblockindex = LookupBlockIndex(hash_block);
                    if (ReadBlockFromDisk(block, pblockindex, Params().GetConsensus())) {
                        nblock = pblockindex->nHeight;
                        ntx = 0;
                        for (const auto& tx : block.vtx) {
                            if (tx->GetHash() == hash_tx) {
                                break;
                            }
                            ntx += 1;
                        }
                        std::stringstream ss;
                        ss << "k" << std::hex << nblock << "t" << std::hex << ntx;
                        txidshort = ss.str();
                    }
                    else {
                        throw JSONRPCError(RPC_INTERNAL_ERROR, "Can't read block from disk");
                    }
                }
                else {
                    throw JSONRPCError(RPC_TRANSACTION_ERROR, "Can't fint tx");
                }
            } else if (txhex.length() < 64 && txhex.length() > 3 && txhex.find('k') != string::npos && txhex.find('t') != string::npos && txhex.find('t') > txhex.find('k')) {
                txidshort = txhex;
                std::istringstream(txidshort.substr(txidshort.find('k') + 1, txidshort.find('t') - txidshort.find('k') - 1)) >> std::hex >> nblock;
                std::istringstream(txidshort.substr(txidshort.find('t') + 1, txidshort.length() - txidshort.find('t') - 1)) >> std::hex >> ntx;

                CBlock block;
                const CBlockIndex* pblockindex = chainActive[nblock];
                if (ReadBlockFromDisk(block, pblockindex, Params().GetConsensus())) {
                    for (const auto& tx : block.vtx) {
                        if (ntx == 0) {
                            txid = tx->GetHash().GetHex();
                            break;
                        }
                        ntx -= 1;
                    }
                }
                else {
                    throw JSONRPCError(RPC_INTERNAL_ERROR, "Can't read block from disk");
                }
            } else {
                throw JSONRPCError(RPC_INVALID_PARAMS, "Invalid transaction format");
            }
        } else {
            throw JSONRPCError(RPC_INVALID_PARAMS, "Invalid transaction format");
        }
    }

    if (request.params.size() > 1) {
        RPCTypeCheckArgument(request.params[1], UniValue::VSTR);
        address = request.params[1].get_str();
        if (address.length() < 34) {
            addressshort = address;

            std::istringstream(addressshort.substr(addressshort.find('s') + 1, txidshort.length() - txidshort.find('s') - 1)) >> std::hex >> userid;

            reindexer::Item userItm;
            reindexer::Error errU = g_pocketdb->SelectOne(reindexer::Query("UsersView").Where("id", CondEq, userid), userItm);
            if (errU.ok()) {
                address = userItm["address"].As<string>();
            } else {
                throw JSONRPCError(RPC_DATABASE_ERROR, std::string("There is no user in DB"));
            }
        }
        else {
            CTxDestination dest = DecodeDestination(address);
            if (IsValidDestination(dest)) {
                reindexer::Item userItm;
                reindexer::Error errU = g_pocketdb->SelectOne(reindexer::Query("UsersView").Where("address", CondEq, address), userItm);
                if (errU.ok()) {
                    std::stringstream ss;
                    ss << "s" << std::hex << userItm["id"].As<int>();
                    addressshort = ss.str();
                } else {
                    throw JSONRPCError(RPC_DATABASE_ERROR, std::string("There is no user in DB"));
                }
            } else {
                throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, std::string("Invalid Pocketcoin address: "));
            }
        }
    }

    UniValue result(UniValue::VOBJ);
    result.pushKV("txid", txid);
    result.pushKV("txidshort", txidshort);
    result.pushKV("address", address);
    result.pushKV("addressshort", addressshort);
    return result;
}

// Do not change input or output params (used in gethierarchicalstrip)
UniValue gethistoricalstrip(const JSONRPCRequest& request)
{
    if (request.fHelp)
        throw std::runtime_error(
            "gethistoricalstrip\n"
            "\n.\n");

    int nHeight = chainActive.Height();
    if (request.params.size() > 0) {
        if (request.params[0].isNum()) {
            if (request.params[0].get_int() > 0) {
                nHeight = request.params[0].get_int();
            }
        }
    }

    std::string start_txid = "";
    if (request.params.size() > 1) {
        start_txid = request.params[1].get_str();
    }

    int countOut = 10;
    if (request.params.size() > 2) {
        if (request.params[2].isNum()) {
            countOut = request.params[2].get_int();
        }
    }

    std::string lang = "";
    if (request.params.size() > 3) {
        lang = request.params[3].get_str();
    }

    std::vector<string> tags;
    if (request.params.size() > 4) {
        if (request.params[4].isStr()) {
            std::string tag = boost::trim_copy(request.params[4].get_str());
            if (!tag.empty()) {
                tags.push_back(tag);
            }
        } else if (request.params[4].isArray()) {
            UniValue tgs = request.params[4].get_array();
            if (tgs.size() > 1000) {
                throw JSONRPCError(RPC_INVALID_PARAMS, "Too large array tags");
            }
            if(tgs.size() > 0) {
                for (unsigned int idx = 0; idx < tgs.size(); idx++) {
                    std::string tag = boost::trim_copy(tgs[idx].get_str());
                    if (!tag.empty()) {
                        tags.push_back(tag);
                    }
                }
            }
        }
    }

    std::vector<int> contentTypes;
    if (request.params.size() > 5) {
        if (request.params[5].isNum()) {
            contentTypes.push_back(request.params[5].get_int());
        } else if (request.params[5].isStr()) {
            if (getcontenttype(request.params[5].get_str()) >= 0) {
                contentTypes.push_back(getcontenttype(request.params[5].get_str()));
            }
        } else if (request.params[5].isArray()) {
            UniValue cntntTps = request.params[5].get_array();
            if (cntntTps.size() > 10) {
                throw JSONRPCError(RPC_INVALID_PARAMS, "Too large array content types");
            }
            if(cntntTps.size() > 0) {
                for (unsigned int idx = 0; idx < cntntTps.size(); idx++) {
                    if (cntntTps[idx].isNum()) {
                        contentTypes.push_back(cntntTps[idx].get_int());
                    } else if (cntntTps[idx].isStr()) {
                        if (getcontenttype(cntntTps[idx].get_str()) >= 0) {
                            contentTypes.push_back(getcontenttype(cntntTps[idx].get_str()));
                        }
                    }
                }
            }
        }
    }

    std::vector<string> txidsExcluded;
    if (request.params.size() > 6) {
        if (request.params[6].isStr()) {
            txidsExcluded.push_back(request.params[6].get_str());
        } else if (request.params[6].isArray()) {
            UniValue txids = request.params[6].get_array();
            //if (txids.size() > 1000) {
            //    throw JSONRPCError(RPC_INVALID_PARAMS, "Too large array txids");
            //}
            if(txids.size() > 0) {
                for (unsigned int idx = 0; idx < txids.size(); idx++) {
                    std::string txidEx = boost::trim_copy(txids[idx].get_str());
                    if (!txidEx.empty()) {
                        txidsExcluded.push_back(txidEx);
                    }
                }
            }
        }
    }

    std::vector<string> adrsExcluded;
    if (request.params.size() > 7) {
        if (request.params[7].isStr()) {
            adrsExcluded.push_back(request.params[7].get_str());
        } else if (request.params[7].isArray()) {
            UniValue adrs = request.params[7].get_array();
            if (adrs.size() > 1000) {
                throw JSONRPCError(RPC_INVALID_PARAMS, "Too large array addresses");
            }
            if(adrs.size() > 0) {
                for (unsigned int idx = 0; idx < adrs.size(); idx++) {
                    std::string adrEx = boost::trim_copy(adrs[idx].get_str());
                    if (!adrEx.empty()) {
                        adrsExcluded.push_back(adrEx);
                    }
                }
            }
        }
    }

    reindexer::Error err;
    reindexer::Query query;
    reindexer::QueryResults queryResults;

    int cntTransactionInSameBlock = 0;
    int nHeightFrom = nHeight;
    if (!start_txid.empty()) {
        reindexer::Item blItm;
        reindexer::Error blError = g_pocketdb->SelectOne(reindexer::Query("Posts").Where("txid", CondEq, start_txid), blItm);
        if (blError.ok()) {
            nHeightFrom = blItm["block"].As<int>();
            cntTransactionInSameBlock = g_pocketdb->SelectCount(reindexer::Query("Posts").Where("block", CondEq, nHeightFrom));
        }
    }

    query = reindexer::Query("Posts");
    query = query.Where("block", CondLe, nHeightFrom);
    query = query.Where("time", CondLe, GetAdjustedTime());
    query = query.Where("txidRepost", CondEq, "");
    if (!lang.empty()) {
        query = query.Where("lang", CondEq, lang);
    }
    if (!tags.empty()) {
        query = query.Where("tags", CondSet, tags);
    }
    if (!contentTypes.empty()) {
        query = query.Where("type", CondSet, contentTypes);
    }
    if (!txidsExcluded.empty()) {
        query = query.Not().Where("txid", CondSet, txidsExcluded);
    }
    if (!adrsExcluded.empty()) {
        query = query.Not().Where("address", CondSet, adrsExcluded);
    }
    query = query.Sort("block", true);
    query = query.Sort("time", true);
    query = query.Limit(countOut + cntTransactionInSameBlock);

    err = g_pocketdb->DB()->Select(query, queryResults);

    UniValue contents(UniValue::VARR);
    if (err.ok()) {
        bool onOutput = start_txid.empty();
        for (auto it : queryResults) {
            reindexer::Item postItm(it.GetItem());

            if (onOutput) {
                UniValue entry(UniValue::VOBJ);
                entry = getPostData(postItm, "");
                contents.push_back(entry);
            }

            if (!start_txid.empty()) {
                onOutput = onOutput || start_txid == postItm["txid"].As<string>();
            }
        }
    }

    UniValue result(UniValue::VOBJ);
    result.pushKV("height", nHeight);
    result.pushKV("contents", contents);
    return result;
}

UniValue gethierarchicalstrip(const JSONRPCRequest& request)
{
    if (request.fHelp)
        throw std::runtime_error(
            "gethierarchicalstrip\n"
            "\n.\n");

    int nHeight = chainActive.Height();
    if (request.params.size() > 0) {
        if (request.params[0].isNum()) {
            if (request.params[0].get_int() > 0) {
                nHeight = request.params[0].get_int();
            }
        }
    }

    std::string start_txid = "";
    if (request.params.size() > 1) {
        start_txid = request.params[1].get_str();
    }

    int countOut = 10;
    if (request.params.size() > 2) {
        if (request.params[2].isNum()) {
            countOut = request.params[2].get_int();
        }
    }

    std::string lang = "";
    if (request.params.size() > 3) {
        lang = request.params[3].get_str();
    }

    std::vector<string> tags;
    UniValue uvTags(UniValue::VARR);
    if (request.params.size() > 4) {
        if (request.params[4].isStr()) {
            std::string tag = boost::trim_copy(request.params[4].get_str());
            if (!tag.empty()) {
                tags.push_back(tag);
                uvTags.push_back(tag);
            }
        } else if (request.params[4].isArray()) {
            UniValue tgs = request.params[4].get_array();
            if (tgs.size() > 1000) {
                throw JSONRPCError(RPC_INVALID_PARAMS, "Too large array tags");
            }
            if(tgs.size() > 0) {
                uvTags = tgs;
                for (unsigned int idx = 0; idx < tgs.size(); idx++) {
                    std::string tag = boost::trim_copy(tgs[idx].get_str());
                    if (!tag.empty()) {
                        tags.push_back(tag);
                    }
                }
            }
        }
    }

    std::vector<int> contentTypes;
    UniValue uvContentTypes(UniValue::VARR);
    if (request.params.size() > 5) {
        if (request.params[5].isNum()) {
            contentTypes.push_back(request.params[5].get_int());
            uvContentTypes.push_back(request.params[5].get_int());
        } else if (request.params[5].isStr()) {
            if (getcontenttype(request.params[5].get_str()) >= 0) {
                contentTypes.push_back(getcontenttype(request.params[5].get_str()));
                uvContentTypes.push_back(request.params[5].get_str());
            }
        } else if (request.params[5].isArray()) {
            UniValue cntntTps = request.params[5].get_array();
            if (cntntTps.size() > 10) {
                throw JSONRPCError(RPC_INVALID_PARAMS, "Too large array content types");
            }
            if(cntntTps.size() > 0) {
                uvContentTypes = cntntTps;
                for (unsigned int idx = 0; idx < cntntTps.size(); idx++) {
                    if (cntntTps[idx].isNum()) {
                        contentTypes.push_back(cntntTps[idx].get_int());
                    } else if (cntntTps[idx].isStr()) {
                        if (getcontenttype(cntntTps[idx].get_str()) >= 0) {
                            contentTypes.push_back(getcontenttype(cntntTps[idx].get_str()));
                        }
                    }
                }
            }
        }
    }

    std::vector<string> txidsExcluded;
    UniValue uvTxidsExcluded(UniValue::VARR);
    if (request.params.size() > 6) {
        if (request.params[6].isStr()) {
            txidsExcluded.push_back(request.params[6].get_str());
            uvTxidsExcluded.push_back(request.params[6].get_str());
        } else if (request.params[6].isArray()) {
            UniValue txids = request.params[6].get_array();
            if (txids.size() > 1000) {
                throw JSONRPCError(RPC_INVALID_PARAMS, "Too large array txids");
            }
            if(txids.size() > 0) {
                uvTxidsExcluded = txids;
                for (unsigned int idx = 0; idx < txids.size(); idx++) {
                    std::string txidEx = boost::trim_copy(txids[idx].get_str());
                    if (!txidEx.empty()) {
                        txidsExcluded.push_back(txidEx);
                    }
                }
            }
        }
    }

    std::vector<string> adrsExcluded;
    UniValue uvAdrsExcluded(UniValue::VARR);
    if (request.params.size() > 7) {
        if (request.params[7].isStr()) {
            adrsExcluded.push_back(request.params[7].get_str());
            uvAdrsExcluded.push_back(request.params[7].get_str());
        } else if (request.params[7].isArray()) {
            UniValue adrs = request.params[7].get_array();
            if (adrs.size() > 1000) {
                throw JSONRPCError(RPC_INVALID_PARAMS, "Too large array addresses");
            }
            if(adrs.size() > 0) {
                uvAdrsExcluded = adrs;
                for (unsigned int idx = 0; idx < adrs.size(); idx++) {
                    std::string adrEx = boost::trim_copy(adrs[idx].get_str());
                    if (!adrEx.empty()) {
                        adrsExcluded.push_back(adrEx);
                    }
                }
            }
        }
    }

    enum PostRanks {LAST5, LAST5R, BOOST, UREP, UREPR, DREP, PREP, PREPR, DPOST, POSTRF};
    int cntBlocksForResult = 300;
    int cntPrevPosts = 5;
    int durationBlocksForPrevPosts = 30 * 24 * 60; // about 1 month
    double dekayRep = 0.82;//0.7;
    double dekayPost = 0.96;

    std::vector<std::string> txidsOut;
    std::vector<std::string> txidsHierarchical;

    std::map<std::string, std::map<PostRanks, double>> postsRanks;
    /*
    enum scoreDetail {ADDRESS, VALUE, TXID}; // DEBUGINFO
    std::map<std::string, std::vector<std::map<scoreDetail, std::string>>> postScoresDetails; // DEBUGINFO
    std::map<std::string, std::vector<std::map<scoreDetail, std::string>>> postPrevScoresDetails; // DEBUGINFO
     */

    reindexer::Error err;
    reindexer::Query query;
    reindexer::QueryResults queryResults;

    query = reindexer::Query("Posts");
    query = query.Where("block", CondLe, nHeight);
    query = query.Where("block", CondGt, nHeight - cntBlocksForResult);
    query = query.Where("time", CondLe, GetAdjustedTime());
    query = query.Where("txidRepost", CondEq, "");
    if (!lang.empty()) {
        query = query.Where("lang", CondEq, lang);
    }
    if (!tags.empty()) {
        query = query.Where("tags", CondSet, tags);
    }
    if (!contentTypes.empty()) {
        query = query.Where("type", CondSet, contentTypes);
    }
    if (!txidsExcluded.empty()) {
        query = query.Not().Where("txid", CondSet, txidsExcluded);
    }
    if (!adrsExcluded.empty()) {
        query = query.Not().Where("address", CondSet, adrsExcluded);
    }
    query = query.LeftJoin("txid", "posttxid", CondEq, reindexer::Query("PostRatings").Where("block", CondLe, nHeight).Sort("block", true).Limit(1));
    query = query.LeftJoin("address", "address", CondEq, reindexer::Query("UserRatings").Where("block", CondLe, nHeight).Sort("block", true).Limit(1));
    query = query.LeftJoin("txid", "txid", CondEq, reindexer::Query("PostsHistory").Where("block", CondLe, nHeight).Sort("block", false).Limit(1));
    query = query.Sort("block", true).Sort("time", true);

    err = g_pocketdb->DB()->Select(query, queryResults);
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
            std::string posttxid = itm["txid"].As<string>();
            std::string postaddress = itm["address"].As<string>();
            int postblock = itm["block"].As<int>();
            int postblockOrig = postblock;
            if (it.GetJoined().size() > 2 && it.GetJoined()[2].Count() > 0) {
                postblockOrig = it.GetJoined()[2][0].GetItem()["block"].As<int>();
            }
            uvTxidsExcluded.push_back(posttxid);

            /*
            // DEBUGINFO
            if (postRep != 0) {
                reindexer::QueryResults postRepQueryResult;
                if (g_pocketdb->DB()->Select(reindexer::Query("Scores").Where("posttxid", CondEq, posttxid).Where("block", CondLe, nHeight), postRepQueryResult).ok()) {
                    for (auto itPostRep : postRepQueryResult) {
                        reindexer::Item itmPR(itPostRep.GetItem());
                        if (g_antibot->AllowModifyReputationOverPost(itmPR["address"].As<string>(), postaddress, itmPR["block"].As<int>(), itmPR["time"].As<int64_t>(), posttxid, false)) {
                            std::map<scoreDetail, std::string> postScoresDetail;           // DEBUGINFO
                            postScoresDetail[ADDRESS] = itmPR["address"].As<string>(); // DEBUGINFO
                            postScoresDetail[VALUE] = itmPR["value"].As<string>();     // DEBUGINFO
                            postScoresDetails[posttxid].push_back(postScoresDetail);       // DEBUGINFO
                        }
                    }
                }
            }
             */

            queryPrevsPosts = reindexer::Query("Posts", 0, cntPrevPosts)
                .Where("address", CondEq, postaddress)
                .Where("block", CondLt, postblockOrig)
                .Where("block", CondGe, postblockOrig - durationBlocksForPrevPosts);
            err = g_pocketdb->DB()->Select(queryPrevsPosts, queryPrevsPostsResult);
            if (err.ok()) {
                std::vector<std::string> prevPostsIds;
                for (auto itPrevPost : queryPrevsPostsResult) {
                    reindexer::Item itmPP(itPrevPost.GetItem());
                    prevPostsIds.push_back(itmPP["txid"].As<string>());
                }
                std::vector<int> scores = {1, 5};
                reindexer::Query queryScores;
                reindexer::QueryResults queryScoresResult;
                queryScores = reindexer::Query("Scores");
                queryScores = queryScores.Where("posttxid", CondSet, prevPostsIds);
                queryScores = queryScores.Where("block", CondLe, postblock);
                queryScores = queryScores.Where("value", CondSet, scores);
                reindexer::Error errSc = g_pocketdb->DB()->Select(queryScores, queryScoresResult);
                if (errSc.ok()) {
                    std::vector<string> addressesRated;
                    for (auto itScore : queryScoresResult) {
                        reindexer::Item itmScore(itScore.GetItem());
                        if (g_antibot->AllowModifyReputationOverPost(itmScore["address"].As<string>(), postaddress, itmScore["block"].As<int>(), itmScore["time"].As<int64_t>(), itmScore["txid"].As<string>(), false)) {
                            if(std::find(addressesRated.begin(), addressesRated.end(), itmScore["address"].As<string>()) == addressesRated.end()) {
                                addressesRated.push_back(itmScore["address"].As<string>());
                                cntPositiveScores += itmScore["value"].As<int>() == 5 ? 1 : -1;
                                /*
                                std::map<scoreDetail, std::string> postPrevScoresDetail; // DEBUGINFO
                                postPrevScoresDetail[ADDRESS] = itmScore["address"].As<string>(); // DEBUGINFO
                                postPrevScoresDetail[VALUE] = itmScore["value"].As<string>(); // DEBUGINFO
                                postPrevScoresDetail[TXID] = itmScore["posttxid"].As<string>(); // DEBUGINFO
                                postPrevScoresDetails[posttxid].push_back(postPrevScoresDetail); // DEBUGINFO
                                 */
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

        std::map<PostRanks, int> count;
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
                postsRanks[iPostRank.first][UREPR] = std::min(postsRanks[iPostRank.first][UREP], 1.0 * (count[UREPR] * 100) / (nElements - 1)) * (postsRanks[iPostRank.first][UREP] < 0 ? 2.0 : 1.0);
                postsRanks[iPostRank.first][PREPR] = std::min(postsRanks[iPostRank.first][PREP], 1.0 * (count[PREPR] * 100) / (nElements - 1)) * (postsRanks[iPostRank.first][PREP] < 0 ? 2.0 : 1.0);
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

        std::vector<std::pair<double, string>> postsRaited;

        UniValue oPost(UniValue::VOBJ);
        for (auto iPostRank : postsRanks) {
            postsRaited.push_back(std::make_pair(iPostRank.second[POSTRF], iPostRank.first));
        }

        std::sort(postsRaited.begin(), postsRaited.end(), std::greater{});

        for (auto v : postsRaited) {
            txidsHierarchical.push_back(v.second);
        }
    }

    UniValue contents(UniValue::VARR);

    if(!txidsHierarchical.empty())
    {
        std::vector<std::string>::iterator itVec;
        if (!start_txid.empty()) {
            if ((itVec = std::find(txidsHierarchical.begin(), txidsHierarchical.end(), start_txid)) != txidsHierarchical.end()) {
                ++itVec;
                start_txid.clear();
            }
        }
        else {
            itVec = txidsHierarchical.begin();
        }

        for(; itVec != txidsHierarchical.end() && countOut > 0; ++itVec, countOut--) {
            reindexer::Item postItm;
            reindexer::Error errS = g_pocketdb->SelectOne(reindexer::Query("Posts").Where("txid", CondEq, *itVec), postItm);
            UniValue entry(UniValue::VOBJ);
            entry = getPostData(postItm, "");
            /*
            if(postsRanks.find(*itVec) != postsRanks.end()) {
                UniValue entryRanks(UniValue::VOBJ); // DEBUGINFO
                entryRanks.pushKV("LAST5", postsRanks[*itVec][LAST5]); // DEBUGINFO
                entryRanks.pushKV("LAST5R", postsRanks[*itVec][LAST5R]); // DEBUGINFO

                UniValue entrtyPrevScoresAdrs(UniValue::VARR); // DEBUGINFO
                for(auto pPrevScoreDetail : postPrevScoresDetails[*itVec]) // DEBUGINFO
                {
                    UniValue entryScoreAdr(UniValue::VOBJ); // DEBUGINFO
                    entryScoreAdr.pushKV("address", pPrevScoreDetail[ADDRESS]); // DEBUGINFO
                    entryScoreAdr.pushKV("value", pPrevScoreDetail[VALUE]); // DEBUGINFO
                    entryScoreAdr.pushKV("posttxid", pPrevScoreDetail[TXID]); // DEBUGINFO
                    entrtyPrevScoresAdrs.push_back(entryScoreAdr); // DEBUGINFO
                }
                entryRanks.pushKV("LAST5RADRS", entrtyPrevScoresAdrs); // DEBUGINFO

                entryRanks.pushKV("BOOST", postsRanks[*itVec][BOOST]); // DEBUGINFO
                entryRanks.pushKV("UREP", postsRanks[*itVec][UREP]); // DEBUGINFO
                entryRanks.pushKV("UREPR", postsRanks[*itVec][UREPR]); // DEBUGINFO
                entryRanks.pushKV("DREP", postsRanks[*itVec][DREP]); // DEBUGINFO
                entryRanks.pushKV("PREP", postsRanks[*itVec][PREP]); // DEBUGINFO

                UniValue entrtyScoresAdrs(UniValue::VARR); // DEBUGINFO
                for(auto pScoreDetail : postScoresDetails[*itVec]) // DEBUGINFO
                {
                    UniValue entryScoreAdr(UniValue::VOBJ); // DEBUGINFO
                    entryScoreAdr.pushKV("address", pScoreDetail[ADDRESS]); // DEBUGINFO
                    entryScoreAdr.pushKV("value", pScoreDetail[VALUE]); // DEBUGINFO
                    entrtyScoresAdrs.push_back(entryScoreAdr); // DEBUGINFO
                }
                entryRanks.pushKV("PREPRADRS", entrtyScoresAdrs); // DEBUGINFO

                entryRanks.pushKV("PREPR", postsRanks[*itVec][PREPR]); // DEBUGINFO
                entryRanks.pushKV("DPOST", postsRanks[*itVec][DPOST]); // DEBUGINFO
                entryRanks.pushKV("POSTRF", postsRanks[*itVec][POSTRF]); // DEBUGINFO
                entry.pushKV("ranks", entryRanks); // DEBUGINFO
            }
             */
            contents.push_back(entry);
            uvTxidsExcluded.push_back(*itVec);
        }
    }

    if (txidsHierarchical.empty() || countOut > 0) {
        JSONRPCRequest new_request;
        new_request = request;
        UniValue new_params(UniValue::VARR);
        new_params.push_back(nHeight);
        new_params.push_back(contents.empty() ? start_txid : "");
        new_params.push_back(countOut);
        new_params.push_back(lang);
        new_params.push_back(uvTags);
        new_params.push_back(uvContentTypes);
        new_params.push_back(uvTxidsExcluded);
        new_params.push_back(uvAdrsExcluded);
        new_request.params = new_params;

        UniValue histRes = gethistoricalstrip(new_request);
        if (!histRes.empty()) {
            if (histRes.size() > 1) {
                UniValue histContents(UniValue::VARR);
                histContents = histRes[1].get_array();
                for (const auto& item : histContents.getValues()) {
                    contents.push_back(item);
                }
            }
        }
    }

    UniValue result(UniValue::VOBJ);
    result.pushKV("height", nHeight);
    result.pushKV("contents", contents);
    return result;
}
//----------------------------------------------------------

static const CRPCCommand commands[] =
    {
        {"pocketnetrpc", "getrawtransactionwithmessage",      &getrawtransactionwithmessage,      {"address_from", "address_to", "start_txid", "count", "lang", "tags", "contenttypes"}, false},
        {"pocketnetrpc", "getrawtransactionwithmessage2",     &getrawtransactionwithmessage2,     {"address_from", "address_to", "start_txid", "count"},                                 false},
        {"pocketnetrpc", "getrawtransactionwithmessagebyid",  &getrawtransactionwithmessagebyid,  {"txs", "address"},                                                                    false},
        {"pocketnetrpc", "getrawtransactionwithmessagebyid2", &getrawtransactionwithmessagebyid2, {"txs", "address"},                                                                    false},
        {"pocketnetrpc", "getuserprofile",                    &getuserprofile,                    {"addresses", "short"},                                                                false},
        {"pocketnetrpc", "getmissedinfo",                     &getmissedinfo,                     {"address", "blocknumber"},                                                            false},
        {"pocketnetrpc", "getmissedinfo2",                    &getmissedinfo2,                    {"address", "blocknumber"},                                                            false},
        {"pocketnetrpc", "txunspent",                         &txunspent,                         {"addresses", "minconf", "maxconf", "include_unsafe", "query_options"},                false},
        {"pocketnetrpc", "getaddressregistration",            &getaddressregistration,            {"addresses"},                                                                         false},
        {"pocketnetrpc", "getuserstate",                      &getuserstate,                      {"address"},                                                                           false},
        {"pocketnetrpc", "gettime",                           &gettime,                           {},                                                                                    false},
        {"pocketnetrpc", "getrecommendedposts",               &getrecommendedposts,               {"address", "count"},                                                                  false},
        {"pocketnetrpc", "getrecommendedposts2",              &getrecommendedposts2,              {"address", "count"},                                                                  false},
        {"pocketnetrpc", "searchtags",                        &searchtags,                        {"search_string", "count"},                                                            false},
        {"pocketnetrpc", "search",                            &search,                            {"search_string", "type", "count"},                                                    false},
        {"pocketnetrpc", "search2",                           &search2,                           {"search_string", "type", "count"},                                                    false},
        {"pocketnetrpc", "gethotposts",                       &gethotposts,                       {"count", "depth", "height", "lang", "contenttypes"},                                  false},
        {"pocketnetrpc", "gethotposts2",                      &gethotposts2,                      {"count", "depth"},                                                                    false},
        {"pocketnetrpc", "getuseraddress",                    &getuseraddress,                    {"name", "count"},                                                                     false},
        {"pocketnetrpc", "getreputations",                    &getreputations,                    {},                                                                                    false},
        {"pocketnetrpc", "getcontents",                       &getcontents,                       {"address"},                                                                           false},
        {"pocketnetrpc", "gettags",                           &gettags,                           {"address", "count"},                                                                  false},
        {"pocketnetrpc", "getlastcomments2",                  &getlastcomments,                   {"count", "address"},                                                                  false},
        {"pocketnetrpc", "getlastcomments",                   &getlastcomments,                   {"count", "address"},                                                                  false},
        {"pocketnetrpc", "getcomments2",                      &getcomments,                       {"postid", "parentid", "address", "ids"},                                              false},
        {"pocketnetrpc", "getcomments",                       &getcomments,                       {"postid", "parentid", "address", "ids"},                                              false},
        {"pocketnetrpc", "getaddressscores",                  &getaddressscores,                  {"address", "txs"},                                                                    false},
        {"pocketnetrpc", "getpostscores",                     &getpostscores,                     {"txs", "address"},                                                                    false},
        {"pocketnetrpc", "getpagescores",                     &getpagescores,                     {"txs", "address", "cmntids"},                                                         false},
        {"pocketnetrpc", "converttxidaddress",                &converttxidaddress,                {"txid", "address"},                                                                   false},
        {"pocketnetrpc", "gethistoricalstrip",                &gethistoricalstrip,                {"height", "start_txid", "count", "lang", "tags", "contenttypes", "txids_exclude", "adrs_exclude"}, false},
        {"pocketnetrpc", "gethierarchicalstrip",              &gethierarchicalstrip,              {"height", "start_txid", "count", "lang", "tags", "contenttypes", "txids_exclude", "adrs_exclude"}, false},

        // Pocketnet transactions
        {"pocketnetrpc", "sendrawtransactionwithmessage",     &sendrawtransactionwithmessage,     {"hexstring", "message", "type"}, false},

// TODO (brangr): new types
//        {"pocketnetrpc", "setshare",                          &SetShare,                          {"hexstring", "message"},         false},
//        {"pocketnetrpc", "setvideo",                          &SetVideo,                          {"hexstring", "message"},         false},
//        {"pocketnetrpc", "setserverping",                     &SetServerPing,                     {"hexstring", "message"},         false},
//        {"pocketnetrpc", "settranslate",                      &SetTranslate,                      {"hexstring", "message"},         false},
//
//        {"pocketnetrpc", "setupvoteshare",                    &SetUpvoteShare,                    {"hexstring", "message"},         false},
//        {"pocketnetrpc", "setsubscribe",                      &SetSubscribe,                      {"hexstring", "message"},         false},
//        {"pocketnetrpc", "setsubscribeprivate",               &SetSubscribePrivate,               {"hexstring", "message"},         false},
//        {"pocketnetrpc", "setunsubscribe",                    &SetUnsubscribe,                    {"hexstring", "message"},         false},
//        {"pocketnetrpc", "setcomplainshare",                  &SetComplainShare,                  {"hexstring", "message"},         false},
//        {"pocketnetrpc", "setblocking",                       &SetBlocking,                       {"hexstring", "message"},         false},
//        {"pocketnetrpc", "setunblocking",                     &SetUnblocking,                     {"hexstring", "message"},         false},
//        {"pocketnetrpc", "setcomment",                        &SetComment,                        {"hexstring", "message"},         false},
//        {"pocketnetrpc", "setcommentedit",                    &SetCommentEdit,                    {"hexstring", "message"},         false},
//        {"pocketnetrpc", "setcommentdelete",                  &SetCommentDelete,                  {"hexstring", "message"},         false},
//        {"pocketnetrpc", "setcommentscore",                   &SetCommentScore,                   {"hexstring", "message"},         false},
//
//        {"pocketnetrpc", "setuserinfo",                       &SetUserInfo,                       {"hexstring", "message"},         false},
//        {"pocketnetrpc", "setvideoserver",                    &SetVideoServer,                    {"hexstring", "message"},         false},
//        {"pocketnetrpc", "setmessageserver",                  &SetMessageServer,                  {"hexstring", "message"},         false},
    };

void RegisterPocketnetRPCCommands(CRPCTable& t)
{
    for (unsigned int vcidx = 0; vcidx < ARRAYLEN(commands); vcidx++)
        t.appendCommand(commands[vcidx].name, &commands[vcidx]);
}