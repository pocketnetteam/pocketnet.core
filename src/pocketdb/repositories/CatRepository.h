#ifndef TESTDEMO_CATREPOSITORY_H
#define TESTDEMO_CATREPOSITORY_H

#include "../models/Cat.h"
#include "../sqlite/sqlite3.h"
#include "SQLiteDatabase.h"
#include "BaseRepository.h"

class CatRepository : public BaseRepository {
private:
    sqlite3_stmt* m_insert_stmt{nullptr};
    sqlite3_stmt* m_delete_stmt{nullptr};
    sqlite3_stmt* m_select_all_stmt{nullptr};

    void SetupSqlStatements();
public:
    explicit CatRepository(SQLiteDatabase &database);

    void Insert(const Cat& cat);

    void BulkInsert(const std::vector<Cat>& cats);

    void Delete(int id);

    std::vector<Cat *> * SelectAllCats();
};


#endif //TESTDEMO_CATREPOSITORY_H
