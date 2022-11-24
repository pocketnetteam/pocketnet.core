// Wrapper for basic actions with LevelDB
//-----------------------------------------------------
#include <ldb/ldb.h>
//-----------------------------------------------------
LDB::LDB(std::string _dbPath, size_t _n_cache_size, bool _logging)
{
    dbPath = _dbPath;
    logging = _logging;
    //----------------------------
    // Prepare DB
    //----------------------------
    // Parameter and path
    dbOptions.create_if_missing = true;
    //dbOptions.block_cache = leveldb::NewLRUCache(_n_cache_size / 2);
    //dbOptions.write_buffer_size = _n_cache_size / 4;
    dbOptions.compression = leveldb::CompressionType::kNoCompression;
    //----------------------------
    leveldb::Status status;

    // Create dir for DB
    fs::create_directories(dbPath);

    // Open DB TXOuts
    status = leveldb::DB::Open(dbOptions, dbPath, &ldb);
    if (!status.ok()) {
        LogPrintf("Fatal LevelDB (%s) error: %s\n", dbPath, status.ToString());
    }
    //----------------------------
    // Debug checking
    LogPrintf("Start checking `%s` db...\n", dbPath);
    int count = 0;
    leveldb::Iterator* it = ldb->NewIterator(leveldb::ReadOptions());
    for (it->SeekToFirst(); it->Valid(); it->Next()) {
        std::string value;
        status = ldb->Get(leveldb::ReadOptions(), it->key(), &value);
        count += 1;
    }
    LogPrintf("Complete checking `%s`: complete %i transaction(s).\n", dbPath, count);
    if (!it->status().ok()) LogPrintf("Error in transaction: %s\n", it->status().ToString());
    delete it;
    //----------------------------
};
//-----------------------------------------------------
LDB::~LDB()
{
    delete ldb;
    ldb = nullptr;
}
//-----------------------------------------------------
leveldb::Status LDB::Get(std::string _key, std::string* _value)
{
    leveldb::Slice sKey(_key);
    leveldb::Status status = ldb->Get(leveldb::ReadOptions(), sKey, _value);

    if (logging && !status.ok() && !status.IsNotFound()) {
        LogPrintf("LevelDB fatal error: %s\n", status.ToString());
    }

    return status;
}
leveldb::Status LDB::Put(std::string _key, std::string _value)
{
    leveldb::Slice sKey(_key);
    leveldb::Slice sValue(_value);
    leveldb::Status status = ldb->Put(leveldb::WriteOptions(), sKey, sValue);

    if (logging && !status.ok() && !status.IsNotFound()) {
        LogPrintf("LevelDB fatal error: %s\n", status.ToString());
    }

    return status;
}
leveldb::Status LDB::Put(std::string _key, const char* _value, unsigned int _size) {
	leveldb::Slice sKey(_key);
	leveldb::Slice sValue(_value, _size);
	leveldb::Status status = ldb->Put(leveldb::WriteOptions(), sKey, sValue);

	if (logging && !status.ok() && !status.IsNotFound()) {
		LogPrintf("LevelDB fatal error: %s\n", status.ToString());
	}

	return status;
}
leveldb::Status LDB::Delete(std::string _key)
{
    leveldb::Slice sKey(_key);
    leveldb::Status status = ldb->Delete(leveldb::WriteOptions(), sKey);

    if (logging && !status.ok() && !status.IsNotFound()) {
        LogPrintf("LevelDB fatal error: %s\n", status.ToString());
    }

    return status;
}
leveldb::Status LDB::Update(std::string _key, std::string _value)
{
    leveldb::Slice sKey(_key);
    leveldb::Slice sValue(_value);

    ldb->Delete(leveldb::WriteOptions(), sKey);
    leveldb::Status status = ldb->Put(leveldb::WriteOptions(), sKey, sValue);

    if (logging && !status.ok() && !status.IsNotFound()) {
        LogPrintf("LevelDB fatal error: %s\n", status.ToString());
    }

    return status;
}
bool LDB::Exists(std::string _key)
{
    std::string value;
    leveldb::Slice sKey(_key);
    leveldb::Status status = ldb->Get(leveldb::ReadOptions(), sKey, &value);

    if (logging && !status.ok() && !status.IsNotFound()) {
        LogPrintf("LevelDB fatal error: %s\n", status.ToString());
    }

    return status.ok() && !status.IsNotFound();
}
//-----------------------------------------------------