// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETDB_SHORTFORMREPOSITORYSHELPER_H
#define POCKETDB_SHORTFORMREPOSITORYSHELPER_H

#include "pocketdb/repositories/RowAccessor.hpp"
#include "pocketdb/helpers/ShortFormModelsHelper.h"

#include <string>
#include <map>
#include <optional>
#include <functional>

namespace PocketHelpers
{
    using namespace PocketDb;
        // STMT here is used to avoid including here any of sqlite3 headers, however
    // it is expected that STMT is a correct sqlite3_stmt ptr.
    // QueryPrarams can differ between queries.
    template <class STMT, class QueryParams>
    struct ShortFormSqlEntry
    {
        std::string query; // The query that will be a part of big "union" request.
        /**
         * A functor that should bind the query above with given QueryParams
         * @param 1 - sqlite3_stmt ptr
         * @param 2 - current column index
         * @param 3 - query parameters to be binded to stmt
         */
        std::function<void (STMT, int&, QueryParams const&)> binding;
    };

    struct NotifierEntry
    {
        std::optional<ShortAccount> account;
        std::map<ShortTxType, std::vector<UniValue>> notifications;
    };

    class NotificationsResult
    {
    public:
        bool HasData(const int64_t& blocknum);

        void InsertData(const ShortForm& shortForm);

        void InsertNotifiers(const int64_t& blocknum, ShortTxType contextType, std::map<std::string, std::optional<ShortAccount>> addresses);

        UniValue Serialize() const;

    private:
        std::map<std::string /* address */, NotifierEntry> m_notifiers;
        std::map<int64_t /* blocknum */, int64_t /* corresponding m_data's array index */> m_txArrIndicies;
        std::vector<UniValue> m_data;
    };

    class ShortFormParser : public RowAccessor
    {
    public:
        void Reset(const int& startIndex);

        ShortForm ParseFull(sqlite3_stmt* stmt);

        int64_t ParseBlockNum(sqlite3_stmt* stmt);

        ShortTxType ParseType(sqlite3_stmt* stmt);

        std::string ParseHash(sqlite3_stmt* stmt);

        std::optional<std::vector<ShortTxOutput>> ParseOutputs(sqlite3_stmt* stmt);

        std::optional<ShortAccount> ParseAccount(sqlite3_stmt* stmt, const int& index);

    protected:
        std::optional<ShortTxData> ProcessTxData(sqlite3_stmt* stmt, int& index);

    private:
        int m_startIndex = 0;
    };

    class EventsReconstructor : public RowAccessor
    {
    public:
        EventsReconstructor();

        void FeedRow(sqlite3_stmt* stmt);

        std::vector<ShortForm> GetResult() const;
    private:
        ShortFormParser m_parser;
        std::vector<ShortForm> m_result;
    };

    class NotificationsReconstructor : public RowAccessor
    {
    public:
        NotificationsReconstructor();

        void FeedRow(sqlite3_stmt* stmt);

        NotificationsResult GetResult() const;
    private:
        ShortFormParser m_parser;
        NotificationsResult m_notifications;
    };

    class NotificationSummaryReconstructor : public RowAccessor
    {
    public:
        void FeedRow(sqlite3_stmt* stmt);

        auto GetResult() const
        {
            return m_result;
        }
    private:
        std::map<std::string, std::map<ShortTxType, int>> m_result;  
    };
}

#endif // POCKETDB_SHORTFORMREPOSITORYSHELPER_H