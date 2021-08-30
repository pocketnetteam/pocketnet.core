// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include "pocketdb/helpers/TransactionHelper.h"

namespace PocketHelpers
{
    txnouttype TransactionHelper::ScriptType(const CScript& scriptPubKey)
    {
        std::vector<std::vector<unsigned char>> vSolutions;
        return Solver(scriptPubKey, vSolutions);
    }

    std::string TransactionHelper::ExtractDestination(const CScript& scriptPubKey)
    {
        CTxDestination destAddress;
        if (::ExtractDestination(scriptPubKey, destAddress))
            return EncodeDestination(destAddress);

        return "";
    }

    tuple<bool, string> TransactionHelper::GetPocketAuthorAddress(const CTransactionRef& tx)
    {
        if (tx->vout.size() < 2)
            return make_tuple(false, "");

        string address = ExtractDestination(tx->vout[1].scriptPubKey);
        return make_tuple(!address.empty(), address);
    }

    PocketTxType TransactionHelper::ConvertOpReturnToType(const string& op)
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

        return PocketTxType::TX_DEFAULT;
    }

    string TransactionHelper::ParseAsmType(const CTransactionRef& tx, vector<string>& vasm)
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

    PocketTxType TransactionHelper::ParseType(const CTransactionRef& tx, vector<string>& vasm)
    {
        if (tx->IsCoinBase())
        {
            int txOutSum = std::accumulate(begin(tx->vout), end(tx->vout), 0,
                [](int i, const CTxOut& o) { return o.nValue + i; });

            if (txOutSum <= 0)
                return PocketTxType::NOT_SUPPORTED;
            else
                return PocketTxType::TX_COINBASE;
        }

        if (tx->IsCoinStake())
            return PocketTxType::TX_COINSTAKE;

        return ConvertOpReturnToType(ParseAsmType(tx, vasm));
    }

    PocketTxType TransactionHelper::ParseType(const CTransactionRef& tx)
    {
        vector<string> vasm;
        return ParseType(tx, vasm);
    }

    string TransactionHelper::ConvertToReindexerTable(const Transaction& transaction)
    {
        switch (*transaction.GetType())
        {
            case PocketTxType::ACCOUNT_USER:
                return "Users";
            case PocketTxType::CONTENT_POST:
            case PocketTxType::CONTENT_VIDEO:
                return "Posts";
            case PocketTxType::CONTENT_COMMENT:
            case PocketTxType::CONTENT_COMMENT_EDIT:
            case PocketTxType::CONTENT_COMMENT_DELETE:
                return "Comment";
            case PocketTxType::ACTION_SCORE_CONTENT:
                return "Scores";
            case PocketTxType::ACTION_SCORE_COMMENT:
                return "CommentScores";
            case PocketTxType::ACTION_COMPLAIN:
                return "Complains";
            case PocketTxType::ACTION_BLOCKING_CANCEL:
            case PocketTxType::ACTION_BLOCKING:
                return "Blocking";
            case PocketTxType::ACTION_SUBSCRIBE_CANCEL:
            case PocketTxType::ACTION_SUBSCRIBE_PRIVATE:
            case PocketTxType::ACTION_SUBSCRIBE:
                return "Subscribes";
            default:
                return "";
        }
    }

    bool TransactionHelper::IsPocketSupportedTransaction(const CTransactionRef& tx, PocketTxType& txType)
    {
        txType = ParseType(tx);
        return txType != NOT_SUPPORTED;
    }

    bool TransactionHelper::IsPocketSupportedTransaction(const CTransactionRef& tx)
    {
        PocketTxType txType = NOT_SUPPORTED;
        return IsPocketSupportedTransaction(tx, txType);
    }

    bool TransactionHelper::IsPocketTransaction(PocketTxType& txType)
    {
        return txType != NOT_SUPPORTED &&
               txType != TX_COINBASE &&
               txType != TX_COINSTAKE &&
               txType != TX_DEFAULT;
    }

    bool TransactionHelper::IsPocketTransaction(const CTransactionRef& tx, PocketTxType& txType)
    {
        txType = ParseType(tx);
        return IsPocketTransaction(txType);
    }

    bool TransactionHelper::IsPocketTransaction(const CTransactionRef& tx)
    {
        PocketTxType txType = NOT_SUPPORTED;
        return IsPocketTransaction(tx, txType);
    }

    bool TransactionHelper::IsPocketTransaction(const CTransaction& tx)
    {
        auto txRef = MakeTransactionRef(tx);
        return IsPocketTransaction(txRef);
    }

    tuple<bool, shared_ptr<ScoreDataDto>> TransactionHelper::ParseScore(const CTransactionRef& tx)
    {
        shared_ptr<ScoreDataDto> scoreData = make_shared<ScoreDataDto>();

        vector<string> vasm;
        scoreData->ScoreType = ParseType(tx, vasm);

        if (scoreData->ScoreType != PocketTxType::ACTION_SCORE_CONTENT &&
            scoreData->ScoreType != PocketTxType::ACTION_SCORE_COMMENT)
            return make_tuple(false, scoreData);

        if (vasm.size() >= 4)
        {
            vector<unsigned char> _data_hex = ParseHex(vasm[3]);
            string _data_str(_data_hex.begin(), _data_hex.end());
            vector<string> _data;
            boost::split(_data, _data_str, boost::is_any_of("\t "));

            if (_data.size() >= 2)
            {
                if (auto[ok, addr] = GetPocketAuthorAddress(tx); ok)
                    scoreData->ScoreAddressHash = addr;

                scoreData->ContentAddressHash = _data[0];
                scoreData->ScoreValue = std::stoi(_data[1]);
            }
        }

        bool finalCheck = !scoreData->ScoreAddressHash.empty() &&
                          !scoreData->ContentAddressHash.empty();

        scoreData->ScoreTxHash = tx->GetHash().GetHex();
        return make_tuple(finalCheck, scoreData);
    }

    PTransactionRef TransactionHelper::CreateInstance(PocketTxType txType, const std::string& txHash, uint32_t nTime)
    {
        PTransactionRef ptx = nullptr;
        switch (txType)
        {
            case TX_COINBASE:
                ptx = make_shared<Coinbase>(txHash, nTime);
                break;
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