#include "CatRepository.h"
#include "../tinyformat.h"
#include "../sqlite/sqlite3.h"
#include <iostream>

CatRepository::CatRepository(SQLiteDatabase &database)
    : BaseRepository(database) {
    SetupSqlStatements();
}

void CatRepository::SetupSqlStatements() {
    m_insert_stmt = SetupSqlStatement(m_insert_stmt, "INSERT INTO Cats (Name, Age) VALUES(?, ?)");

    m_delete_stmt = SetupSqlStatement(m_delete_stmt, "DELETE FROM Cats WHERE Id = ?");

    m_select_all_stmt = SetupSqlStatement(m_select_all_stmt, "SELECT Id, Name, Age FROM Cats");
}

void CatRepository::Insert(const Cat &cat) {
    if (!m_database.m_db) {
        throw std::runtime_error(strprintf("SQLiteDatabase: database didn't opened: %s\n"));
    }

    if (!TryBindStatementText(m_insert_stmt, 1, cat.Text)) {
        return;
    }

    if (!TryBindStatementInt(m_insert_stmt, 2, cat.Age)) {
        return;
    }

    int res = sqlite3_step(m_insert_stmt);
    if (res != SQLITE_ROW) {
        if (res != SQLITE_DONE) {
            std::cout << strprintf("%s: Unable to execute statement: %s\n", __func__, sqlite3_errstr(res));
        }
    }

    sqlite3_clear_bindings(m_insert_stmt);
    sqlite3_reset(m_insert_stmt);
}

void CatRepository::Delete(int id) {
    if (!m_database.m_db) {
        throw std::runtime_error(strprintf("SQLiteDatabase: database didn't opened: %s\n"));
    }

    if (!TryBindStatementInt(m_delete_stmt, 1, id)) {
        return;
    }

    int res = sqlite3_step(m_delete_stmt);
    if (res != SQLITE_DONE) {
        std::cout << strprintf("%s: Unable to execute statement: %s\n", __func__, sqlite3_errstr(res));
    }

    sqlite3_clear_bindings(m_delete_stmt);
    sqlite3_reset(m_delete_stmt);
}

void CatRepository::BulkInsert(const std::vector<Cat>& cats) {
    if (!m_database.m_db) {
        throw std::runtime_error(strprintf("SQLiteDatabase: database didn't opened: %s\n"));
    }

    if (!m_database.BeginTransaction()) {
        throw std::runtime_error(strprintf("SQLiteDatabase: can't begin transaction: %s\n"));
    }

    try {
        for (const auto& cat : cats) {
            if (!TryBindStatementText(m_insert_stmt, 1, cat.Text)) {
                return;
            }

            if (!TryBindStatementInt(m_insert_stmt, 2, cat.Age)) {
                return;
            }

            int res = sqlite3_step(m_insert_stmt);
            if (res != SQLITE_ROW) {
                if (res != SQLITE_DONE) {
                    std::cout << strprintf("%s: Unable to execute statement: %s\n", __func__, sqlite3_errstr(res));
                }
            }

            sqlite3_reset(m_insert_stmt);
        }

        if (!m_database.CommitTransaction()) {
            throw std::runtime_error(strprintf("SQLiteDatabase: can't commit transaction: %s\n"));
        }
    }
    catch (std::exception & ex) {
        m_database.AbortTransaction();
    }

    sqlite3_clear_bindings(m_insert_stmt);
    sqlite3_reset(m_insert_stmt);
}

std::vector<Cat *> * CatRepository::SelectAllCats() {
    if (!m_database.m_db) {
        throw std::runtime_error(strprintf("SQLiteDatabase: database didn't opened: %s\n"));
    }

    auto res = sqlite3_step(m_select_all_stmt);
    if (res != SQLITE_ROW) {
        if (res != SQLITE_DONE) {
            std::cout << strprintf("%s: Unable to execute statement: %s\n", __func__, sqlite3_errstr(res));
            //TODO throw
            return new std::vector<Cat *>();
        }
    }

    auto * result = new std::vector<Cat*>();

    while(sqlite3_column_int(m_select_all_stmt, 0)) {
        auto id = sqlite3_column_int(m_select_all_stmt, 0);
        auto text = (const char *) sqlite3_column_text(m_select_all_stmt, 1);
        auto age = sqlite3_column_int(m_select_all_stmt, 2);

        result->push_back(new Cat(text, age, id));

        sqlite3_step(m_select_all_stmt);
    }

    sqlite3_clear_bindings(m_select_all_stmt);
    sqlite3_reset(m_select_all_stmt);

    return result;
}
