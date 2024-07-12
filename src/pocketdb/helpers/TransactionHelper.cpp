// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include "pocketdb/helpers/TransactionHelper.h"
#include "core_io.h"

namespace PocketHelpers
{
    TxoutType TransactionHelper::ScriptType(const CScript& scriptPubKey)
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

    TxType TransactionHelper::ConvertOpReturnToType(const string& op)
    {
        if (op == OR_POST || op == OR_POSTEDIT)
            return TxType::CONTENT_POST;
        else if (op == OR_VIDEO)
            return TxType::CONTENT_VIDEO;
        else if (op == OR_ARTICLE)
            return TxType::CONTENT_ARTICLE;
        else if (op == OR_STREAM)
            return TxType::CONTENT_STREAM;
        else if (op == OR_AUDIO)
            return TxType::CONTENT_AUDIO;

        else if (op == OR_COLLECTION)
            return TxType::CONTENT_COLLECTION;
            
        else if (op == OR_APP)
            return TxType::APP;
            
        else if (op == OR_CONTENT_BOOST)
            return TxType::BOOST_CONTENT;
        else if (op == OR_CONTENT_DELETE)
            return TxType::CONTENT_DELETE;

        else if (op == OR_SUBSCRIBE)
            return TxType::ACTION_SUBSCRIBE;
        else if (op == OR_SUBSCRIBEPRIVATE)
            return TxType::ACTION_SUBSCRIBE_PRIVATE;
        else if (op == OR_UNSUBSCRIBE)
            return TxType::ACTION_SUBSCRIBE_CANCEL;

        else if (op == OR_USERINFO)
            return TxType::ACCOUNT_USER;
        else if (op == OR_ACCOUNT_SETTING)
            return TxType::ACCOUNT_SETTING;
        else if (op == OR_ACCOUNT_DELETE)
            return TxType::ACCOUNT_DELETE;

        else if (op == OR_BLOCKING)
            return TxType::ACTION_BLOCKING;
        else if (op == OR_UNBLOCKING)
            return TxType::ACTION_BLOCKING_CANCEL;

        else if (op == OR_COMMENT)
            return TxType::CONTENT_COMMENT;
        else if (op == OR_COMMENT_EDIT)
            return TxType::CONTENT_COMMENT_EDIT;
        else if (op == OR_COMMENT_DELETE)
            return TxType::CONTENT_COMMENT_DELETE;

        else if (op == OR_COMMENT_SCORE)
            return TxType::ACTION_SCORE_COMMENT;
        else if (op == OR_SCORE)
            return TxType::ACTION_SCORE_CONTENT;

        // MODERATION
        else if (op == OR_COMPLAIN)
            return TxType::ACTION_COMPLAIN;
        else if (op == OR_MODERATION_FLAG)
            return TxType::MODERATION_FLAG;
        else if (op == OR_MODERATION_VOTE)
            return TxType::MODERATION_VOTE;

        // BARTERON
        else if (op == OR_BARTERON_ACCOUNT)
            return TxType::BARTERON_ACCOUNT;
        else if (op == OR_BARTERON_OFFER)
            return TxType::BARTERON_OFFER;

        return TxType::TX_DEFAULT;
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

    TxType TransactionHelper::ParseType(const CTransactionRef& tx, vector<string>& vasm)
    {
        if (tx->IsCoinBase())
        {
            int txOutSum = std::accumulate(begin(tx->vout), end(tx->vout), 0,
                [](int i, const CTxOut& o) { return o.nValue + i; });

            if (txOutSum <= 0)
                return TxType::NOT_SUPPORTED;
            else
                return TxType::TX_COINBASE;
        }

        if (tx->IsCoinStake())
            return TxType::TX_COINSTAKE;

        return ConvertOpReturnToType(ParseAsmType(tx, vasm));
    }

    TxType TransactionHelper::ParseType(const CTransactionRef& tx)
    {
        vector<string> vasm;
        return ParseType(tx, vasm);
    }

    string TransactionHelper::ConvertToReindexerTable(const Transaction& transaction)
    {
        // TODO (aok) (v0.21.0): need remove for next generation serialization
        switch (*transaction.GetType())
        {
            case TxType::ACCOUNT_USER:
                return "Users";
            case TxType::ACCOUNT_SETTING:
                return "AccountSettings";
            case TxType::ACCOUNT_DELETE:
                return "AccountDelete";
            case TxType::CONTENT_POST:
            case TxType::CONTENT_VIDEO:
            case TxType::CONTENT_ARTICLE:
            case TxType::CONTENT_STREAM:
            case TxType::CONTENT_AUDIO:
            case TxType::CONTENT_DELETE:
                return "Posts";
            case TxType::CONTENT_COLLECTION:
                return "Collection";
            case TxType::CONTENT_COMMENT:
            case TxType::CONTENT_COMMENT_EDIT:
            case TxType::CONTENT_COMMENT_DELETE:
                return "Comment";
            case TxType::APP:
                return "App";
            case TxType::ACTION_SCORE_CONTENT:
                return "Scores";
            case TxType::ACTION_SCORE_COMMENT:
                return "CommentScores";
            case TxType::ACTION_COMPLAIN:
                return "Complains";
            case TxType::ACTION_BLOCKING_CANCEL:
            case TxType::ACTION_BLOCKING:
                return "Blocking";
            case TxType::ACTION_SUBSCRIBE_CANCEL:
            case TxType::ACTION_SUBSCRIBE_PRIVATE:
            case TxType::ACTION_SUBSCRIBE:
                return "Subscribes";
            case TxType::BOOST_CONTENT:
                return "Boosts";
            case TxType::MODERATION_FLAG:
                return "ModFlag";
            case TxType::MODERATION_VOTE:
                return "ModVote";
            case TxType::BARTERON_ACCOUNT:
                return "BrtAccount";
            case TxType::BARTERON_OFFER:
                return "BrtOffer";
            default:
                return "";
        }
    }

    string TransactionHelper::ExtractOpReturnHash(const CTransactionRef& tx)
    {
        vector<string> vasm;
        ParseAsmType(tx, vasm);
        
        if (vasm.size() < 3)
            return "";

        return vasm[2];
    }

    tuple<bool, string> TransactionHelper::ExtractOpReturnPayload(const CTransactionRef& tx)
    {
        vector<string> vasm;
        ParseAsmType(tx, vasm);

        if (vasm.size() < 4)
            return { false, "" };

        return { !vasm[3].empty(), vasm[3] };
    }

    bool TransactionHelper::IsPocketSupportedTransaction(const CTransactionRef& tx, TxType& txType)
    {
        txType = ParseType(tx);
        return txType != NOT_SUPPORTED;
    }

    bool TransactionHelper::IsPocketSupportedTransaction(const CTransactionRef& tx)
    {
        TxType txType = NOT_SUPPORTED;
        return IsPocketSupportedTransaction(tx, txType);
    }

    bool TransactionHelper::IsPocketSupportedTransaction(const CTransaction& tx)
    {
        auto txRef = MakeTransactionRef(tx);
        return IsPocketSupportedTransaction(txRef);
    }

    bool TransactionHelper::IsPocketTransaction(TxType& txType)
    {
        return txType != NOT_SUPPORTED &&
               txType != TX_COINBASE &&
               txType != TX_COINSTAKE &&
               txType != TX_DEFAULT;
    }

    bool TransactionHelper::IsPocketTransaction(const CTransactionRef& tx, TxType& txType)
    {
        txType = ParseType(tx);
        return IsPocketTransaction(txType);
    }

    bool TransactionHelper::IsPocketTransaction(const CTransactionRef& tx)
    {
        TxType txType = NOT_SUPPORTED;
        return IsPocketTransaction(tx, txType);
    }

    bool TransactionHelper::IsPocketTransaction(const CTransaction& tx)
    {
        auto txRef = MakeTransactionRef(tx);
        return IsPocketTransaction(txRef);
    }

    // TODO (o1q): Implement it for setting minimum fee for several PocketNet transactions
    bool TransactionHelper::IsPocketNeededPaymentTransaction(const CTransactionRef& tx)
    {
        return false;
    }

    tuple<bool, shared_ptr<ScoreDataDto>> TransactionHelper::ParseScore(const CTransactionRef& tx)
    {
        shared_ptr<ScoreDataDto> scoreData = make_shared<ScoreDataDto>();

        vector<string> vasm;
        scoreData->ScoreType = ParseType(tx, vasm);

        if (scoreData->ScoreType != TxType::ACTION_SCORE_CONTENT &&
            scoreData->ScoreType != TxType::ACTION_SCORE_COMMENT)
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

    PTransactionRef TransactionHelper::CreateInstance(TxType txType, const CTransactionRef& tx)
    {
        PTransactionRef ptx = nullptr;
        switch (txType)
        {
            case TX_COINBASE:
                ptx = make_shared<Coinbase>(tx);
                break;
            case TX_COINSTAKE:
                ptx = make_shared<Coinstake>(tx);
                break;
            case TX_DEFAULT:
                ptx = make_shared<Default>(tx);
                break;
            case ACCOUNT_SETTING:
                ptx = make_shared<AccountSetting>(tx);
                break;
            case ACCOUNT_DELETE:
                ptx = make_shared<AccountDelete>(tx);
                break;
            case ACCOUNT_USER:
                ptx = make_shared<User>(tx);
                break;
            case CONTENT_POST:
                ptx = make_shared<Post>(tx);
                break;
            case CONTENT_VIDEO:
                ptx = make_shared<Video>(tx);
                break;
            case CONTENT_ARTICLE:
                ptx = make_shared<Article>(tx);
                break;
            case CONTENT_STREAM:
                ptx = make_shared<Stream>(tx);
                break;
            case CONTENT_AUDIO:
                ptx = make_shared<Audio>(tx);
                break;
            case CONTENT_COLLECTION:
                ptx = make_shared<Collection>(tx);
                break;
            case APP:
                ptx = make_shared<App>(tx);
                break;
            case CONTENT_DELETE:
                ptx = make_shared<ContentDelete>(tx);
                break;
            case BOOST_CONTENT:
                ptx = make_shared<BoostContent>(tx);
                break;
            case CONTENT_COMMENT:
                ptx = make_shared<Comment>(tx);
                break;
            case CONTENT_COMMENT_EDIT:
                ptx = make_shared<CommentEdit>(tx);
                break;
            case CONTENT_COMMENT_DELETE:
                ptx = make_shared<CommentDelete>(tx);
                break;
            case ACTION_SCORE_CONTENT:
                ptx = make_shared<ScoreContent>(tx);
                break;
            case ACTION_SCORE_COMMENT:
                ptx = make_shared<ScoreComment>(tx);
                break;
            case ACTION_SUBSCRIBE:
                ptx = make_shared<Subscribe>(tx);
                break;
            case ACTION_SUBSCRIBE_PRIVATE:
                ptx = make_shared<SubscribePrivate>(tx);
                break;
            case ACTION_SUBSCRIBE_CANCEL:
                ptx = make_shared<SubscribeCancel>(tx);
                break;
            case ACTION_BLOCKING:
                ptx = make_shared<Blocking>(tx);
                break;
            case ACTION_BLOCKING_CANCEL:
                ptx = make_shared<BlockingCancel>(tx);
                break;
            case ACTION_COMPLAIN:
                ptx = make_shared<Complain>(tx);
                break;
            case MODERATION_FLAG:
                ptx = make_shared<ModerationFlag>(tx);
                break;
            case MODERATION_VOTE:
                ptx = make_shared<ModerationVote>(tx);
                break;
            case BARTERON_ACCOUNT:
                ptx = make_shared<BarteronAccount>(tx);
                break;
            case BARTERON_OFFER:
                ptx = make_shared<BarteronOffer>(tx);
                break;
            default:
                return nullptr;
        }

        return ptx;
    }

    PTransactionRef TransactionHelper::CreateInstance(TxType txType)
    {
        PTransactionRef ptx = nullptr;
        switch (txType)
        {
            case TX_COINBASE:
                ptx = make_shared<Coinbase>();
                break;
            case TX_COINSTAKE:
                ptx = make_shared<Coinstake>();
                break;
            case TX_DEFAULT:
                ptx = make_shared<Default>();
                break;
            case ACCOUNT_SETTING:
                ptx = make_shared<AccountSetting>();
                break;
            case ACCOUNT_DELETE:
                ptx = make_shared<AccountDelete>();
                break;
            case ACCOUNT_USER:
                ptx = make_shared<User>();
                break;
            case CONTENT_POST:
                ptx = make_shared<Post>();
                break;
            case CONTENT_VIDEO:
                ptx = make_shared<Video>();
                break;
            case CONTENT_ARTICLE:
                ptx = make_shared<Article>();
                break;
            case CONTENT_STREAM:
                ptx = make_shared<Stream>();
                break;
            case CONTENT_AUDIO:
                ptx = make_shared<Audio>();
                break;
            case CONTENT_COLLECTION:
                ptx = make_shared<Collection>();
                break;
            case APP:
                ptx = make_shared<App>();
                break;
            case CONTENT_DELETE:
                ptx = make_shared<ContentDelete>();
                break;
            case BOOST_CONTENT:
                ptx = make_shared<BoostContent>();
                break;
            case CONTENT_COMMENT:
                ptx = make_shared<Comment>();
                break;
            case CONTENT_COMMENT_EDIT:
                ptx = make_shared<CommentEdit>();
                break;
            case CONTENT_COMMENT_DELETE:
                ptx = make_shared<CommentDelete>();
                break;
            case ACTION_SCORE_CONTENT:
                ptx = make_shared<ScoreContent>();
                break;
            case ACTION_SCORE_COMMENT:
                ptx = make_shared<ScoreComment>();
                break;
            case ACTION_SUBSCRIBE:
                ptx = make_shared<Subscribe>();
                break;
            case ACTION_SUBSCRIBE_PRIVATE:
                ptx = make_shared<SubscribePrivate>();
                break;
            case ACTION_SUBSCRIBE_CANCEL:
                ptx = make_shared<SubscribeCancel>();
                break;
            case ACTION_BLOCKING:
                ptx = make_shared<Blocking>();
                break;
            case ACTION_BLOCKING_CANCEL:
                ptx = make_shared<BlockingCancel>();
                break;
            case ACTION_COMPLAIN:
                ptx = make_shared<Complain>();
                break;
            case MODERATION_FLAG:
                ptx = make_shared<ModerationFlag>();
                break;
            case MODERATION_VOTE:
                ptx = make_shared<ModerationVote>();
                break;
            case BARTERON_ACCOUNT:
                ptx = make_shared<BarteronAccount>();
                break;
            case BARTERON_OFFER:
                ptx = make_shared<BarteronOffer>();
                break;
            default:
                return nullptr;
        }

        return ptx;
    }

    bool TransactionHelper::IsIn(TxType txType, const vector<TxType>& inTypes)
    {
        for (auto inType : inTypes)
            if (inType == txType)
                return true;

        return false;
    }

    string TransactionHelper::TxStringType(TxType type)
    {
        switch (type)
        {
            case CONTENT_DELETE:
                return "contentDelete";
            case CONTENT_POST:
                return "share";
            case CONTENT_VIDEO:
                return "video";
            case CONTENT_ARTICLE:
                return "article";
            case CONTENT_STREAM:
                return "stream";
            case CONTENT_AUDIO:
                return "audio";
            case CONTENT_COLLECTION:
                return "collection";
            case APP:
                return "app";
            case ACTION_SCORE_CONTENT:
                return "upvoteShare";
            case ACTION_SUBSCRIBE:
                return "subscribe";
            case ACTION_SUBSCRIBE_PRIVATE:
                return "subscribePrivate";
            case ACTION_SUBSCRIBE_CANCEL:
                return "unsubscribe";
            case ACCOUNT_USER:
                return "userInfo";
            case ACCOUNT_SETTING:
                return "accSet";
            case ACCOUNT_DELETE:
                return "accDel";
            case CONTENT_COMMENT:
                return "comment";
            case CONTENT_COMMENT_EDIT:
                return "commentEdit";
            case CONTENT_COMMENT_DELETE:
                return "commentDelete";
            case ACTION_SCORE_COMMENT:
                return "cScore";
            case BOOST_CONTENT:
                return "contentBoost";
            case MODERATION_FLAG:
                return "modFlag";
            case MODERATION_VOTE:
                return "modVote";
            case BARTERON_ACCOUNT:
                return "brtaccount";
            case BARTERON_OFFER:
                return "brtoffer";
            default:
                return "";
        }
    }

    TxType TransactionHelper::TxIntType(const string& type)
    {
        if (type == "contentDelete" || type == OR_CONTENT_DELETE) return TxType::CONTENT_DELETE;
        else if (type == "share" || type == "shareEdit" || type == OR_POST || type == OR_POSTEDIT) return TxType::CONTENT_POST;
        else if (type == "video" || type == OR_VIDEO) return TxType::CONTENT_VIDEO;
        else if (type == "article" || type == OR_ARTICLE) return TxType::CONTENT_ARTICLE;
        else if (type == "stream" || type == OR_STREAM) return TxType::CONTENT_STREAM;
        else if (type == "audio" || type == OR_AUDIO) return TxType::CONTENT_AUDIO;
        else if (type == "collection" || type == OR_COLLECTION) return TxType::CONTENT_COLLECTION;
        else if (type == "app" || type == OR_APP) return TxType::APP;
        else if (type == "upvoteShare" || type == OR_SCORE) return TxType::ACTION_SCORE_CONTENT;
        else if (type == "subscribe" || type == OR_SUBSCRIBE) return TxType::ACTION_SUBSCRIBE;
        else if (type == "subscribePrivate" || type == OR_SUBSCRIBEPRIVATE) return TxType::ACTION_SUBSCRIBE_PRIVATE;
        else if (type == "unsubscribe" || type == OR_UNSUBSCRIBE) return TxType::ACTION_SUBSCRIBE_CANCEL;
        else if (type == "userInfo" || type == OR_USERINFO) return TxType::ACCOUNT_USER;
        else if (type == "accSet" || type == OR_ACCOUNT_SETTING) return TxType::ACCOUNT_SETTING;
        else if (type == "accDel" || type == OR_ACCOUNT_DELETE) return TxType::ACCOUNT_DELETE;
        else if (type == "comment" || type == OR_COMMENT) return TxType::CONTENT_COMMENT;
        else if (type == "commentEdit" || type == OR_COMMENT_EDIT) return TxType::CONTENT_COMMENT_EDIT;
        else if (type == "commentDelete" || type == OR_COMMENT_DELETE) return TxType::CONTENT_COMMENT_DELETE;
        else if (type == "cScore" || type == OR_COMMENT_SCORE) return TxType::ACTION_SCORE_COMMENT;
        else if (type == "contentBoost" || type == OR_CONTENT_BOOST) return TxType::BOOST_CONTENT;
        else if (type == "modFlag") return TxType::MODERATION_FLAG;
        else if (type == "modVote") return TxType::MODERATION_VOTE;
        else if (type == "brtaccount") return TxType::BARTERON_ACCOUNT;
        else if (type == "brtoffer") return TxType::BARTERON_OFFER;
        else return TxType::NOT_SUPPORTED;
    }
}