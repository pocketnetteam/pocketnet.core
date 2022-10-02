// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETDB_ROW_ACCESSOR_H
#define POCKETDB_ROW_ACCESSOR_H

#include <stdio.h>
#include <string>
#include <sqlite3.h>
#include <tuple>

namespace PocketDb
{
    using namespace std;

    class RowAccessor
    {
    public:
        tuple<bool, string> TryGetColumnString(sqlite3_stmt* stmt, int index)
        {
            return sqlite3_column_type(stmt, index) == SQLITE_NULL
                   ? make_tuple(false, "")
                   : make_tuple(true, string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, index))));
        }

        tuple<bool, int64_t> TryGetColumnInt64(sqlite3_stmt* stmt, int index)
        {
            return sqlite3_column_type(stmt, index) == SQLITE_NULL
                   ? make_tuple(false, (int64_t) 0)
                   : make_tuple(true, (int64_t) sqlite3_column_int64(stmt, index));
        }

        tuple<bool, int> TryGetColumnInt(sqlite3_stmt* stmt, int index)
        {
            return sqlite3_column_type(stmt, index) == SQLITE_NULL
                   ? make_tuple(false, 0)
                   : make_tuple(true, sqlite3_column_int(stmt, index));
        }
    };

}


#endif //POCKETDB_ROW_ACCESSOR_H