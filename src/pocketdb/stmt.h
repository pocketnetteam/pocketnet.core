// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETDB_STMT_H
#define POCKETDB_STMT_H

#include "pocketdb/SQLiteDatabase.h"
#include <sqlite3.h>

#include <optional>

namespace PocketDb
{
    /**
    * Wrapper around sqlite3_stmt pointer.
    * Implemets RAII and calls sqlite3_finalize on object destruction.
    * If stmt object is not dynamically generated, it can be created as static and
    * reused by calling Stmt::Reset() method at the end of processing queue.
    */
    class Stmt
    {
    public:
        Stmt(const Stmt&) = delete;
        Stmt(Stmt&&) = default;
        Stmt() = default;
        ~Stmt();
        void Init(SQLiteDatabase& db, const std::string& sql);
        int Step();
        int Finalize();
        int Reset();

        // --------------------------------
        // BINDS
        // --------------------------------
        // Thanks to Itaros (https://github.com/Itaros) for help in implementing this
        template <class ...Binds>
        void Bind(const Binds&... binds)
        {
            (Binder<Binds>::bind(*this, m_currentBindIndex, binds), ...);
        }
        // Forces user to handle memory more correct because of SQLITE_STATIC requires it
        void TryBindStatementText(int index, const std::string&& value) = delete;
        void TryBindStatementText(int index, const std::string& value);
        // Forces user to handle memory more correct because of SQLITE_STATIC requires it
        bool TryBindStatementText(int index, const std::optional<std::string>&& value) = delete;
        bool TryBindStatementText(int index, const std::optional<std::string>& value);
        bool TryBindStatementInt(int index, const std::optional<int>& value);
        void TryBindStatementInt(int index, int value);
        bool TryBindStatementInt64(int index, const std::optional<int64_t>& value);
        void TryBindStatementInt64(int index, int64_t value);
        bool TryBindStatementNull(int index);

        template <class ...Collects>
        void Collect(Collects&... collects)
        {
            (Collector<Collects>::collect(*this, m_currentCollectIndex, collects), ...);
        }
        tuple<bool, std::string> TryGetColumnString(int index);
        tuple<bool, int64_t> TryGetColumnInt64(int index);
        tuple<bool, int> TryGetColumnInt(int index);

        bool CheckValidResult(int result);

        auto Log()
        {
            return sqlite3_expanded_sql(m_stmt);
        }

    protected:
        void ResetInternalIndicies();

    private:
        sqlite3_stmt* m_stmt = nullptr;
        int m_currentBindIndex = 1;
        int m_currentCollectIndex = 0;

    private:
        template<class T>
        class Binder
        {
        public:
            static void bind(Stmt& stmt, int& i, T const& t)
            {
                // Using is_same_v here to bind int to statement where the value is literally int. If it is long, long long, short, etc we want
                // to convert it to int64_t below to prevent any overflows
                if constexpr (std::is_same_v<T, int> || std::is_same_v<T, std::optional<int>>) {
                    stmt.TryBindStatementInt(i++, t);
                }
                else if constexpr (std::is_convertible_v<T, int64_t> || std::is_convertible_v<T, optional<int64_t>>) {
                    stmt.TryBindStatementInt64(i++, t);
                }
                else if constexpr (std::is_convertible_v<T, string> || std::is_convertible_v<T, optional<string>>) {
                    stmt.TryBindStatementText(i++, t);
                } else {
                    // TODO (losty): use something like std::is_vetor_v
                    for (const auto& elem: t) {
                        Binder<decltype(elem)>::bind(stmt, i, elem); // Recursion
                    }
                }
            }
        };

        template<class T>
        class Collector
        {
        public:
            static void collect(Stmt& stmt, int& i, T& t)
            {
                if constexpr (std::is_same_v<T, int>) {
                    if (auto [ok, val] = stmt.TryGetColumnInt(i++); ok) t = val;
                } else if constexpr (std::is_same_v<T, int64_t>) {
                    if (auto [ok, val] = stmt.TryGetColumnInt64(i++); ok) t = val;
                } else if constexpr (std::is_same_v<T, std::string>) {
                    if (auto [ok, val] = stmt.TryGetColumnString(i++); ok) t = val;
                }
            }
        };
    };
}

#endif // POCKETDB_STMT_H