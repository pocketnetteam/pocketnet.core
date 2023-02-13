// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETDB_STMT_H
#define POCKETDB_STMT_H

#include "pocketdb/SQLiteDatabase.h"
#include "univalue.h"
#include <sqlite3.h>

#include <optional>

namespace PocketDb
{
    class Cursor;
    
    /**
    * Wrapper around sqlite3_stmt pointer.
    * Implemets RAII and calls sqlite3_finalize on object destruction.
    * If stmt object is not dynamically generated, it can be created as static and
    * reused by calling Stmt::Reset() method at the end of processing queue.
    */

    class StmtWrapper
    {
    public:
        StmtWrapper() = default;
        StmtWrapper(const StmtWrapper&) = delete;
        StmtWrapper(StmtWrapper&&) = default;
        ~StmtWrapper();

        int PrepareV2(SQLiteDatabase& db, const std::string& sql);
        int Step();

        int BindText(int index, const std::string& val);
        int BindInt(int index, int val);
        int BindInt64(int index, int64_t val);
        int BindNull(int index);

        int GetColumnType(int index);

        optional<string> GetColumnText(int index);
        optional<int> GetColumnInt(int index);
        optional<int64_t> GetColumnInt64(int index);

        int Reset();
        int Finalize();
        int ClearBindings();

        const char* ExpandedSql();

    private:
        sqlite3_stmt* m_stmt = nullptr;
    };

    class Stmt
    {
    public:
        Stmt(const Stmt&) = delete;
        Stmt(Stmt&&) = default;
        Stmt() = default;

        void Init(SQLiteDatabase& db, const std::string& sql);
        int Run();
        bool CheckValidResult(int result);
        int Select(const std::function<void(Cursor&)>& func);

        auto Log()
        {
            return m_stmt->ExpandedSql();
        }

        // --------------------------------
        // BINDS
        // --------------------------------
        // Thanks to Itaros (https://github.com/Itaros) for help in implementing this
        template <class ...Binds>
        Stmt& Bind(const Binds&... binds)
        {
            (Binder<Binds>::bind(*this, m_currentBindIndex, binds), ...);
            return *this;
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

    protected:
        std::shared_ptr<StmtWrapper> m_stmt = nullptr;
        void ResetCurrentBindIndex();
        // int Finalize(); // Removed because unused.

    private:
        int m_currentBindIndex = 1;

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
    };

    template <size_t N>
    class SeqRes
    {
    public:
        SeqRes(std::array<bool, N> resArray)
            : m_resArray(std::move(resArray)),
              m_overallRes(std::all_of(m_resArray.begin(), m_resArray.end(), [](const bool& elem) { return elem; })) {}

        operator bool() const { return m_overallRes; }

        bool operator[](const size_t& index)
        {
            if (index < m_resArray.size() && index >=0)
                return m_resArray[index];
            else
                return false;
        }
    private:
        const std::array<bool, N> m_resArray;
        const bool m_overallRes;
    };

    class Cursor
    {
    public:
        Cursor(std::shared_ptr<StmtWrapper> stmt)
            : m_stmt(std::move(stmt)) {}
        ~Cursor();

        Cursor(const Cursor&) = delete;
        Cursor() = delete;
        Cursor(Cursor&&) = default;

        bool Step();
        // Step verbose (with result code)
        int StepV();

        int Reset();

        // Collect data
        template <class ...Collects>
        auto CollectAll(Collects&... collects)
        {
            return SeqRes(std::array{Collector<Collects>::collect(*this, m_currentCollectIndex, collects) ...});
        }

        bool Collect(const int& index, string& val);
        bool Collect(const int& index, optional<string>& val);
        bool Collect(const int& index, int& val);
        bool Collect(const int& index, optional<int>& val);
        bool Collect(const int& index, int64_t& val);
        bool Collect(const int& index, optional<int64_t>& val);
        bool Collect(const int& index, bool& val);
        bool Collect(const int& index, optional<bool>& val);

        template <class Func>
        bool Collect(const int& index, const Func& func)
        {
            function f = func;
            _collectFunctor(index, f);
        }

        template <class T>
        bool Collect(const int& index, UniValue& uni, const std::string& key)
        {
            return Collect(
                index,
                [&](const T& val)
                {
                    uni.pushKV(key, val);
                }
            );
        }

        template <class T>
        tuple<bool, T> TryGetColumn(Cursor& cursor, int index)
        {
            T val;
            auto res = cursor.Collect(index, val);
            return {res, val};
        }
        tuple<bool, string> TryGetColumnString(int index);
        tuple<bool, int64_t> TryGetColumnInt64(int index);
        tuple<bool, int> TryGetColumnInt(int index);
    
    private:
        void ResetCurrentCollectIndex()
        {
            m_currentCollectIndex = 0;
        }
        std::shared_ptr<StmtWrapper> m_stmt;
        int m_currentCollectIndex = 0;

        template <class T>
        bool _collectFunctor(const int& index, const std::function<void(T)>& func)
        {
            using Type = remove_const_t<remove_reference_t<T>>;
            Type val;
            auto res = Collect(index, val);
            if (res) func(val);
            return res;
        }

        template<class T>
        class Collector
        {
        public:
            static bool collect(Cursor& stmt, int& i, T& t)
            {
                return stmt.Collect(i++, t);
            }
        };
    };
}

#endif // POCKETDB_STMT_H