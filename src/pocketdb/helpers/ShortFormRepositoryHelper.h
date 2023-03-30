// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETDB_SHORTFORMREPOSITORYSHELPER_H
#define POCKETDB_SHORTFORMREPOSITORYSHELPER_H

#include "pocketdb/helpers/ShortFormModelsHelper.h"
#include "pocketdb/stmt.h"

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
        std::function<void (STMT, QueryParams const&)> binding;
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
        std::map<int64_t /* blocknum */, int64_t /* corresponding m_data's array index */> m_txArrIndices;
        std::vector<UniValue> m_data;
    };

    class ShortFormParser
    {
    public:
        void Reset(const int& startIndex);

        ShortForm ParseFull(Cursor& cursor);

        int64_t ParseBlockNum(Cursor& cursor);

        ShortTxType ParseType(Cursor& cursor);

        std::string ParseHash(Cursor& cursor);

        std::optional<std::vector<ShortTxOutput>> ParseOutputs(Cursor& cursor);

        std::optional<ShortAccount> ParseAccount(Cursor& cursor, const int& index);

    protected:
        std::optional<ShortTxData> ProcessTxData(Cursor& cursor, int& index);

    private:
        int m_startIndex = 0;
    };

    class EventsReconstructor
    {
    public:
        EventsReconstructor();

        void FeedRow(Cursor& cursor);

        std::vector<ShortForm> GetResult() const;
    private:
        ShortFormParser m_parser;
        std::vector<ShortForm> m_result;
    };

    class NotificationsReconstructor
    {
    public:
        NotificationsReconstructor();

        void FeedRow(Cursor& cursor);

        NotificationsResult GetResult() const;
    private:
        ShortFormParser m_parser;
        NotificationsResult m_notifications;
    };

    class NotificationSummaryReconstructor
    {
    public:
        void FeedRow(Cursor& cursor);

        auto GetResult() const
        {
            return m_result;
        }
    private:
        std::map<std::string, std::map<ShortTxType, int>> m_result;  
    };
}

#endif // POCKETDB_SHORTFORMREPOSITORYSHELPER_H