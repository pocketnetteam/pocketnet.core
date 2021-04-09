#include "BaseRepository.h"
#include "../tinyformat.h"

bool BaseRepository::TryBindStatementText(sqlite3_stmt *stmt, int index, const std::string * value) {
    if (!value) {
        return true;
    }

    int res = sqlite3_bind_text(stmt, index, value->c_str(), value->size(), SQLITE_STATIC);
    if (!CheckValidResult(stmt, res)) {
        return false;
    }

    return true;
}

bool BaseRepository::TryBindStatementInt(sqlite3_stmt *stmt, int index, int value) {
    int res = sqlite3_bind_int(stmt, index, value);
    if (!CheckValidResult(stmt, res)) {
        return false;
    }

    return true;
}

sqlite3_stmt* BaseRepository::SetupSqlStatement(sqlite3_stmt* stmt, const std::string& sql) const {
    int res;

    if (!stmt) {
        if ((res = sqlite3_prepare_v2(m_database.m_db, sql.c_str(), sql.size(), &stmt, nullptr)) != SQLITE_OK) {
            throw std::runtime_error(strprintf("SQLiteDatabase: Failed to setup SQL statements: %s\n", sqlite3_errstr(res)));
        }
    }

    return stmt;
}

bool BaseRepository::CheckValidResult(sqlite3_stmt *stmt, int result) {
    if (result != SQLITE_OK) {
        std::cout << strprintf("%s: Unable to bind statement: %s\n", __func__, sqlite3_errstr(result));
        sqlite3_clear_bindings(stmt);
        sqlite3_reset(stmt);
        return false;
    }

    return true;
}

BaseRepository::BaseRepository(SQLiteDatabase &database)
    : m_database(database) {
}
