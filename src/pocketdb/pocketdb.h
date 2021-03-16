// Copyright (c) 2018 PocketNet developers
// PocketDB general wrapper
//-----------------------------------------------------
#ifndef POCKETDB_H
#define POCKETDB_H
//-----------------------------------------------------
#include "core/namespace.h"
#include "core/namespacedef.h"
#include "core/reindexer.h"
#include "core/type_consts.h"
#include "tools/errors.h"
#include "util.h"
#include <crypto/sha256.h>
#include <uint256.h>
#include <univalue.h>
#include <utilstrencodings.h>
//-----------------------------------------------------
using namespace reindexer;
//-----------------------------------------------------
enum RatingType {
    UserReputation = 0,
    UserLikers = 1,
    PostRating = 2,
    CommentRating = 3,
};
//-----------------------------------------------------
class PocketDB
{
private:
    Reindexer* db;

    int cur_version = 2;

    void CloseNamespaces();
    bool UpdateDB();
    bool ConnectDB();

public:
    PocketDB();
    ~PocketDB();

    Reindexer* DB() { return db; };

    bool Init();
    bool InitDB(std::string table = "ALL");
    bool DropTable(std::string table);

    bool CheckIndexes(UniValue& obj);
    void UpdateIndexes(std::string table = "ALL");

    // Statistics for DB
    bool GetStatistic(std::string table, UniValue& obj);

    bool Exists(Query query);
    size_t SelectTotalCount(std::string table);
    size_t SelectCount(Query query);

    Error Select(Query query, QueryResults& res);
    Error SelectOne(Query query, Item& item);
    Error SelectAggr(Query query, QueryResults& aggRes);
    Error SelectAggr(Query query, std::string aggId, AggregationResult& aggRes);

    Error Upsert(std::string table, Item& item);
    Error UpsertWithCommit(std::string table, Item& item);

    // Delete items from query
    Error Delete(Query query);
    // Delete items from query and commit
    Error DeleteWithCommit(Query query);
    Error DeleteWithCommit(Query query, size_t& deleted);

    Error Update(std::string table, Item& item, bool commit = true);

    // Get last item and write to UsersView
    Error UpdateUsersView(std::string address, int height);
    // Get last item and write to SubscribesView
    Error UpdateSubscribesView(std::string address, std::string address_to);
    // Get last item and write to BlockingView
    Error UpdateBlockingView(std::string address, std::string address_to);

    // Return hash by values for compare with OP_RETURN
    bool GetHashItem(Item& item, std::string table, bool with_referrer, std::string& out_hash);

    // Ratings
    // User
    bool SetUserReputation(std::string address, int rep);
    bool UpdateUserReputation(std::string address, int height);
    int GetUserReputation(std::string _address, int height);
    int GetUserLikersCount(int userId, int height);
    bool ExistsUserLiker(int userId, int likerId, int height);

    // Post
    bool UpdatePostRating(std::string posttxid, int sum, int cnt, int& rep);
    bool UpdatePostRating(std::string posttxid, int height);
    void GetPostRating(std::string posttxid, int& sum, int& cnt, int& rep, int height);

    // Comment
    bool UpdateCommentRating(std::string commentid, int up, int down, int& rep);
    bool UpdateCommentRating(std::string commentid, int height);
    void GetCommentRating(std::string commentid, int& up, int& down, int& rep, int height);

    // Returns sum of all unspent transactions for address
    int64_t GetUserBalance(std::string _address, int height);
    int GetUserId(std::string address);

    // Search tags in DB
    void SearchTags(std::string search, int count, std::map<std::string, int>& tags, int& totalCount);

    // Add new Post with move old version to history table
    Error CommitPostItem(Item& itm, int height);

    // Restore previous version of Post
    Error RestorePostItem(std::string posttxid, int height);

    Error CommitLastItem(std::string table, Item& itm, int height);
    Error RestoreLastItem(std::string table, std::string txid, std::string otxid, int height);
};
//-----------------------------------------------------
extern std::unique_ptr<PocketDB> g_pocketdb;

/*
    Temp dictionary for PocketNET data, received by another nodes
    Key - block hash
    Value - JSON string (UniValue)
*/
extern std::map<uint256, std::string> POCKETNET_DATA;
//-----------------------------------------------------
#endif // POCKETDB_H