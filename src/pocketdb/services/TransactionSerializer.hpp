// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 Bitcoin developers
// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETTX_TRANSACTIONSERIALIZER_HPP
#define POCKETTX_TRANSACTIONSERIALIZER_HPP

#include "../models/base/Transaction.hpp"
#include "../models/dto/User.hpp"
#include "../models/dto/Post.hpp"
#include "../models/dto/Blocking.hpp"
#include "../models/dto/BlockingCancel.hpp"
#include "../models/dto/Comment.hpp"
#include "../models/dto/ScorePost.hpp"
#include "../models/dto/Subscribe.hpp"
#include "../models/dto/SubscribePrivate.hpp"
#include "../models/dto/SubscribeCancel.hpp"
#include "../models/dto/ScoreComment.hpp"
#include "../models/dto/Complain.hpp"

namespace PocketServices
{
    using namespace PocketTx;

    class TransactionSerializer
    {
    public:

        static PocketTxType ParseType(const CTransactionRef &tx, std::vector<std::string> &vasm)
        {
            const CTxOut &txout = tx->vout[0];
            if (txout.scriptPubKey[0] == OP_RETURN)
            {
                auto asmStr = ScriptToAsmStr(txout.scriptPubKey);
                boost::split(vasm, asmStr, boost::is_any_of("\t "));
                if (vasm.size() >= 2)
                    return ConvertOpReturnToType(vasm[1]);
            }

            return PocketTxType::NOT_SUPPORTED;
        }

        static PocketTxType ParseType(const std::string &reindexerTable, const UniValue &reindexerSrc)
        {
            // TODO (brangr): implement enum for tx types
            if (reindexerTable == "Users")
                return PocketTxType::USER_ACCOUNT;

            if (reindexerTable == "Posts")
                return PocketTxType::POST_CONTENT;

            if (reindexerTable == "Comment")
                return PocketTxType::COMMENT_CONTENT;

            if (reindexerTable == "Scores")
                return PocketTxType::SCORE_POST_ACTION;

            if (reindexerTable == "CommentScores")
                return PocketTxType::SCORE_COMMENT_ACTION;

            if (reindexerTable == "Blocking")
            {
                if (reindexerSrc.exists("unblocking") &&
                    reindexerSrc["unblocking"].isBool() &&
                    reindexerSrc["unblocking"].get_bool())
                    return PocketTxType::BLOCKING_CANCEL_ACTION;

                return PocketTxType::BLOCKING_ACTION;
            }

            if (reindexerTable == "Subscribes")
            {
                if (reindexerSrc.exists("unsubscribe") &&
                    reindexerSrc["unsubscribe"].isBool() &&
                    reindexerSrc["unsubscribe"].get_bool())
                    return PocketTxType::SUBSCRIBE_CANCEL_ACTION;

                if (reindexerSrc.exists("private") &&
                    reindexerSrc["private"].isBool() &&
                    reindexerSrc["private"].get_bool())
                    return PocketTxType::SUBSCRIBE_PRIVATE_ACTION;

                return PocketTxType::SUBSCRIBE_ACTION;
            }

            if (reindexerTable == "Complains")
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

    private:

        static PocketTxType ConvertOpReturnToType(const std::string &op)
        {
            if (op == OR_POST || op == OR_POSTEDIT)
                return PocketTxType::POST_CONTENT;
            else if (op == OR_VIDEO)
                return PocketTxType::VIDEO_CONTENT;
            else if (op == OR_SERVER_PING)
                return PocketTxType::SERVERPING_CONTENT;
            else if (op == OR_SCORE)
                return PocketTxType::SCORE_POST_ACTION;
            else if (op == OR_COMPLAIN)
                return PocketTxType::COMPLAIN_ACTION;
            else if (op == OR_SUBSCRIBE)
                return PocketTxType::SUBSCRIBE_ACTION;
            else if (op == OR_SUBSCRIBEPRIVATE)
                return PocketTxType::SUBSCRIBE_PRIVATE_ACTION;
            else if (op == OR_UNSUBSCRIBE)
                return PocketTxType::SUBSCRIBE_CANCEL_ACTION;
            else if (op == OR_USERINFO)
                return PocketTxType::USER_ACCOUNT;
            else if (op == OR_VIDEO_SERVER)
                return PocketTxType::VIDEO_SERVER_ACCOUNT;
            else if (op == OR_MESSAGE_SERVER)
                return PocketTxType::MESSAGE_SERVER_ACCOUNT;
            else if (op == OR_BLOCKING)
                return PocketTxType::BLOCKING_ACTION;
            else if (op == OR_UNBLOCKING)
                return PocketTxType::BLOCKING_CANCEL_ACTION;
            else if (op == OR_COMMENT || op == OR_COMMENT_EDIT)
                return PocketTxType::COMMENT_CONTENT;
            else if (op == OR_COMMENT_DELETE)
                return PocketTxType::COMMENT_DELETE_CONTENT;
            else if (op == OR_COMMENT_SCORE)
                return PocketTxType::SCORE_COMMENT_ACTION;

            // TODO (brangr): new types

            return PocketTxType::NOT_SUPPORTED;
        }

    };

}

#endif // POCKETTX_TRANSACTIONSERIALIZER_HPP
