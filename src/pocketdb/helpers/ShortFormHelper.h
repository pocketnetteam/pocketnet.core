// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETDB_SHORTFORMHELPER_H
#define POCKETDB_SHORTFORMHELPER_H

#include "pocketdb/models/shortform/ShortForm.h"
#include "pocketdb/repositories/BaseRepository.h"

#include "univalue.h"
#include "sqlite3.h"

#include <string>
#include <map>
#include <optional>
#include <functional>

namespace PocketHelpers
{
    class ShortTxTypeConvertor
    {
    public:
        static std::string toString(PocketDb::ShortTxType type);
        static PocketDb::ShortTxType strToType(const std::string& typeStr);
    };

    class ShortTxFilterValidator
    {
    public:
        class Notifications
        {
        public:
            static bool IsFilterAllowed(PocketDb::ShortTxType type);
        };

        class NotificationsSummary
        {
        public:
            static bool IsFilterAllowed(PocketDb::ShortTxType type);
        };

        class Activities
        {
        public:
            static bool IsFilterAllowed(PocketDb::ShortTxType type);
        };
        
        class Events
        {
        public:
            static bool IsFilterAllowed(PocketDb::ShortTxType type);
        };
    };

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
        std::optional<PocketDb::ShortAccount> account;
        std::map<PocketDb::ShortTxType, std::vector<UniValue>> notifications;
    };

    class NotificationsResult
    {
    public:
        bool HasData(const int64_t& blocknum);

        void InsertData(const PocketDb::ShortForm& shortForm);

        void InsertNotifiers(const int64_t& blocknum, PocketDb::ShortTxType contextType, std::map<std::string, std::optional<PocketDb::ShortAccount>> addresses);

        UniValue Serialize() const;

    private:
        std::map<std::string /* address */, NotifierEntry> m_notifiers;
        std::map<int64_t /* blocknum */, int64_t /* corresponding m_data's array index */> m_txArrIndicies;
        std::vector<UniValue> m_data;
    };

    class ShortFormParser : public PocketDb::RowAccessor
    {
    public:
        void Reset(const int& startIndex);

        PocketDb::ShortForm ParseFull(sqlite3_stmt* stmt);

        int64_t ParseBlockNum(sqlite3_stmt* stmt);

        PocketDb::ShortTxType ParseType(sqlite3_stmt* stmt);

        std::string ParseHash(sqlite3_stmt* stmt);

        std::optional<std::vector<PocketDb::ShortTxOutput>> ParseOutputs(sqlite3_stmt* stmt);

        std::optional<PocketDb::ShortAccount> ParseAccount(sqlite3_stmt* stmt, const int& index);

    protected:
        std::optional<PocketDb::ShortTxData> ProcessTxData(sqlite3_stmt* stmt, int& index);

    private:
        int m_startIndex = 0;
    };

    class EventsReconstructor : public PocketDb::RowAccessor
    {
    public:
        EventsReconstructor();

        void FeedRow(sqlite3_stmt* stmt);

        std::vector<PocketDb::ShortForm> GetResult() const;
    private:
        ShortFormParser m_parser;
        std::vector<PocketDb::ShortForm> m_result;
    };

    class NotificationsReconstructor : public PocketDb::RowAccessor
    {
    public:
        NotificationsReconstructor();

        void FeedRow(sqlite3_stmt* stmt);

        NotificationsResult GetResult() const;
    private:
        ShortFormParser m_parser;
        NotificationsResult m_notifications;
    };

    class NotificationSummaryReconstructor : public PocketDb::RowAccessor
    {
    public:
        void FeedRow(sqlite3_stmt* stmt);

        auto GetResult() const
        {
            return m_result;
        }
    private:
        std::map<std::string, std::map<PocketDb::ShortTxType, int>> m_result;  
    };
}

#endif // POCKETDB_SHORTFORMHELPER_H