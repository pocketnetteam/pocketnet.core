#include "notifyprocessor.h"

#include "validation.h"
#include "primitives/block.h"
#include "pocketdb/pocketnet.h"


NotifyBlockProcessor::NotifyBlockProcessor(std::shared_ptr<ProtectedMap<std::string, WSUser>> WSConnections) 
{
    m_WSConnections = std::move(WSConnections);
}

void NotifyBlockProcessor::PrepareWSMessage(std::map<std::string, std::vector<UniValue>>& messages, std::string msg_type, std::string addrTo, std::string txid, int64_t txtime, custom_fields cFields)
{
    UniValue msg(UniValue::VOBJ);
    msg.pushKV("addr", addrTo);
    msg.pushKV("msg", msg_type);
    msg.pushKV("txid", txid);
    msg.pushKV("time", txtime);

    for (auto& it : cFields) {
        msg.pushKV(it.first, it.second);
    }

    if (messages.find(addrTo) == messages.end()) {
        std::vector<UniValue> _list;
        messages.insert(std::make_pair(addrTo, _list));
    }

    messages[addrTo].push_back(msg);
}

void NotifyBlockProcessor::Process(std::pair<CBlock, CBlockIndex*> entry)
{
    if (m_WSConnections->empty()) {
        return;
    }

    const auto& block = entry.first;
    auto blockIndex = entry.second;
    std::map<std::string, std::vector<UniValue>> messages;
    uint256 _block_hash = block.GetHash();
    int sharesCnt = 0;
    std::map<std::string, int> sharesCntLang;
    std::string txidpocketnet;
    std::string addrespocketnet = "PEj7QNjKdDPqE9kMDRboKoCtp8V6vZeZPd";

    for (const auto& tx : block.vtx) {
        std::map<std::string, std::pair<int, int64_t>> addrs;
        int64_t txtime = tx->nTime;
        std::string txid = tx->GetHash().GetHex();
        std::string optype;

        // Get all addresses from tx outs and check OP_RETURN
        for (int i = 0; i < tx->vout.size(); i++) {
            const CTxOut& txout = tx->vout[i];
            //-------------------------
            if (txout.scriptPubKey[0] == OP_RETURN) {
                std::string asmstr = ScriptToAsmStr(txout.scriptPubKey);
                std::vector<std::string> spl;
                boost::split(spl, asmstr, boost::is_any_of("\t "));
                if (spl.size() >= 3) {
                    if (spl[1] == OR_POST) {
                        optype = "share";
                        sharesCnt += 1;

                        auto response = PocketDb::NotifierRepoInst.GetPostLang(txid);

                        if (response.exists("lang")) {
                            std::string lang = response["lang"].get_str();

                            auto itl = sharesCntLang.find(lang);
                            if (itl != sharesCntLang.end())
                                itl->second += 1;
                            else
                                sharesCntLang.emplace(lang, 1);
                        }
                    }
                    // else if (spl[1] == OR_POSTEDIT)
                    // 	optype = "shareEdit";
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
                    else if (spl[1] == OR_COMMENT_SCORE)
                        optype = "cScore";
                    else if (spl[1] == OR_COMMENT)
                        optype = "comment";
                    else if (spl[1] == OR_COMMENT_EDIT)
                        optype = "commentEdit";
                    else if (spl[1] == OR_COMMENT_DELETE)
                        optype = "commentDelete";
                }
            }
            //-------------------------
            CTxDestination destAddress;
            bool fValidAddress = ExtractDestination(txout.scriptPubKey, destAddress);
            if (fValidAddress) {
                std::string encoded_address = EncodeDestination(destAddress);
                if (addrs.find(encoded_address) == addrs.end())
                    addrs.emplace(encoded_address, std::make_pair(i, (int64_t)txout.nValue));
            }
        }

        for (auto const& addr : addrs) {
            // Event for new transaction
            custom_fields cTrFields{
                {"nout", std::to_string(addr.second.first)},
                {"amount", std::to_string(addr.second.second)},
            };

            if (!optype.empty()) cTrFields.emplace("type", optype);
            PrepareWSMessage(messages, "transaction", addr.first, txid, txtime, cTrFields);

            // Event for new PocketNET transaction
            if (optype == "share" || optype == "video") {
                auto response = PocketDb::NotifierRepoInst.GetPostInfo(txid);
                if (response.exists("hash") && response.exists("rootHash") && response["hash"].get_str() != response["rootHash"].get_str())
                    continue;

                if (addr.first == addrespocketnet && txidpocketnet.find(txid) == std::string::npos) {
                    txidpocketnet += txid + ",";
                } else {
                    auto response = PocketDb::NotifierRepoInst.GetOriginalPostAddressByRepost(txid);
                    if (response.exists("hash")) {
                        std::string address = response["address"].get_str();

                        custom_fields cFields{
                            {"mesType", "reshare"},
                            {"txidRepost", response["hash"].get_str()},
                            {"addrFrom", response["addressRepost"].get_str()},
                            {"nameFrom", response["nameRepost"].get_str()},
                            {"avatarFrom", ""}};
                        if (response.exists("avatarRepost"))
                            cFields["avatarFrom"] = response["avatarRepost"].get_str();

                        PrepareWSMessage(messages, "event", address, txid, txtime, cFields);
                    }
                }

                auto subscribesResponse = PocketDb::NotifierRepoInst.GetPrivateSubscribeAddressesByAddressTo(addr.first);
                for (size_t i = 0; i < subscribesResponse.size(); ++i) {
                    auto address = subscribesResponse[i]["addressTo"].get_str();

                    custom_fields cFields{
                        {"mesType", "postfromprivate"},
                        {"addrFrom", addr.first},
                        {"nameFrom", subscribesResponse[i]["nameFrom"].get_str()},
                        {"avatarFrom", ""}};

                    if (subscribesResponse[i].exists("avatarFrom"))
                        cFields["avatarFrom"] = subscribesResponse[i]["avatarFrom"].get_str();

                    PrepareWSMessage(messages, "event", address, txid, txtime, cFields);
                }
            } else if (optype == "userInfo") {
                auto response = PocketDb::NotifierRepoInst.GetUserReferrerAddress(txid);
                if (response.exists("referrerAddress")) {
                    custom_fields cFields{
                        {"mesType", optype},
                        {"addrFrom", addr.first},
                        {"nameFrom", response["referralName"].get_str()},
                        {"avatarFrom", ""}};
                    if (response.exists("referralAvatar"))
                        cFields["avatarFrom"] = response["referralAvatar"].get_str();

                    PrepareWSMessage(messages, "event", response["referrerAddress"].get_str(), txid, txtime, cFields);
                }
            } else if (optype == "upvoteShare") {
                auto response = PocketDb::NotifierRepoInst.GetPostInfoAddressByScore(txid);
                if (response.exists("postTxHash")) {
                    custom_fields cFields{
                        {"mesType", optype},
                        {"addrFrom", addr.first},
                        {"nameFrom", response["scoreName"].get_str()},
                        {"avatarFrom", ""},
                        {"posttxid", response["postTxHash"].get_str()},
                        {"upvoteVal", response["value"].get_str()}};

                    if (response.exists("scoreAvatar"))
                        cFields["avatarFrom"] = response["scoreAvatar"].get_str();

                    PrepareWSMessage(messages, "event", response["postAddress"].get_str(), txid, txtime, cFields);
                }
            } else if (optype == "subscribe" || optype == "subscribePrivate" || optype == "unsubscribe") {
                auto response = PocketDb::NotifierRepoInst.GetSubscribeAddressTo(txid);
                if (response.exists("addressTo")) {
                    custom_fields cFields{
                        {"mesType", optype},
                        {"addrFrom", addr.first},
                        {"nameFrom", response["nameFrom"].get_str()},
                        {"avatarFrom", ""}};

                    if (response.exists("avatarFrom"))
                        cFields["avatarFrom"] = response["avatarFrom"].get_str();

                    PrepareWSMessage(messages, "event", response["addressTo"].get_str(), txid, txtime, cFields);
                }
            } else if (optype == "cScore") {
                auto response = PocketDb::NotifierRepoInst.GetCommentInfoAddressByScore(txid);
                if (response.exists("commentHash")) {
                    custom_fields cFields{
                        {"mesType", optype},
                        {"addrFrom", addr.first},
                        {"nameFrom", response["scoreCommentName"].get_str()},
                        {"avatarFrom", ""},
                        {"commentid", response["commentHash"].get_str()},
                        {"upvoteVal", response["value"].get_str()}};

                    if (response.exists("scoreCommentAvatar"))
                        cFields["avatarFrom"] = response["scoreCommentAvatar"].get_str();

                    PrepareWSMessage(messages, "event", response["commentAddress"].get_str(), txid, txtime, cFields);
                }
            } else if (optype == "comment" || optype == "commentEdit" || optype == "commentDelete") {
                auto response = PocketDb::NotifierRepoInst.GetFullCommentInfo(txid);
                if (response.exists("postHash")) {
                    if (response["postAddress"].get_str() == addr.first)
                        continue;

                    custom_fields cFields{
                        {"mesType", optype},
                        {"addrFrom", addr.first},
                        {"nameFrom", response["commentName"].get_str()},
                        {"avatarFrom", ""},
                        {"posttxid", response["postHash"].get_str()},
                        {"parentid", response["parentHash"].get_str()},
                        {"answerid", response["answerHash"].get_str()},
                        {"reason", "post"},
                    };

                    if (response.exists("commentAvatar"))
                        cFields["avatarFrom"] = response["commentAvatar"].get_str();

                    if (response.exists("donation")) {
                        cFields.emplace("donation", "true");
                        cFields.emplace("amount", response["amount"].get_str());
                    }

                    PrepareWSMessage(messages, "event", response["postAddress"].get_str(), response["rootHash"].get_str(), txtime, cFields);

                    if (response.exists("answerAddress") && !response["answerAddress"].get_str().empty()) {
                        custom_fields c1Fields{
                            {"mesType", optype},
                            {"addrFrom", addr.first},
                            {"nameFrom", response["commentName"].get_str()},
                            {"avatarFrom", ""},
                            {"posttxid", response["postHash"].get_str()},
                            {"parentid", response["parentHash"].get_str()},
                            {"answerid", response["answerHash"].get_str()},
                            {"reason", "answer"},
                        };

                        if (response.exists("commentAvatar"))
                            cFields["avatarFrom"] = response["commentAvatar"].get_str();

                        PrepareWSMessage(messages, "event", response["answerAddress"].get_str(), response["rootHash"].get_str(), txtime, c1Fields);
                    }
                }
            }
        }
    }

    // Send all WS clients messages
    UniValue sharesLang(UniValue::VOBJ);
    for (std::map<std::string, int>::iterator itl = sharesCntLang.begin(); itl != sharesCntLang.end(); ++itl) {
        sharesLang.pushKV(itl->first, itl->second);
    }

    auto send = [&](std::pair<const std::string, WSUser>& connWS) {
        UniValue msg(UniValue::VOBJ);
        msg.pushKV("addr", connWS.second.Address);
        msg.pushKV("msg", "new block");
        msg.pushKV("blockhash", _block_hash.GetHex());
        msg.pushKV("time", std::to_string(block.nTime));
        msg.pushKV("height", blockIndex->nHeight);
        msg.pushKV("shares", sharesCnt);
        msg.pushKV("sharesLang", sharesLang);

        auto countResponse = PocketDb::NotifierRepoInst.GetPostCountFromMySubscribes(connWS.second.Address, blockIndex->nHeight);
        if (countResponse.exists("count")) {
            msg.pushKV("sharesSubscr", countResponse["count"].get_int());
        }

        if (blockIndex->nHeight > connWS.second.Block) {
            try {
                connWS.second.Connection->send(msg.write(), [](const SimpleWeb::error_code& ec) {});
            } catch (const std::exception& e) {
                LogPrintf("Error: CChainState::NotifyWSClients (1) - %s\n", e.what());
            }

            if (txidpocketnet != "") {
                try {
                    UniValue m(UniValue::VOBJ);
                    m.pushKV("msg", "sharepocketnet");
                    m.pushKV("time", std::to_string(block.nTime));
                    m.pushKV("txids", txidpocketnet.substr(0, txidpocketnet.size() - 1));
                    connWS.second.Connection->send(m.write(), [](const SimpleWeb::error_code& ec) {});
                } catch (const std::exception& e) {
                    LogPrintf("Error: CChainState::NotifyWSClients (1) - %s\n", e.what());
                }
            }

            if (messages.find(connWS.second.Address) != messages.end()) {
                for (auto m : messages[connWS.second.Address]) {
                    try {
                        connWS.second.Connection->send(m.write(), [](const SimpleWeb::error_code& ec) {});
                    } catch (const std::exception& e) {
                        LogPrintf("Error: CChainState::NotifyWSClients (2) - %s\n", e.what());
                    }
                }
            }

            connWS.second.Block = blockIndex->nHeight;
        }
    };
    m_WSConnections->Iterate(send);
}
