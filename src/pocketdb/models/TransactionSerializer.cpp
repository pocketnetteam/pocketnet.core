#include "TransactionSerializer.h"

namespace PocketTx
{
    static PocketTxType ParseType(const std::string &strType)
    {
        // TODO (brangr): implement enum for tx types
        if (strType == "Users") return PocketTxType::USER_ACCOUNT;
        if (strType == "Posts") return PocketTxType::POST_CONTENT;
        if (strType == "Scores") return PocketTxType::SCORE_POST_ACTION;
        if (strType == "Comment") return PocketTxType::COMMENT_CONTENT;
        if (strType == "Blocking") return PocketTxType::BLOCKING_ACTION;
        if (strType == "Subscribes") return PocketTxType::SUBSCRIBE_ACTION;
        if (strType == "Complains") return PocketTxType::COMPLAIN_ACTION;
        if (strType == "CommentScores") return PocketTxType::SCORE_COMMENT_ACTION;
    }

    static Transaction *BuildInstance(const UniValue &src)
    {
        auto txTypeSrc = src["t"].get_str();
        PocketTxType txType = ParseType(txTypeSrc);

        Transaction* tx;
        switch (txType)
        {
            case USER_ACCOUNT:
                tx = new User();
                break;
            case VIDEO_SERVER_ACCOUNT:
            case MESSAGE_SERVER_ACCOUNT:
            case POST_CONTENT:
                tx = new Post();
                break;
            case VIDEO_CONTENT:
            case TRANSLATE_CONTENT:
            case SERVERPING_CONTENT:
            case COMMENT_CONTENT:
            case SCORE_POST_ACTION:
            case SCORE_COMMENT_ACTION:
            case SUBSCRIBE_ACTION:
            case BLOCKING_ACTION:
            case COMPLAIN_ACTION:
            default:
                tx = nullptr;
        }

        if (tx == nullptr)
            return nullptr;

        UniValue txSrc(UniValue::VOBJ);
        auto txDataBase64 = src["d"].get_str();
        auto txJson = DecodeBase64(txDataBase64);
        txSrc.read(txJson);
        tx->Deserialize(txSrc);
        return tx;
    }

    std::vector<PocketTx::Transaction *> TransactionSerializer::DeserializeBlock(std::string &src)
    {
        std::vector<PocketTx::Transaction*> pocketTxn;

        UniValue pocketData(UniValue::VOBJ);
        pocketData.read(src);

        auto keys = pocketData.getKeys();
        for (const auto& key : keys) {
            auto entrySrc = pocketData[key];
            UniValue entry(UniValue::VOBJ);
            entry.read(entrySrc.get_str());

            auto tx = BuildInstance(entry);
            if (tx != nullptr)
                pocketTxn.push_back(tx);
        }

        return pocketTxn;
    };

}