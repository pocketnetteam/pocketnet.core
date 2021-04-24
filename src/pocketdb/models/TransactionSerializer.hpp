// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 Bitcoin developers
// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETTX_TRANSACTIONSERIALIZER_HPP
#define POCKETTX_TRANSACTIONSERIALIZER_HPP

#include "pocketdb/models/base/Transaction.hpp"
#include "pocketdb/models/dto/User.hpp"
#include "pocketdb/models/dto/Post.hpp"
#include "pocketdb/models/dto/Blocking.hpp"
#include "pocketdb/models/dto/BlockingCancel.hpp"
#include "pocketdb/models/dto/Comment.hpp"
#include "pocketdb/models/dto/ScorePost.hpp"
#include "pocketdb/models/dto/Subscribe.hpp"
#include "pocketdb/models/dto/SubscribePrivate.hpp"
#include "pocketdb/models/dto/SubscribeCancel.hpp"
#include "pocketdb/models/dto/ScoreComment.hpp"
#include "pocketdb/models/dto/Complain.hpp"

namespace PocketTx {

class TransactionSerializer
{
public:
    static PocketTxType ParseType(const std::string &strType, const UniValue& txDataSrc)
    {
        // TODO (brangr): implement enum for tx types
        if (strType == "Users")
            return PocketTxType::USER_ACCOUNT;
        
        if (strType == "Posts") 
            return PocketTxType::POST_CONTENT;

        if (strType == "Comment") 
            return PocketTxType::COMMENT_CONTENT;
        
        if (strType == "Scores") 
            return PocketTxType::SCORE_POST_ACTION;

        if (strType == "CommentScores") 
            return PocketTxType::SCORE_COMMENT_ACTION;
                
        if (strType == "Blocking") {
            if (txDataSrc.exists("unblocking") &&
                txDataSrc["unblocking"].isBool() &&
                txDataSrc["unblocking"].get_bool())
                return PocketTxType::BLOCKING_CANCEL_ACTION;

            return PocketTxType::BLOCKING_ACTION;
        }

        if (strType == "Subscribes") {
            if (txDataSrc.exists("unsubscribe") &&
                txDataSrc["unsubscribe"].isBool() &&
                txDataSrc["unsubscribe"].get_bool())
                return PocketTxType::SUBSCRIBE_CANCEL_ACTION;

            if (txDataSrc.exists("private") &&
                txDataSrc["private"].isBool() &&
                txDataSrc["private"].get_bool())
                return PocketTxType::SUBSCRIBE_PRIVATE_ACTION;

            return PocketTxType::SUBSCRIBE_ACTION;
        }
        
        if (strType == "Complains")
            return PocketTxType::COMPLAIN_ACTION;

        return PocketTxType::NOT_SUPPORTED;
    }

    static shared_ptr<Transaction> BuildInstance(const UniValue &src)
    {
        auto txTypeSrc = src["t"].get_str();

        UniValue txDataSrc(UniValue::VOBJ);
        auto txDataBase64 = src["d"].get_str();
        auto txJson = DecodeBase64(txDataBase64);
        txDataSrc.read(txJson);

        shared_ptr<Transaction> tx;
        PocketTxType txType = ParseType(txTypeSrc, txDataSrc);
        switch (txType)
        {
            case USER_ACCOUNT:
                tx = make_shared<User>();
                break;
            case VIDEO_SERVER_ACCOUNT:
            case MESSAGE_SERVER_ACCOUNT:
                break;
            case POST_CONTENT:
                tx = make_shared<Post>();
                break;
            case VIDEO_CONTENT:
            case TRANSLATE_CONTENT:
            case SERVERPING_CONTENT:
                break;
            case COMMENT_CONTENT:
                tx = make_shared<Comment>();
                break;
            case SCORE_POST_ACTION:
                tx = make_shared<ScorePost>();
                break;
            case SCORE_COMMENT_ACTION:
                tx = make_shared<ScoreComment>();
                break;
            case SUBSCRIBE_ACTION:
                tx = make_shared<Subscribe>();
                break;
            case SUBSCRIBE_PRIVATE_ACTION:
                tx = make_shared<SubscribePrivate>();
                break;
            case SUBSCRIBE_CANCEL_ACTION:
                tx = make_shared<SubscribeCancel>();
                break;
            case BLOCKING_ACTION:
                tx = make_shared<Blocking>();
                break;
            case BLOCKING_CANCEL_ACTION:
                tx = make_shared<BlockingCancel>();
                break;
            case COMPLAIN_ACTION:
                tx = make_shared<Complain>();
                break;
            default:
                return nullptr;
        }

        if (!tx)
            return nullptr;

        tx->Deserialize(txDataSrc);
        tx->BuildPayload(txDataSrc);
        tx->BuildHash(txDataSrc);
        return tx;
    }

    static std::vector<shared_ptr<Transaction>> DeserializeBlock(std::string &src)
    {
        std::vector<shared_ptr<Transaction>> pocketTxn;

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
