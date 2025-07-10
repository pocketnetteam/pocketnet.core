// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef SRC_RPC_MODERATION_REPOSITORY_H
#define SRC_RPC_MODERATION_REPOSITORY_H

#include "pocketdb/repositories/BaseRepository.h"

namespace PocketDb
{
    using namespace std;

    struct JuryContent
    {
        int64_t ContentId;
        TxType ContentType;
        int AddressLikers;
        UniValue JuryData;
    };

    class ModerationRepository : public BaseRepository
    {
    public:
        explicit ModerationRepository(SQLiteDatabase& db, bool timeouted) : BaseRepository(db, timeouted) {}

        void Init() override;
        void Destroy() override;

        JuryContent GetJury(const string& jury);
        vector<JuryContent> GetAllJury(const Pagination& pagination);
        vector<JuryContent> GetJuryAssigned(const string& address, bool verdict, const Pagination& pagination);
        UniValue GetJuryModerators(const string& jury);

        UniValue GetBans(const string& address);
        UniValue GetBadgeHistory(const string& address, BadgeType badge);

    };

    typedef std::shared_ptr<ModerationRepository> ModerationRepositoryRef;
}

#endif //SRC_RPC_MODERATION_REPOSITORY_H
