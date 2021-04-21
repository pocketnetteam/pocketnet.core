#include "TransactionSerializer.h"

namespace PocketTx
{
    static PocketTxType ParseType(const std::string &strType)
    {
        // TODO (brangr): implement enum for tx types
        if (strType == "Users") return PocketTxType::USERACCOUNT;
        if (strType == "Posts") return PocketTxType::POSTCONTENT;
        if (strType == "Scores") return PocketTxType::SCOREPOSTACTION;
        if (strType == "Comment") return PocketTxType::COMMENTCONTENT;
        if (strType == "Blocking") return PocketTxType::BLOCKACTION;
        if (strType == "Subscribes") return PocketTxType::SUBSCRIBEACTION;
        if (strType == "Complains") return PocketTxType::COMPLAINACTION;
        if (strType == "CommentScores") return PocketTxType::SCORECOMMENTACTION;
    }

    static Transaction *BuildInstance(const UniValue &src)
    {
        auto txTypeSrc = src["t"].get_str();
        PocketTxType txType = ParseType(txTypeSrc);

        Transaction* tx;
        switch (txType)
        {
            case USERACCOUNT:
                tx = new User();
                break;
            case VIDEOSERVERACCOUNT:
            case MESSAGESERVERACCOUNT:
            case POSTCONTENT:
                tx = new Post();
                break;
            case VIDEOCONTENT:
            case TRANSLATECONTENT:
            case SERVERPINGCONTENT:
            case COMMENTCONTENT:
            case SCOREPOSTACTION:
            case SCORECOMMENTACTION:
            case SUBSCRIBEACTION:
            case BLOCKACTION:
            case COMPLAINACTION:
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