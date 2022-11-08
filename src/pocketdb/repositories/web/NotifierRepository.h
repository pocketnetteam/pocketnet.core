// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETNET_CORE_NOTIFIERREPOSITORY_H
#define POCKETNET_CORE_NOTIFIERREPOSITORY_H

#include "pocketdb/repositories/BaseRepository.h"

namespace PocketDb
{
    class NotifierRepository : public BaseRepository
    {
    public:
        explicit NotifierRepository(SQLiteDatabase& db) : BaseRepository(db) {}

        void Init() override;
        void Destroy() override;

        UniValue GetAccountInfoByAddress(const string& address);
        UniValue GetPostLang(const string& postHash);
        UniValue GetPostInfo(const string& postHash);
        UniValue GetBoostInfo(const string& boostHash);
        UniValue GetOriginalPostAddressByRepost(const string& repostHash);
        UniValue GetPrivateSubscribeAddressesByAddressTo(const string& addressTo);
//        UniValue GetUserReferrerAddress(const string& userHash);  // Not used. Not planned yet. Invalid request.
        UniValue GetPostInfoAddressByScore(const string& postScoreHash);
        UniValue GetSubscribeAddressTo(const string& subscribeHash);
        UniValue GetCommentInfoAddressByScore(const string& commentScoreHash);
        UniValue GetFullCommentInfo(const string& commentHash);
        UniValue GetPostCountFromMySubscribes(const string& address, int height);
    };

    typedef shared_ptr<NotifierRepository> NotifierRepositoryRef;
}

#endif //POCKETNET_CORE_NOTIFIERREPOSITORY_H
