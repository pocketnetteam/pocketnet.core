// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 Bitcoin developers
// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef SRC_WEBUSERREPOSITORY_H
#define SRC_WEBUSERREPOSITORY_H

#include "pocketdb/helpers/TransactionHelper.hpp"
#include "pocketdb/repositories/BaseRepository.hpp"

#include <boost/algorithm/string/join.hpp>
#include <boost/range/adaptor/transformed.hpp>

namespace PocketDb
{
    using std::runtime_error;

    using boost::algorithm::join;
    using boost::adaptors::transformed;

    using namespace PocketTx;
    using namespace PocketHelpers;

    class WebUserRepository : public BaseRepository
    {
    public:
        explicit WebUserRepository(SQLiteDatabase& db) : BaseRepository(db) {}

        void Init() override;
        void Destroy() override;

        UniValue GetUserAddress(string& name);
        UniValue GetAddressesRegistrationDates(vector<string>& addresses);
    };

    typedef std::shared_ptr<WebUserRepository> WebUserRepositoryRef;

} // namespace PocketDb

#endif //SRC_WEBUSERREPOSITORY_H
