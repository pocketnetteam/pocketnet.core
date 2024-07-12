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

        bool ModeratorBadge;

        int64_t LikersAll() const
        {
            return LikersContent + LikersComment + LikersCommentAnswer;
        }
    };

    struct BadgeSet
    {
        bool Shark = false; // 1
        bool Whale = false; // 2
        bool Moderator = false; // 3
        bool Developer = false; // 4

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

    // ----------------------------------------------------------------------
    // Consensus data
    // ----------------------------------------------------------------------

    struct ConsensusData_AccountUser {
        int LastTxType = -1;
        int EditsCount = 0;
        int MempoolCount = 0;
        int DuplicatesChainCount = 0;
        int DuplicatesMempoolCount = 0;
    };
    
    struct ConsensusData_BarteronAccount {
        int MempoolCount = 0;
    };

    struct ConsensusData_BarteronOffer {
        int MempoolCount = 0;
        int LastTxType = -1;
        int ActiveCount = 0;
    };

    // ----------------------------------------------------------------------

    class ConsensusRepository : public TransactionRepository
    {
    public:
        explicit ConsensusRepository(SQLiteDatabase& db, bool timeouted) : TransactionRepository(db, timeouted) {}

        ConsensusData_BarteronAccount BarteronAccount(const string& address);
        ConsensusData_BarteronOffer BarteronOffer(const string& address, const string& rootTxHash);

        tuple<bool, PTransactionRef> GetFirstContent(const string& rootHash);
        tuple<bool, PTransactionRef> GetLastContent(const string& rootHash, const vector<TxType>& types);
        tuple<bool, vector<PTransactionRef>>GetLastContents(const vector<string>& rootHashes, const vector<TxType>& types);
        int GetLastContentsCount(const vector<string> &rootHashes, const vector<TxType> &types);
        tuple<bool, TxType> GetLastAccountType(const string& address);
        tuple<bool, int64_t> GetTransactionHeight(const string& hash);
        tuple<bool, TxType> GetLastBlockingType(const string& address, const string& addressTo);
        bool ExistBlocking(const string& address, const string& addressTo);
        bool ExistBlocking(const string& address, const string& addressTo, const string& addressesTo);
        tuple<bool, TxType> GetLastSubscribeType(const string& address, const string& addressTo);

        optional<string> GetContentAddress(const string& postHash);
        int64_t GetUserBalance(const string& address);
        int GetUserReputation(const string& addressId);
        int GetUserReputation(int addressId);
        int64_t GetAccountRegistrationTime(const string& address);

        map<string, AccountData> GetAccountsData(const vector<string>& addresses);

        map<string, ScoreDataDtoRef> GetScoresData(int height, int64_t scores_time_depth);
        tuple<bool, string> GetReferrer(const string& address);

        // Exists
        bool ExistsComplain(const string& postHash, const string& address, bool mempool);
        bool ExistsScore(const string& address, const string& contentHash, TxType type, bool mempool);
        bool ExistsUserRegistrations(vector<string>& addresses);
        bool ExistsAccountBan(const string& address, int height);
        bool ExistsAnotherByName(const string& address, const string& name, TxType type);
        bool ExistsNotDeleted(const string& txHash, const string& address, const vector<TxType>& types);
        bool ExistsActiveJury(const string& juryId);

        bool Exists_S1S2T(const string& string1, const string& string2, const vector<TxType>& types);
        bool Exists_MS1T(const string& string1, const vector<TxType>& types);
        bool Exists_MS1S2T(const string& string1, const string& string2, const vector<TxType>& types);
        bool Exists_LS1T(const string& string1, const vector<TxType>& types);
        bool Exists_LS1S2T(const string& string1, const string& string2, const vector<TxType>& types);
        bool Exists_HS1T(const string& txHash, const string& string1, const vector<TxType>& types, bool last);
        bool Exists_HS2T(const string& txHash, const string& string2, const vector<TxType>& types, bool last);
        bool Exists_HS1S2T(const string& txHash, const string& string1, const string& string2, const vector<TxType>& types, bool last);

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

        int CountMempoolStream(const string& address);
        int CountChainStream(const string& address, int height);

        int CountMempoolAudio(const string& address);
        int CountChainAudio(const string& address, int height);

        int CountMempoolCollection(const string& address);
        int CountChainCollection(const string& address, int height);
        
        int CountMempoolBarteronOffer(const std::string& address);
        int CountChainBarteronOffer(const std::string& address, int height);

        int CountMempoolBarteronRequest(const std::string& address);
        int CountChainBarteronRequest(const std::string& address, int height);

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

        int CountMempoolStreamEdit(const string& address, const string& rootTxHash);
        int CountChainStreamEdit(const string& address, const string& rootTxHash);

        int CountMempoolAudioEdit(const string& address, const string& rootTxHash);
        int CountChainAudioEdit(const string& address, const string& rootTxHash);

        int CountMempoolCollectionEdit(const string& address, const string& rootTxHash);
        int CountChainCollectionEdit(const string& address, const string& rootTxHash, const int& nHeight, const int& depth);
        
        int CountMempoolBarteronOfferEdit(const string& address, const string& rootTxHash);
        int CountChainBarteronOfferEdit(const string& address, const string& rootTxHash);

        int CountMempoolContentDelete(const string& address, const string& rootTxHash);

        int CountChainHeight(TxType txType, const string& address);

        /* MODERATION */
        int CountModerationFlag(const string& address, int height, bool includeMempool);
        int CountModerationFlag(const string& address, const string& addressTo, bool includeMempool);
        bool AllowJuryModerate(const string& address, const string& flagTxHash);

    protected:
    
    };

    typedef shared_ptr<ConsensusRepository> ConsensusRepositoryRef;

} // namespace PocketDb

#endif // POCKETDB_CONSENSUSREPOSITORY_H

