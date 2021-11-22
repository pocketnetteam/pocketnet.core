// brangr
// Wrapper for basic actions with LevelDB
//-----------------------------------------------------
#ifndef LDB_H
#define LDB_H
//-----------------------------------------------------
#include <leveldb/cache.h>
#include <leveldb/db.h>
#include <leveldb/write_batch.h>
#include <util/system.h>

using std::string;
//-----------------------------------------------------
class LDB
{
private:
    leveldb::Options dbOptions;
    std::string dbPath;
    bool logging = false;


public:
	leveldb::DB* ldb;
    explicit LDB(std::string _dbPath, size_t _n_cache_size, bool _logging);
    ~LDB();

    leveldb::Status Get(std::string _key, std::string* _value);
    leveldb::Status Put(std::string _key, std::string _value);
	leveldb::Status Put(std::string _key, const char* _value, unsigned int _size);
    leveldb::Status Delete(std::string _key);
    leveldb::Status Update(std::string _key, std::string _value);
    bool Exists(std::string _key);
};
//-----------------------------------------------------
#endif // LDB_H