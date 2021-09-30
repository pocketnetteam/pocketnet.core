//
// Created by Joknek on 9/30/2021.
//

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

        UniValue GetPostLang(const string& postHash);
        UniValue GetOriginalPostAddressByRepost(const string& repostHash);
        UniValue GetPrivateSubscribeAddressByAddressTo(const string& addressTo);
        UniValue GetUserReferrerAddress(const string& userHash);
        UniValue GetPostInfoAddressByScore(const string& postScoreHash);
        UniValue GetSubscribeAddressTo(const string& subscribeHash);
        UniValue GetCommentInfoAddressByScore(const string& commentScoreHash);
        UniValue GetFullCommentInfo(const string& commentHash);
    };
}

#endif //POCKETNET_CORE_NOTIFIERREPOSITORY_H
