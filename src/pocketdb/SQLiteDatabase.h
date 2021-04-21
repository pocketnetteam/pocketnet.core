#ifndef POCKETDB_SQLITEDATABASE_H
#define POCKETDB_SQLITEDATABASE_H

#include "../logging.h"
#include "sqlite3.h"
#include <iostream>

namespace PocketDb {

class SQLiteDatabase
{
private:
    std::string m_dir_path;
    std::string m_file_path;

    void Cleanup() noexcept;

public:
    SQLiteDatabase() = default;
    void Init(const std::string& dir_path, const std::string& file_path);

    void Open();
    void Close();

    bool BeginTransaction();
    bool CommitTransaction();
    bool AbortTransaction();

    sqlite3* m_db{nullptr};
};

} // namespace PocketDb

#endif // POCKETDB_SQLITEDATABASE_H
