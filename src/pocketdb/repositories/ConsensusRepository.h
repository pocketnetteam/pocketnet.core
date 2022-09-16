// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 Bitcoin developers
// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETDB_CONSENSUSREPOSITORY_H
#define POCKETDB_CONSENSUSREPOSITORY_H

#include "pocketdb/helpers/TransactionHelper.h"
#include "pocketdb/repositories/BaseRepository.h"
#include "pocketdb/repositories/TransactionRepository.h"

#include <boost/algorithm/string/join.hpp>
#include <boost/range/adaptor/transformed.hpp>
#include <timedata.h>

namespace PocketDb
{
    using boost::algorithm::join;
    using boost::adaptors::transformed;

    using namespace std;
    using namespace PocketTx;
    using namespace PocketHelpers;

    struct AccountData
    {
        string AddressHash;
        int64_t AddressId;
        int64_t Reputation;
        int64_t RegistrationTime;
        int64_t RegistrationHeight;
        int64_t Balance;

        int64_t LikersContent;
        int64_t LikersComment;
        int64_t LikersCommentAnswer;

        int64_t LikersAll() const
        {
            return LikersContent + LikersComment + LikersCommentAnswer;
        }
    };

    struct BadgeSharkConditions
    {
        int Height;
        int64_t LikersAll;
        int64_t LikersContent;
        int64_t LikersComment;
        int64_t LikersAnswer;
        int64_t RegistrationDepth;
    };

    struct BadgeSet
    {
        bool Shark = false;
        bool Whale = false;
        bool Moderator = false;
        bool Developer = false;

        UniValue ToJson()
        {
            UniValue ret(UniValue::VARR);
            
            if (Shark) ret.push_back("shark");
            if (Whale) ret.push_back("whale");
            if (Moderator) ret.push_back("moderator");
            if (Developer) ret.push_back("developer");

            return ret;
        }
    };

    class ConsensusRepository : public TransactionRepository
    {
    public:
        explicit ConsensusRepository(SQLiteDatabase& db) : TransactionRepository(db) {}

        void Init() override;
        void Destroy() override;

        tuple<bool, PTransactionRef> GetFirstContent(const string& rootHash);
        tuple<bool, PTransactionRef> GetLastContent(const string& rootHash, const vector<TxType>& types);
        tuple<bool, TxType> GetLastAccountType(const string& address);
        tuple<bool, int64_t> GetTransactionHeight(const string& hash);
        tuple<bool, TxType> GetLastBlockingType(const string& address, const string& addressTo);
        bool ExistBlocking(const string& address, const string& addressTo, const string& addressesTo);
        tuple<bool, TxType> GetLastSubscribeType(const string& address, const string& addressTo);

        shared_ptr<string> GetContentAddress(const string& postHash);
        int64_t GetUserBalance(const string& address);
        int GetUserReputation(const string& addressId);
        int GetUserReputation(int addressId);
        int64_t GetAccountRegistrationTime(int addressId);

        AccountData GetAccountData(const string& address);

        ScoreDataDtoRef GetScoreData(const string& txHash);
        shared_ptr<map<string, string>> GetReferrers(const vector<string>& addresses, int minHeight);
        tuple<bool, string> GetReferrer(const string& address);

        int GetScoreContentCount(
            int height,
            const shared_ptr<ScoreDataDto>& scoreData,
            const std::vector<int>& values,
            int64_t scoresOneToOneDepth);

        int GetScoreCommentCount(
            int height,
            const shared_ptr<ScoreDataDto>& scoreData,
            const std::vector<int>& values,
            int64_t scoresOneToOneDepth);

        // Exists
        bool ExistsComplain(const string& postHash, const string& address, bool mempool);
        bool ExistsScore(const string& address, const string& contentHash, TxType type, bool mempool);
        bool ExistsUserRegistrations(vector<string>& addresses);
        bool ExistsAnotherByName(const string& address, const string& name);
        bool Exists(const string& txHash, const vector<TxType>& types, bool inChain);
        bool ExistsInMempool(const string& string1, const vector<TxType>& types);
        bool ExistsInMempool(const string& string1, const string& string2, const vector<TxType>& types);
        bool ExistsNotDeleted(const string& txHash, const string& address, const vector<TxType>& types);

        // get counts in "mempool" - Height is null
        int CountMempoolBlocking(const string& address, const string& addressTo);
        int CountMempoolSubscribe(const string& address, const string& addressTo);

        int CountMempoolComment(const string& address);
        int CountChainCommentTime(const string& address, int64_t time);
        int CountChainCommentHeight(const string& address, int height);

        int CountMempoolComplain(const string& address);
        int CountChainComplainTime(const string& address, int64_t time);
        int CountChainComplainHeight(const string& address, int height);

        int CountMempoolPost(const string& address);
        int CountChainPostTime(const string& address, int64_t time);
        int CountChainPostHeight(const string& address, int height);

        int CountMempoolVideo(const string& address);
        int CountChainVideo(const string& address, int height);

        int CountMempoolArticle(const string& address);
        int CountChainArticle(const string& address, int height);

        int CountMempoolScoreComment(const string& address);
        int CountChainScoreCommentTime(const string& address, int64_t time);
        int CountChainScoreCommentHeight(const string& address, int height);

        int CountMempoolScoreContent(const string& address);
        int CountChainScoreContentTime(const string& address, int64_t time);
        int CountChainScoreContentHeight(const string& address, int height);

        int CountMempoolAccountSetting(const string& address);
        int CountChainAccountSetting(const string& address, int height);

        int CountChainAccount(TxType txType, const string& address, int height);

        int CountMempoolCommentEdit(const string& address, const string& rootTxHash);
        int CountChainCommentEdit(const string& address, const string& rootTxHash);

        int CountMempoolPostEdit(const string& address, const string& rootTxHash);
        int CountChainPostEdit(const string& address, const string& rootTxHash);

        int CountMempoolVideoEdit(const string& address, const string& rootTxHash);
        int CountChainVideoEdit(const string& address, const string& rootTxHash);

        int CountMempoolArticleEdit(const string& address, const string& rootTxHash);
        int CountChainArticleEdit(const string& address, const string& rootTxHash);

        int CountMempoolContentDelete(const string& address, const string& rootTxHash);

        /* MODERATION */
        int CountModerationFlag(const string& address, int height, bool includeMempool);
        int CountModerationFlag(const string& address, const string& addressTo, bool includeMempool);

    };

    typedef shared_ptr<ConsensusRepository> ConsensusRepositoryRef;

} // namespace PocketDb

#endif // POCKETDB_CONSENSUSREPOSITORY_H

