// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 Bitcoin developers
// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETTX_TRANSACTIONSERIALIZER_HPP
#define POCKETTX_TRANSACTIONSERIALIZER_HPP

#include "pocketdb/models/base/Transaction.hpp"
#include "pocketdb/models/base/Payload.hpp"
#include "pocketdb/models/dto/User.hpp"
#include "pocketdb/models/dto/Post.hpp"
#include "pocketdb/models/dto/Blocking.hpp"
#include "pocketdb/models/dto/Comment.hpp"
#include "pocketdb/models/dto/ScorePost.hpp"
#include "pocketdb/models/dto/Subscribe.hpp"
#include "pocketdb/models/dto/ScoreComment.hpp"
#include "pocketdb/models/dto/Complain.hpp"

namespace PocketTx {

class TransactionSerializer
{
public:
    static PocketTxType ParseType(const std::string &strType, const UniValue& txDataSrc)
    {
        // TODO (brangr): implement enum for tx types
        if (strType == "Users") return PocketTxType::USER_ACCOUNT;
        
        if (strType == "Posts") return PocketTxType::POST_CONTENT;
        if (strType == "Comment") return PocketTxType::COMMENT_CONTENT;
        
        if (strType == "Scores") return PocketTxType::SCORE_POST_ACTION;
        if (strType == "CommentScores") return PocketTxType::SCORE_COMMENT_ACTION;
                
        if (strType == "Blocking") {
            if (txDataSrc.exists("unblocking") &&
                txDataSrc.isBool("unblocking") &&
                txDataSrc["unblocking"].get_bool())
                return PocketTXType::BLOCKING_CANCEL_ACTION;

            return PocketTxType::BLOCKING_ACTION;
        }

        if (strType == "Subscribes") {
            if (txDataSrc.exists("unsubscribe") &&
                txDataSrc.isBool("unsubscribe") &&
                txDataSrc["unsubscribe"].get_bool())
                return PocketTXType::SUBSCRIBE_CANCEL_ACTION;

            if (txDataSrc.exists("private") &&
                txDataSrc.isBool("private") &&
                txDataSrc["private"].get_bool())
                return PocketTXType::SUBSCRIBE_PRIVATE_ACTION;

            return PocketTxType::SUBSCRIBE_ACTION;
        }
        
        if (strType == "Complains") return PocketTxType::COMPLAIN_ACTION;
    }

    static Transaction *BuildInstance(const UniValue &src)
    {
        auto txTypeSrc = src["t"].get_str();

        UniValue txDataSrc(UniValue::VOBJ);
        auto txDataBase64 = src["d"].get_str();
        auto txJson = DecodeBase64(txDataBase64);
        txDataSrc.read(txJson);

        PocketTxType txType = ParseType(txTypeSrc, txDataSrc);

        Transaction *tx;
        switch (txType)
        {
            case USER_ACCOUNT:
                return new User(txDataSrc);
            case VIDEO_SERVER_ACCOUNT:
            case MESSAGE_SERVER_ACCOUNT:
            return nullptr;
            case POST_CONTENT:
                return new Post(txDataSrc);
            case VIDEO_CONTENT:
            case TRANSLATE_CONTENT:
            case SERVERPING_CONTENT:
                return nullptr;
            case COMMENT_CONTENT:
                return new Comment(txDataSrc);
            case SCORE_POST_ACTION:
                return new ScorePost(txDataSrc);
            case SCORE_COMMENT_ACTION:
                return new ScoreComment(txDataSrc);
            case SUBSCRIBE_ACTION:
                return new Subscribe(txDataSrc);
            case SUBSCRIBE_PRIVATE_ACTION:
                return new SubscribePrivate(txDataSrc);
            case SUBSCRIBE_CANCEL_ACTION:
                return new SubscribeCancel(txDataSrc);
            case BLOCKING_ACTION:
                return new Blocking(txDataSrc);
            case BLOCKING_CANCEL_ACTION:
                return new BlockingCancel(txDataSrc);
            case COMPLAIN_ACTION:
                return new Complain(txDataSrc);
            default:
                return nullptr;
        }
    }

    static std::vector<PocketTx::Transaction *> DeserializeBlock(std::string &src)
    {
        std::vector<PocketTx::Transaction *> pocketTxn;

        UniValue pocketData(UniValue::VOBJ);
        pocketData.read(src);

        auto keys = pocketData.getKeys();
        for (const auto &key : keys)
        {
            auto entrySrc = pocketData[key];
            UniValue entry(UniValue::VOBJ);
            entry.read(entrySrc.get_str());

            auto tx = BuildInstance(entry);
            if (tx != nullptr)
                pocketTxn.push_back(tx);
        }

        return pocketTxn;
    }
};

}

#endif // POCKETTX_TRANSACTIONSERIALIZER_HPP
