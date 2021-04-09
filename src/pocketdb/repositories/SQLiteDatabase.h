#ifndef TESTDEMO_SQLITEDATABASE_H
#define TESTDEMO_SQLITEDATABASE_H

#include "../sqlite/sqlite3.h"

#include <iostream>
#include <filesystem>

#define fs std::filesystem //TODO move out

class SQLiteDatabase {
private:
    const std::string m_dir_path;

    const std::string m_file_path;

    void Cleanup() noexcept;
public:
    SQLiteDatabase() = delete;

    SQLiteDatabase(const fs::path& dir_path, const fs::path& file_path);

    void Open();
    void Close();

    bool BeginTransaction();
    bool CommitTransaction();
    bool AbortTransaction();

    sqlite3* m_db{nullptr};
};


#endif //TESTDEMO_SQLITEDATABASE_H
