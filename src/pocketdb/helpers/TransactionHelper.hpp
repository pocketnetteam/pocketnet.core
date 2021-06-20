// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 Bitcoin developers
// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETHELPERS_TRANSACTIONHELPER_HPP
#define POCKETHELPERS_TRANSACTIONHELPER_HPP

#include <string>
#include <key_io.h>
#include <boost/algorithm/string.hpp>

#include "primitives/transaction.h"

#include "pocketdb/models/base/ReturnDtoModels.hpp"
#include "pocketdb/models/base/PocketTypes.hpp"

#include "pocketdb/models/dto/Blocking.hpp"
#include "pocketdb/models/dto/BlockingCancel.hpp"
#include "pocketdb/models/dto/Coinstake.hpp"
#include "pocketdb/models/dto/Default.hpp"
#include "pocketdb/models/dto/Post.hpp"
#include "pocketdb/models/dto/Video.hpp"
#include "pocketdb/models/dto/Comment.hpp"
#include "pocketdb/models/dto/CommentEdit.hpp"
#include "pocketdb/models/dto/CommentDelete.hpp"
#include "pocketdb/models/dto/Subscribe.hpp"
#include "pocketdb/models/dto/SubscribeCancel.hpp"
#include "pocketdb/models/dto/SubscribePrivate.hpp"
#include "pocketdb/models/dto/Complain.hpp"
#include "pocketdb/models/dto/User.hpp"
#include "pocketdb/models/dto/ScoreContent.hpp"
#include "pocketdb/models/dto/ScoreComment.hpp"

namespace PocketHelpers
{
    using std::tuple;
    using std::string;
    using std::vector;
    using std::make_tuple;
    using std::shared_ptr;
    using std::map;

    using namespace PocketTx;

    // Accumulate transactions in block
    typedef vector<shared_ptr<Transaction>> PocketBlock;

    static txnouttype ScriptType(const CScript& scriptPubKey)
    {
        std::vector<std::vector<unsigned char>> vSolutions;
        return Solver(scriptPubKey, vSolutions);
    }

    static std::string ExtractDestination(const CScript& scriptPubKey)
    {
        CTxDestination destAddress;
        if (ExtractDestination(scriptPubKey, destAddress))
            return EncodeDestination(destAddress);

        return "";
    }

    static tuple<bool, string> GetPocketAuthorAddress(const CTransactionRef& tx)
    {
        if (tx->vout.size() < 2)
            return make_tuple(false, "");

        string address = ExtractDestination(tx->vout[1].scriptPubKey);
        return make_tuple(!address.empty(), address);
    }

    static PocketTxType ConvertOpReturnToType(const string& op)
    {
        if (op == OR_POST || op == OR_POSTEDIT)
            return PocketTxType::CONTENT_POST;
        else if (op == OR_VIDEO)
            return PocketTxType::CONTENT_VIDEO;
        else if (op == OR_SERVER_PING)
            return PocketTxType::CONTENT_SERVERPING;
        else if (op == OR_SCORE)
            return PocketTxType::ACTION_SCORE_CONTENT;
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
        else if (op == OR_COMMENT)
            return PocketTxType::CONTENT_COMMENT;
        else if (op == OR_COMMENT_EDIT)
            return PocketTxType::CONTENT_COMMENT_EDIT;
        else if (op == OR_COMMENT_DELETE)
            return PocketTxType::CONTENT_COMMENT_DELETE;
        else if (op == OR_COMMENT_SCORE)
            return PocketTxType::ACTION_SCORE_COMMENT;

        // TODO (brangr): new types

        return PocketTxType::TX_DEFAULT;
    }

    static string ParseAsmType(const CTransactionRef& tx, vector<string>& vasm)
    {
        if (tx->vout.empty())
            return "";

        const CTxOut& txout = tx->vout[0];
        if (txout.scriptPubKey[0] == OP_RETURN)
        {
            auto asmStr = ScriptToAsmStr(txout.scriptPubKey);
            boost::split(vasm, asmStr, boost::is_any_of("\t "));
            if (vasm.size() >= 2)
                return vasm[1];
        }

        return "";
    }

    static PocketTxType ParseType(const CTransactionRef& tx, vector<string>& vasm)
    {
        if (tx->vin.empty())
            return PocketTxType::NOT_SUPPORTED;

        if (tx->IsCoinBase())
            return PocketTxType::TX_COINBASE;

        if (tx->IsCoinStake())
            return PocketTxType::TX_COINSTAKE;

        return ConvertOpReturnToType(ParseAsmType(tx, vasm));
    }

    static PocketTxType ParseType(const CTransactionRef& tx)
    {
        vector<string> vasm;
        return ParseType(tx, vasm);
    }

