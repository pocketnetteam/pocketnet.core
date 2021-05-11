// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 Bitcoin developers
// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETTX_TRANSACTIONSERIALIZER_HPP
#define POCKETTX_TRANSACTIONSERIALIZER_HPP

#include "pocketdb/models/base/Transaction.hpp"
#include "pocketdb/models/base/Coinbase.hpp"
#include "pocketdb/models/base/Coinstake.hpp"
#include "pocketdb/models/base/Default.hpp"
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

#include "streams.h"

namespace PocketServices
{
    using namespace PocketTx;

    class TransactionSerializer
    {
    public:

        template<typename Stream>
        static void DeserializeBlock(Stream& stream, CBlock& block)
        {
            // Prepare source data - old format (Json)
            // TODO: speed up protocol
            string src;
            stream >> src;

            // Restore json
            UniValue pocketData(UniValue::VOBJ);
            pocketData.read(src);

            // Restore pocket transaction instance
            for (const auto& tx : block.vtx) {
                auto txHash = tx->GetHash().GetHex();

                UniValue entry(UniValue::VOBJ);
                if (pocketData.exists(txHash)) {
                    auto entrySrc = pocketData[txHash];

                    try
                    {
                        entry.read(entrySrc.get_str());
                    }
                    catch (std::exception &ex)
                    {
                        LogPrintf("Error deserialize transaction: %s: %s\n", txHash, ex.what());
                    }
                }
                
                tx.ptx = BuildInstance(tx, entry);
            }
        }

    private:

        static PocketTxType ConvertOpReturnToType(const std::string &op)
        {
            if (op == OR_POST || op == OR_POSTEDIT)
                return PocketTxType::CONTENT_POST;
            else if (op == OR_VIDEO)
                return PocketTxType::CONTENT_VIDEO;
            else if (op == OR_SERVER_PING)
                return PocketTxType::CONTENT_SERVERPING;
            else if (op == OR_SCORE)
                return PocketTxType::ACTION_SCORE_POST;
            else if (op == OR_COMPLAIN)
                return PocketTxType::ACTION_COMPLAIN;
            else if (op == OR_SUBSCRIBE)
                return PocketTxType::ACTION_SUBSCRIBE;
            else if (op == OR_SUBSCRIBEPRIVATE)
                return PocketTxType::ACTION_SUBSCRIBE_PRIVATE;
            else if (op == OR_UNSUBSCRIBE)
                return PocketTxType::ACTION_SUBSCRIBE_CANCEL;
            else if (op == OR_USERINFO)
                return PocketTxType::ACCOUNT_USER;
            else if (op == OR_VIDEO_SERVER)
                return PocketTxType::ACCOUNT_VIDEO_SERVER;
            else if (op == OR_MESSAGE_SERVER)
                return PocketTxType::ACCOUNT_MESSAGE_SERVER;
            else if (op == OR_BLOCKING)
                return PocketTxType::ACTION_BLOCKING;
            else if (op == OR_UNBLOCKING)
                return PocketTxType::ACTION_BLOCKING_CANCEL;
            else if (op == OR_COMMENT || op == OR_COMMENT_EDIT)
                return PocketTxType::CONTENT_COMMENT;
            else if (op == OR_COMMENT_DELETE)
                return PocketTxType::CONTENT_COMMENT_DELETE;
            else if (op == OR_COMMENT_SCORE)
                return PocketTxType::ACTION_SCORE_COMMENT;

            // TODO (brangr): new types

            return PocketTxType::NOT_SUPPORTED;
        }

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

            if (tx->IsCoinBase())
                return PocketTxType::TX_COINBASE;

            if (tx->IsCoinStake())
                return PocketTxType::TX_COINSTAKE;

            return PocketTxType::TX_DEFAULT;
        }

