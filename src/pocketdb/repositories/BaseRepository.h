#ifndef TESTDEMO_BASEREPOSITORY_H
#define TESTDEMO_BASEREPOSITORY_H

#include "pocketdb/SQLiteDatabase.h"

using namespace PocketDb;

class BaseRepository
{
protected:
    SQLiteDatabase& m_database;

    static bool TryBindStatementText(sqlite3_stmt* stmt, int index, const std::string* value);
    static bool TryBindStatementInt(sqlite3_stmt* stmt, int index, const int* value);
    static bool TryBindStatementInt64(sqlite3_stmt* stmt, int index, const int64_t* value);

    static bool CheckValidResult(sqlite3_stmt* stmt, int result);

    sqlite3_stmt* SetupSqlStatement(sqlite3_stmt* stmt, const std::string& sql) const;

public:
    BaseRepository(SQLiteDatabase& db);
    virtual void Init() = 0;
};


#endif //TESTDEMO_BASEREPOSITORY_H
