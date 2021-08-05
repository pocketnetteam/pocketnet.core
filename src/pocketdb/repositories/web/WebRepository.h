// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETDB_WEBREPOSITORY_H
#define POCKETDB_WEBREPOSITORY_H

#include "pocketdb/helpers/TransactionHelper.hpp"
#include "pocketdb/repositories/BaseRepository.hpp"

#include <timedata.h>

namespace PocketDb
{
using std::runtime_error;

using namespace PocketTx;
using namespace PocketHelpers;

class WebRepository : public BaseRepository
{
public:
    class param
    {
    private:
        int i;
        std::string s;
        std::vector<int> vi;
        std::vector<std::string> vs;
    public:
        param();
        param(int _i);
        param(std::string _s);
        param(std::vector<int> _vi);
        param(std::vector<std::string> _vs);

        int get_int();
        std::string get_str();
        std::vector<int> get_vint();
        std::vector<std::string> get_vstring();
    };
    explicit WebRepository(SQLiteDatabase& db) : BaseRepository(db) {}

    void Init() override;
    void Destroy() override;

    UniValue GetAddressInfo(int count);
    UniValue GetCommentsByPost(const std::string& postHash, const std::string& parentHash, const std::string& addressHash);
    UniValue GetCommentsByIds(string& addressHash, vector<string>& commentHashes);
    UniValue GetLastComments(int count, int height, std::string lang = "");
    UniValue GetPostScores(vector<string>& postHashes, string& address);
    UniValue GetPageScores(vector<string>& commentHashes, string& addressHash);
    map<string, UniValue> GetUserProfile(vector<string>& addresses, bool shortForm = true, int option = 0);
    map<string, UniValue> GetSubscribesAddresses(vector<string>& addresses);
    map<string, UniValue> GetSubscribersAddresses(vector<string>& addresses);
    map<string, UniValue> GetBlockingToAddresses(vector<string>& addresses);
    map<string, UniValue> GetContentsData(vector<string>& txids);
    map<string, UniValue> GetContents(std::map<std::string, param>& conditions);
private:
    UniValue ParseCommentRow(sqlite3_stmt* stmt);
};

typedef std::shared_ptr<WebRepository> WebRepositoryRef;

} // namespace PocketDb

#endif // POCKETDB_WEBREPOSITORY_H