    static string ConvertToReindexerTable(const Transaction& transaction)
    {
        switch (*transaction.GetType())
        {
            case PocketTxType::ACCOUNT_USER:
                return "Users";
                break;
            case PocketTxType::CONTENT_POST:
            case PocketTxType::CONTENT_VIDEO:
                return "Posts";
                break;
            case PocketTxType::CONTENT_COMMENT:
            case PocketTxType::CONTENT_COMMENT_EDIT:
            case PocketTxType::CONTENT_COMMENT_DELETE:
                return "Comment";
                break;
            case PocketTxType::ACTION_SCORE_CONTENT:
                return "Scores";
                break;
            case PocketTxType::ACTION_SCORE_COMMENT:
                return "CommentScores";
                break;
            case PocketTxType::ACTION_COMPLAIN:
                return "Complains";
                break;
            case PocketTxType::ACTION_BLOCKING_CANCEL:
            case PocketTxType::ACTION_BLOCKING:
                return "Blocking";
                break;
            case PocketTxType::ACTION_SUBSCRIBE_CANCEL:
            case PocketTxType::ACTION_SUBSCRIBE_PRIVATE:
            case PocketTxType::ACTION_SUBSCRIBE:
                return "Subscribes";
                break;
            default:
                return "";
        };
    }

    static bool IsPocketTransaction(const CTransactionRef& tx)
    {
        auto txType = ParseType(tx);

        return txType != NOT_SUPPORTED &&
               txType != TX_COINBASE &&
               txType != TX_COINSTAKE &&
               txType != TX_DEFAULT;
    }

    static tuple<bool, ScoreDataDto> ParseScore(const CTransactionRef& tx)
    {
        ScoreDataDto scoreData;

        vector<string> vasm;
        scoreData.ScoreType = PocketHelpers::ParseType(tx, vasm);

        if (scoreData.ScoreType != PocketTxType::ACTION_SCORE_CONTENT &&
            scoreData.ScoreType != PocketTxType::ACTION_SCORE_COMMENT)
            return make_tuple(false, scoreData);

        if (vasm.size() == 4)
        {
            vector<unsigned char> _data_hex = ParseHex(vasm[3]);
            string _data_str(_data_hex.begin(), _data_hex.end());
            vector<string> _data;
            boost::split(_data, _data_str, boost::is_any_of("\t "));

            if (_data.size() >= 2)
            {
                if (auto[ok, addr] = PocketHelpers::GetPocketAuthorAddress(tx); ok)
                    scoreData.ScoreAddressHash = addr;

                scoreData.ContentAddressHash = _data[0];
                scoreData.ScoreValue = std::stoi(_data[1]);
            }
        }

        bool finalCheck = !scoreData.ScoreAddressHash.empty() &&
                          !scoreData.ContentAddressHash.empty();

        scoreData.ScoreTxHash = tx->GetHash().GetHex();
        return make_tuple(finalCheck, scoreData);
    }

    static shared_ptr<Transaction> CreateInstance(PocketTxType txType, std::string txHash, uint32_t nTime)
    {
        shared_ptr<Transaction> ptx = nullptr;
        switch (txType)
        {
            case TX_COINBASE:
            case TX_COINSTAKE:
                ptx = make_shared<Coinstake>(txHash, nTime);
                break;
            case TX_DEFAULT:
                ptx = make_shared<Default>(txHash, nTime);
                break;
            case ACCOUNT_USER:
                ptx = make_shared<User>(txHash, nTime);
                break;
            case CONTENT_POST:
                ptx = make_shared<Post>(txHash, nTime);
                break;
            case CONTENT_VIDEO:
                ptx = make_shared<Video>(txHash, nTime);
                break;
            case CONTENT_COMMENT:
                ptx = make_shared<Comment>(txHash, nTime);
                break;
            case CONTENT_COMMENT_EDIT:
                ptx = make_shared<CommentEdit>(txHash, nTime);
                break;
            case CONTENT_COMMENT_DELETE:
                ptx = make_shared<CommentDelete>(txHash, nTime);
                break;
            case ACTION_SCORE_CONTENT:
                ptx = make_shared<ScoreContent>(txHash, nTime);
                break;
            case ACTION_SCORE_COMMENT:
                ptx = make_shared<ScoreComment>(txHash, nTime);
                break;
            case ACTION_SUBSCRIBE:
                ptx = make_shared<Subscribe>(txHash, nTime);
                break;
            case ACTION_SUBSCRIBE_PRIVATE:
                ptx = make_shared<SubscribePrivate>(txHash, nTime);
                break;
            case ACTION_SUBSCRIBE_CANCEL:
                ptx = make_shared<SubscribeCancel>(txHash, nTime);
                break;
            case ACTION_BLOCKING:
                ptx = make_shared<Blocking>(txHash, nTime);
                break;
            case ACTION_BLOCKING_CANCEL:
                ptx = make_shared<BlockingCancel>(txHash, nTime);
                break;
            case ACTION_COMPLAIN:
                ptx = make_shared<Complain>(txHash, nTime);
                break;
            default:
                return nullptr;
        }

        return ptx;
    }
}

#endif // POCKETHELPERS_TRANSACTIONHELPER_HPP
