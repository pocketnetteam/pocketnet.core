#ifndef TESTDEMO_BASEREPOSITORY_H
#define TESTDEMO_BASEREPOSITORY_H

#include "SQLiteDatabase.h"

class BaseRepository {
protected:
    SQLiteDatabase& m_database;

    static bool TryBindStatementText(sqlite3_stmt* stmt, int index, const std::string * value);
    static bool TryBindStatementInt(sqlite3_stmt *stmt, int index, int value);

    static bool CheckValidResult(sqlite3_stmt * stmt, int result);

    sqlite3_stmt* SetupSqlStatement(sqlite3_stmt* stmt, const std::string& sql) const;
public:
    explicit BaseRepository(SQLiteDatabase &database);
};


#endif //TESTDEMO_BASEREPOSITORY_H