        static PocketTxType ParseType(const std::string &reindexerTable, const UniValue &reindexerSrc)
        {
            // TODO (brangr): implement enum for tx types
            if (reindexerTable == "Users")
                return PocketTxType::ACCOUNT_USER;

            if (reindexerTable == "Posts")
                return PocketTxType::CONTENT_POST;

            if (reindexerTable == "Comment")
                return PocketTxType::CONTENT_COMMENT;

            if (reindexerTable == "Scores")
                return PocketTxType::ACTION_SCORE_POST;

            if (reindexerTable == "CommentScores")
                return PocketTxType::ACTION_SCORE_COMMENT;

            if (reindexerTable == "Blocking")
            {
                if (reindexerSrc.exists("unblocking") &&
                    reindexerSrc["unblocking"].isBool() &&
                    reindexerSrc["unblocking"].get_bool())
                    return PocketTxType::ACTION_BLOCKING_CANCEL;

                return PocketTxType::ACTION_BLOCKING;
            }

            if (reindexerTable == "Subscribes")
            {
                if (reindexerSrc.exists("unsubscribe") &&
                    reindexerSrc["unsubscribe"].isBool() &&
                    reindexerSrc["unsubscribe"].get_bool())
                    return PocketTxType::ACTION_SUBSCRIBE_CANCEL;

                if (reindexerSrc.exists("private") &&
                    reindexerSrc["private"].isBool() &&
                    reindexerSrc["private"].get_bool())
                    return PocketTxType::ACTION_SUBSCRIBE_PRIVATE;

                return PocketTxType::ACTION_SUBSCRIBE;
            }

            if (reindexerTable == "Complains")
                return PocketTxType::ACTION_COMPLAIN;

            return PocketTxType::NOT_SUPPORTED;
        }

        static shared_ptr<Transaction> BuildInstance(const CTransactionRef &tx, const UniValue &src)
        {
            // TODO (brangr): change detect type
            auto txTypeSrc = src["t"].get_str();

            UniValue txDataSrc(UniValue::VOBJ);
            auto txDataBase64 = src["d"].get_str();
            auto txJson = DecodeBase64(txDataBase64);
            txDataSrc.read(txJson);

            shared_ptr<Transaction> tx;
            PocketTxType txType = ParseType(txTypeSrc, txDataSrc);
            switch (txType)
            {
                case ACCOUNT_USER:
                    tx = make_shared<User>();
                    break;
                case ACCOUNT_VIDEO_SERVER:
                case ACCOUNT_MESSAGE_SERVER:
                    break;
                case CONTENT_POST:
                    tx = make_shared<Post>();
                    break;
                case CONTENT_VIDEO:
                case CONTENT_TRANSLATE:
                case CONTENT_SERVERPING:
                    break;
                case CONTENT_COMMENT:
                    tx = make_shared<Comment>();
                    break;
                case ACTION_SCORE_POST:
                    tx = make_shared<ScorePost>();
                    break;
                case ACTION_SCORE_COMMENT:
                    tx = make_shared<ScoreComment>();
                    break;
                case ACTION_SUBSCRIBE:
                    tx = make_shared<Subscribe>();
                    break;
                case ACTION_SUBSCRIBE_PRIVATE:
                    tx = make_shared<SubscribePrivate>();
                    break;
                case ACTION_SUBSCRIBE_CANCEL:
                    tx = make_shared<SubscribeCancel>();
                    break;
                case ACTION_BLOCKING:
                    tx = make_shared<Blocking>();
                    break;
                case ACTION_BLOCKING_CANCEL:
                    tx = make_shared<BlockingCancel>();
                    break;
                case ACTION_COMPLAIN:
                    tx = make_shared<Complain>();
                    break;

                case TX_COINBASE:
                    tx = make_shared<Coinbase>();
                    break;
                case TX_COINSTAKE:
                    tx = make_shared<Coinstake>();
                    break;
                case TX_DEFAULT:
                    tx = make_shared<Default>();
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

    };

}

#endif // POCKETTX_TRANSACTIONSERIALIZER_HPP
