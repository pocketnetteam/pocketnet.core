// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 Bitcoin developers
// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETHELPERS_TRANSACTIONHELPER_HPP
#define POCKETHELPERS_TRANSACTIONHELPER_HPP

#include <string>
#include <key_io.h>
#include "primitives/transaction.h"

#include "pocketdb/models/base/PocketTypes.hpp"

namespace PocketHelpers
{
    using std::tuple;
    using std::string;
    using std::vector;
    using std::make_tuple;

    using namespace PocketTx;

    static tuple<bool, string> GetPocketAuthorAddress(const CTransactionRef& tx)
    {
        if (tx->vout.empty())
            return make_tuple(false, "");

        CTxDestination address;
        if (ExtractDestination(tx->vout[0].scriptPubKey, address))
        {
            return make_tuple(true, EncodeDestination(address));
        }

        return make_tuple(false, "");
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

    static PocketTxType ParseType(const string& reindexerTable, const UniValue& reindexerSrc)
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

        if (scoreData.ScoreType != PocketTxType::ACTION_SCORE_POST &&
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
}

#endif // POCKETHELPERS_TRANSACTIONHELPER_HPP
