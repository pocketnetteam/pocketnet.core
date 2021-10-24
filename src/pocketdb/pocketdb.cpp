// Copyright (c) 2018 PocketNet developers
// PocketDB general wrapper
//-----------------------------------------------------
#include "pocketdb/pocketdb.h"
#include "html.h"
#include "tools/logger.h"

#if defined(HAVE_CONFIG_H)
#include <config/pocketcoin-config.h>
#endif //HAVE_CONFIG_H
//-----------------------------------------------------
std::unique_ptr<PocketDB> g_pocketdb;
Mutex POCKETNET_DATA_MUTEX;
std::map<uint256, std::string> POCKETNET_DATA;
//-----------------------------------------------------
PocketDB::PocketDB()
{
    // reindexer::logInstallWriter([](int level, char* buf) {
    //     LogPrintf("=== %s\n", buf);
    // });
}
PocketDB::~PocketDB()
{
    CloseNamespaces();
}

void PocketDB::CloseNamespaces()
{
    db->CloseNamespace("Service");
    db->CloseNamespace("Mempool");
    db->CloseNamespace("UsersView");
    db->CloseNamespace("Users");
    db->CloseNamespace("Tags");
    db->CloseNamespace("Posts");
    db->CloseNamespace("PostsHistory");
    db->CloseNamespace("Scores");
    db->CloseNamespace("Complains");
    db->CloseNamespace("UserRatings");
    db->CloseNamespace("PostRatings");
    db->CloseNamespace("SubscribesView");
    db->CloseNamespace("Subscribes");
    db->CloseNamespace("BlockingView");
    db->CloseNamespace("Blocking");
    db->CloseNamespace("Reposts");
    db->CloseNamespace("UTXO");
    db->CloseNamespace("Addresses");
    db->CloseNamespace("Comments");
    db->CloseNamespace("Comment");
}

// Check for update DB
bool PocketDB::UpdateDB()
{
    // Find current version of RDB
    int db_version = 0;
    Item service_itm;
    Error err = SelectOne(Query("Service").Sort("version", true), service_itm);
    if (err.ok()) db_version = service_itm["version"].As<int>();

    // Need to update?
    if (db_version < cur_version) {
        LogPrintf("Update RDB structure from v%s to v%s. Blockchain data will be erased and uploaded again.\n", db_version, cur_version);

        CloseNamespaces();
        db->~Reindexer();
        db = nullptr;

        remove_all(GetDataDir() / "blocks");
        remove_all(GetDataDir() / "chainstate");
        remove_all(GetDataDir() / "indexes");
        remove_all(GetDataDir() / "pocketdb");

        return ConnectDB();
    }

    return true;
}

bool PocketDB::ConnectDB()
{
    db = new Reindexer();
    Error err = db->Connect("builtin://" + (GetDataDir() / "pocketdb").string());
    if (!err.ok()) {
        LogPrintf("Cannot open Reindexer DB (%s) - %s\n", (GetDataDir() / "pocketdb").string(), err.what());
        return false;
    }

    return InitDB();
}
//-----------------------------------------------------

//-----------------------------------------------------
bool findInVector(std::vector<reindexer::NamespaceDef> defs, std::string name)
{
    return std::find_if(defs.begin(), defs.end(), [&](const reindexer::NamespaceDef& nsDef) { return nsDef.name == name; }) != defs.end();
}

bool PocketDB::Init()
{
    if (!ConnectDB()) return false;
    if (!UpdateDB()) return false;

    LogPrintf("Loaded Reindexer DB (%s)\n", (GetDataDir() / "pocketdb").string());

    // Save current version
    Item service_new_item = db->NewItem("Service");
    service_new_item["version"] = cur_version;
    UpsertWithCommit("Service", service_new_item);

    // Turn on/off statistic
    // Item conf_item = db->NewItem("#config");
    // conf_item.FromJSON(R"json({
	// 	"type":"profiling", 
	// 	"profiling":{
	// 		"queriesperfstats":false,
	// 		"perfstats":false,
	// 		"memstats":false,
    //         "activitystats": false
	// 	}
	// })json");
    // UpsertWithCommit("#config", conf_item);

    return true;
}

bool PocketDB::InitDB(std::string table)
{
    // Service
    if (table == "Service" || table == "ALL") {
        db->OpenNamespace("Service", StorageOpts().Enabled().CreateIfMissing());
        db->AddIndex("Service", {"version", "tree", "int", IndexOpts().PK()});
        db->Commit("Service");
    }

    // RI Mempool
    if (table == "Mempool" || table == "ALL") {
        db->OpenNamespace("Mempool", StorageOpts().Enabled().CreateIfMissing());
        db->AddIndex("Mempool", {"txid", "hash", "string", IndexOpts().PK()});
        db->AddIndex("Mempool", {"txid_source", "hash", "string", IndexOpts()});
        db->AddIndex("Mempool", {"table", "-", "string", IndexOpts()});
        db->AddIndex("Mempool", {"data", "-", "string", IndexOpts()});
        db->Commit("Mempool");
    }

    // Account settings
    if (table == "AccountSettings" || table == "ALL") {
        db->OpenNamespace("AccountSettings", StorageOpts().Enabled().CreateIfMissing());
        db->AddIndex("AccountSettings", {"txid", "hash", "string", IndexOpts().PK()});
        db->AddIndex("AccountSettings", {"block", "tree", "int", IndexOpts()});
        db->AddIndex("AccountSettings", {"time", "tree", "int64", IndexOpts()});
        db->AddIndex("AccountSettings", {"address", "hash", "string", IndexOpts()});
        db->AddIndex("AccountSettings", {"data", "-", "string", IndexOpts().SetCollateMode(CollateUTF8)});
        db->Commit("AccountSettings");
    }

    // Users
    if (table == "UsersView" || table == "ALL") {
        db->OpenNamespace("UsersView", StorageOpts().Enabled().CreateIfMissing());
        db->AddIndex("UsersView", {"txid", "hash", "string", IndexOpts()});
        db->AddIndex("UsersView", {"block", "tree", "int", IndexOpts()});
        db->AddIndex("UsersView", {"time", "tree", "int64", IndexOpts()});
        db->AddIndex("UsersView", {"address", "hash", "string", IndexOpts().PK()});
        db->AddIndex("UsersView", {"name", "hash", "string", IndexOpts().SetCollateMode(CollateUTF8)});
        db->AddIndex("UsersView", {"birthday", "-", "int", IndexOpts()});
        db->AddIndex("UsersView", {"gender", "hash", "int", IndexOpts()}); // enum AccountType
        db->AddIndex("UsersView", {"regdate", "-", "int64", IndexOpts()});
        db->AddIndex("UsersView", {"avatar", "-", "string", IndexOpts()});
        db->AddIndex("UsersView", {"about", "-", "string", IndexOpts().SetCollateMode(CollateUTF8)});
        db->AddIndex("UsersView", {"lang", "-", "string", IndexOpts().SetCollateMode(CollateUTF8)});
        db->AddIndex("UsersView", {"url", "-", "string", IndexOpts().SetCollateMode(CollateUTF8)});
        db->AddIndex("UsersView", {"pubkey", "-", "string", IndexOpts()});
        db->AddIndex("UsersView", {"donations", "-", "string", IndexOpts()});
        db->AddIndex("UsersView", {"referrer", "hash", "string", IndexOpts()});
        db->AddIndex("UsersView", {"id", "hash", "int", IndexOpts()});
        db->AddIndex("UsersView", {"reputation", "-", "int", IndexOpts()});
        db->AddIndex("UsersView", {"name_text", {"name"}, "text", "composite", IndexOpts().SetCollateMode(CollateUTF8)});
        db->Commit("UsersView");
    }

    // RI UsersHistory
    if (table == "Users" || table == "ALL") {
        db->OpenNamespace("Users", StorageOpts().Enabled().CreateIfMissing());
        db->AddIndex("Users", {"txid", "hash", "string", IndexOpts().PK()});
        db->AddIndex("Users", {"block", "tree", "int", IndexOpts()});
        db->AddIndex("Users", {"time", "tree", "int64", IndexOpts()});
        db->AddIndex("Users", {"address", "hash", "string", IndexOpts()});
        db->AddIndex("Users", {"name", "-", "string", IndexOpts().SetCollateMode(CollateUTF8)});
        db->AddIndex("Users", {"birthday", "-", "int", IndexOpts()});
        db->AddIndex("Users", {"gender", "-", "int", IndexOpts()}); // enum AccountType
        db->AddIndex("Users", {"regdate", "-", "int64", IndexOpts()});
        db->AddIndex("Users", {"avatar", "-", "string", IndexOpts()});
        db->AddIndex("Users", {"about", "-", "string", IndexOpts().SetCollateMode(CollateUTF8)});
        db->AddIndex("Users", {"lang", "-", "string", IndexOpts().SetCollateMode(CollateUTF8)});
        db->AddIndex("Users", {"url", "-", "string", IndexOpts().SetCollateMode(CollateUTF8)});
        db->AddIndex("Users", {"pubkey", "-", "string", IndexOpts()});
        db->AddIndex("Users", {"donations", "-", "string", IndexOpts()});
        db->AddIndex("Users", {"referrer", "-", "string", IndexOpts()});
        db->AddIndex("Users", {"id", "-", "int", IndexOpts()});
        db->Commit("Users");
    }

    // UserRatings
    if (table == "UserRatings" || table == "ALL") {
        db->OpenNamespace("UserRatings", StorageOpts().Enabled().CreateIfMissing());
        db->AddIndex("UserRatings", {"block", "tree", "int", IndexOpts()});
        db->AddIndex("UserRatings", {"address", "hash", "string", IndexOpts()});
        db->AddIndex("UserRatings", {"reputation", "-", "int", IndexOpts()});
        db->AddIndex("UserRatings", {"address+block", {"address", "block"}, "hash", "composite", IndexOpts().PK()});
        db->Commit("UserRatings");
    }

    // Posts
    if (table == "Posts" || table == "ALL") {
        db->OpenNamespace("Posts", StorageOpts().Enabled().CreateIfMissing());
        db->AddIndex("Posts", {"txid", "hash", "string", IndexOpts().PK()});
        db->AddIndex("Posts", {"txidEdit", "hash", "string", IndexOpts()});
        db->AddIndex("Posts", {"txidRepost", "hash", "string", IndexOpts()});
        db->AddIndex("Posts", {"block", "tree", "int", IndexOpts()});
        db->AddIndex("Posts", {"time", "tree", "int64", IndexOpts()});
        db->AddIndex("Posts", {"address", "hash", "string", IndexOpts()});
        db->AddIndex("Posts", {"type", "hash", "int", IndexOpts()}); // enum ContentType
        db->AddIndex("Posts", {"lang", "hash", "string", IndexOpts()});
        db->AddIndex("Posts", {"caption", "-", "string", IndexOpts().SetCollateMode(CollateUTF8)});
        db->AddIndex("Posts", {"caption_", "-", "string", IndexOpts().SetCollateMode(CollateUTF8)});
        db->AddIndex("Posts", {"message", "-", "string", IndexOpts().SetCollateMode(CollateUTF8)});
        db->AddIndex("Posts", {"message_", "-", "string", IndexOpts().SetCollateMode(CollateUTF8)});
        db->AddIndex("Posts", {"tags", "-", "string", IndexOpts().Array().SetCollateMode(CollateUTF8)});
        db->AddIndex("Posts", {"url", "-", "string", IndexOpts().SetCollateMode(CollateUTF8)});
        db->AddIndex("Posts", {"images", "-", "string", IndexOpts().Array().SetCollateMode(CollateUTF8)});
        db->AddIndex("Posts", {"settings", "-", "string", IndexOpts().SetCollateMode(CollateUTF8)});
        db->AddIndex("Posts", {"scoreSum", "-", "int", IndexOpts()});
        db->AddIndex("Posts", {"scoreCnt", "-", "int", IndexOpts()});
        db->AddIndex("Posts", {"reputation", "-", "int", IndexOpts()});
        db->AddIndex("Posts", {"caption+message", {"caption_", "message_"}, "text", "composite", IndexOpts().SetCollateMode(CollateUTF8)});
        db->Commit("Posts");
    }

    // Posts Hitstory
    if (table == "PostsHistory" || table == "ALL") {
        db->OpenNamespace("PostsHistory", StorageOpts().Enabled().CreateIfMissing());
        db->AddIndex("PostsHistory", {"txid", "hash", "string", IndexOpts()});
        db->AddIndex("PostsHistory", {"txidEdit", "hash", "string", IndexOpts()});
        db->AddIndex("PostsHistory", {"txidRepost", "hash", "string", IndexOpts()});
        db->AddIndex("PostsHistory", {"block", "tree", "int", IndexOpts()});
        db->AddIndex("PostsHistory", {"time", "tree", "int64", IndexOpts()});
        db->AddIndex("PostsHistory", {"address", "hash", "string", IndexOpts()});
        db->AddIndex("PostsHistory", {"type", "-", "int", IndexOpts()}); // enum ContentType
        db->AddIndex("PostsHistory", {"lang", "-", "string", IndexOpts()});
        db->AddIndex("PostsHistory", {"caption", "-", "string", IndexOpts().SetCollateMode(CollateUTF8)});
        db->AddIndex("PostsHistory", {"message", "-", "string", IndexOpts().SetCollateMode(CollateUTF8)});
        db->AddIndex("PostsHistory", {"tags", "-", "string", IndexOpts().Array().SetCollateMode(CollateUTF8)});
        db->AddIndex("PostsHistory", {"url", "-", "string", IndexOpts().SetCollateMode(CollateUTF8)});
        db->AddIndex("PostsHistory", {"images", "-", "string", IndexOpts().Array().SetCollateMode(CollateUTF8)});
        db->AddIndex("PostsHistory", {"settings", "-", "string", IndexOpts().SetCollateMode(CollateUTF8)});
        db->AddIndex("PostsHistory", {"txid+block", {"txid", "block"}, "hash", "composite", IndexOpts().PK()});
        db->Commit("PostsHistory");
    }

    // PostRatings
    if (table == "PostRatings" || table == "ALL") {
        db->OpenNamespace("PostRatings", StorageOpts().Enabled().CreateIfMissing());
        db->AddIndex("PostRatings", {"block", "tree", "int", IndexOpts()});
        db->AddIndex("PostRatings", {"posttxid", "hash", "string", IndexOpts()});
        db->AddIndex("PostRatings", {"scoreSum", "-", "int", IndexOpts()});
        db->AddIndex("PostRatings", {"scoreCnt", "-", "int", IndexOpts()});
        db->AddIndex("PostRatings", {"reputation", "-", "int", IndexOpts()});
        db->AddIndex("PostRatings", {"posttxid+block", {"posttxid", "block"}, "hash", "composite", IndexOpts().PK()});
        db->Commit("PostRatings");
    }

    // Scores
    if (table == "Scores" || table == "ALL") {
        db->OpenNamespace("Scores", StorageOpts().Enabled().CreateIfMissing());
        db->AddIndex("Scores", {"txid", "hash", "string", IndexOpts().PK()});
        db->AddIndex("Scores", {"block", "tree", "int", IndexOpts()});
        db->AddIndex("Scores", {"time", "tree", "int64", IndexOpts()});
        db->AddIndex("Scores", {"posttxid", "hash", "string", IndexOpts()});
        db->AddIndex("Scores", {"address", "hash", "string", IndexOpts()});
        db->AddIndex("Scores", {"value", "-", "int", IndexOpts()});
        db->Commit("Scores");
    }

    // Subscribes
    if (table == "SubscribesView" || table == "ALL") {
        db->OpenNamespace("SubscribesView", StorageOpts().Enabled().CreateIfMissing());
        db->AddIndex("SubscribesView", {"txid", "hash", "string", IndexOpts()});
        db->AddIndex("SubscribesView", {"block", "tree", "int", IndexOpts()});
        db->AddIndex("SubscribesView", {"time", "tree", "int64", IndexOpts()});
        db->AddIndex("SubscribesView", {"address", "hash", "string", IndexOpts()});
        db->AddIndex("SubscribesView", {"address_to", "hash", "string", IndexOpts()});
        db->AddIndex("SubscribesView", {"private", "-", "bool", IndexOpts()});
        db->AddIndex("SubscribesView", {"address+address_to", {"address", "address_to"}, "hash", "composite", IndexOpts().PK()});
        db->Commit("SubscribesView");
    }

    // RI SubscribesHistory
    if (table == "Subscribes" || table == "ALL") {
        db->OpenNamespace("Subscribes", StorageOpts().Enabled().CreateIfMissing());
        db->AddIndex("Subscribes", {"txid", "hash", "string", IndexOpts().PK()});
        db->AddIndex("Subscribes", {"block", "tree", "int", IndexOpts()});
        db->AddIndex("Subscribes", {"time", "tree", "int64", IndexOpts()});
        db->AddIndex("Subscribes", {"address", "hash", "string", IndexOpts()});
        db->AddIndex("Subscribes", {"address_to", "hash", "string", IndexOpts()});
        db->AddIndex("Subscribes", {"private", "-", "bool", IndexOpts()});
        db->AddIndex("Subscribes", {"unsubscribe", "-", "bool", IndexOpts()});
        db->Commit("Subscribes");
    }

    // Blocking
    if (table == "BlockingView" || table == "ALL") {
        db->OpenNamespace("BlockingView", StorageOpts().Enabled().CreateIfMissing());
        db->AddIndex("BlockingView", {"txid", "hash", "string", IndexOpts().PK()});
        db->AddIndex("BlockingView", {"block", "tree", "int", IndexOpts()});
        db->AddIndex("BlockingView", {"time", "tree", "int64", IndexOpts()});
        db->AddIndex("BlockingView", {"address", "hash", "string", IndexOpts()});
        db->AddIndex("BlockingView", {"address_to", "hash", "string", IndexOpts()});
        db->AddIndex("BlockingView", {"address_reputation", "-", "int", IndexOpts()});
        db->Commit("BlockingView");
    }

    // BlockingHistory
    if (table == "Blocking" || table == "ALL") {
        db->OpenNamespace("Blocking", StorageOpts().Enabled().CreateIfMissing());
        db->AddIndex("Blocking", {"txid", "hash", "string", IndexOpts().PK()});
        db->AddIndex("Blocking", {"block", "tree", "int", IndexOpts()});
        db->AddIndex("Blocking", {"time", "tree", "int64", IndexOpts()});
        db->AddIndex("Blocking", {"address", "hash", "string", IndexOpts()});
        db->AddIndex("Blocking", {"address_to", "hash", "string", IndexOpts()});
        db->AddIndex("Blocking", {"unblocking", "-", "bool", IndexOpts()});
        db->Commit("Blocking");
    }

    // Complains
    if (table == "Complains" || table == "ALL") {
        db->OpenNamespace("Complains", StorageOpts().Enabled().CreateIfMissing());
        db->AddIndex("Complains", {"txid", "hash", "string", IndexOpts().PK()});
        db->AddIndex("Complains", {"block", "tree", "int", IndexOpts()});
        db->AddIndex("Complains", {"time", "tree", "int64", IndexOpts()});
        db->AddIndex("Complains", {"posttxid", "hash", "string", IndexOpts()});
        db->AddIndex("Complains", {"address", "hash", "string", IndexOpts()});
        db->AddIndex("Complains", {"reason", "-", "int", IndexOpts()});
        db->Commit("Complains");
    }

    // UTXO
    if (table == "UTXO" || table == "ALL") {
        db->OpenNamespace("UTXO", StorageOpts().Enabled().CreateIfMissing());
        db->AddIndex("UTXO", {"txid", "-", "string", IndexOpts()});
        db->AddIndex("UTXO", {"txout", "-", "int", IndexOpts()});
        db->AddIndex("UTXO", {"time", "-", "int64", IndexOpts()});
        db->AddIndex("UTXO", {"block", "tree", "int", IndexOpts()});
        db->AddIndex("UTXO", {"address", "hash", "string", IndexOpts()});
        db->AddIndex("UTXO", {"amount", "-", "int64", IndexOpts()});
        db->AddIndex("UTXO", {"spent_block", "tree", "int", IndexOpts()});
        db->AddIndex("UTXO", {"txid+txout", {"txid", "txout"}, "hash", "composite", IndexOpts().PK()});
        db->Commit("UTXO");
    }

    // Addresses
    if (table == "Addresses" || table == "ALL") {
        db->OpenNamespace("Addresses", StorageOpts().Enabled().CreateIfMissing());
        db->AddIndex("Addresses", {"txid", "hash", "string", IndexOpts()});
        db->AddIndex("Addresses", {"block", "tree", "int", IndexOpts()});
        db->AddIndex("Addresses", {"address", "hash", "string", IndexOpts().PK()});
        db->AddIndex("Addresses", {"time", "tree", "int64", IndexOpts()});
        db->Commit("Addresses");
    }

    // Comment
    if (table == "Comment" || table == "ALL") {
        db->OpenNamespace("Comment", StorageOpts().Enabled().CreateIfMissing());
        db->AddIndex("Comment", {"txid", "hash", "string", IndexOpts().PK()});
        db->AddIndex("Comment", {"otxid", "hash", "string", IndexOpts()});
        db->AddIndex("Comment", {"last", "-", "bool", IndexOpts()});
        db->AddIndex("Comment", {"postid", "hash", "string", IndexOpts()});
        db->AddIndex("Comment", {"address", "hash", "string", IndexOpts()});
        db->AddIndex("Comment", {"time", "tree", "int64", IndexOpts()});
        db->AddIndex("Comment", {"block", "tree", "int", IndexOpts()});
        db->AddIndex("Comment", {"msg", "-", "string", IndexOpts().SetCollateMode(CollateUTF8)});
        db->AddIndex("Comment", {"parentid", "hash", "string", IndexOpts()});
        db->AddIndex("Comment", {"answerid", "hash", "string", IndexOpts()});
        db->AddIndex("Comment", {"scoreUp", "-", "int", IndexOpts()});
        db->AddIndex("Comment", {"scoreDown", "-", "int", IndexOpts()});
        db->AddIndex("Comment", {"reputation", "-", "int", IndexOpts()});
        db->Commit("Comment");
    }

    // CommentRatings
    if (table == "CommentRatings" || table == "ALL") {
        db->OpenNamespace("CommentRatings", StorageOpts().Enabled().CreateIfMissing());
        db->AddIndex("CommentRatings", {"block", "tree", "int", IndexOpts()});
        db->AddIndex("CommentRatings", {"commentid", "hash", "string", IndexOpts()});
        db->AddIndex("CommentRatings", {"scoreUp", "-", "int", IndexOpts()});
        db->AddIndex("CommentRatings", {"scoreDown", "-", "int", IndexOpts()});
        db->AddIndex("CommentRatings", {"reputation", "-", "int", IndexOpts()});
        db->AddIndex("CommentRatings", {"commentid+block", {"commentid", "block"}, "hash", "composite", IndexOpts().PK()});
        db->Commit("CommentRatings");
    }

    // CommentScores
    if (table == "CommentScores" || table == "ALL") {
        db->OpenNamespace("CommentScores", StorageOpts().Enabled().CreateIfMissing());
        db->AddIndex("CommentScores", {"txid", "hash", "string", IndexOpts().PK()});
        db->AddIndex("CommentScores", {"block", "tree", "int", IndexOpts()});
        db->AddIndex("CommentScores", {"time", "tree", "int64", IndexOpts()});
        db->AddIndex("CommentScores", {"commentid", "hash", "string", IndexOpts()});
        db->AddIndex("CommentScores", {"address", "hash", "string", IndexOpts()});
        db->AddIndex("CommentScores", {"value", "-", "int", IndexOpts()});
        db->Commit("CommentScores");
    }

    // Cumulative ratings table
    // Type for split:
    // ? - user reputation
    // 1 - user scores - for threshold reputation
    // ? - post rating
    // ? - comment rating
    if (table == "Ratings" || table == "ALL") {
        db->OpenNamespace("Ratings", StorageOpts().Enabled().CreateIfMissing());
        db->AddIndex("Ratings", {"type", "hash", "int", IndexOpts()});
        db->AddIndex("Ratings", {"block", "tree", "int", IndexOpts()});
        db->AddIndex("Ratings", {"key", "hash", "int", IndexOpts()});
        db->AddIndex("Ratings", {"value", "hash", "int", IndexOpts()});
        db->AddIndex("Ratings", {"type+block+key+value", {"type", "block", "key", "value"}, "hash", "composite", IndexOpts().PK()});
        // -----------------------------------
        db->Commit("Ratings");
    }

    return true;
}

bool PocketDB::DropTable(std::string table)
{
    Error err = db->DropNamespace(table);
    if (!err.ok()) LogPrintf("Drop namespace(%s) %s\n", table, err.what());

    err = db->Commit(table);
    if (!err.ok()) LogPrintf("Commit drop namespace(%s) %s\n", table, err.what());

    return InitDB(table);
}

bool PocketDB::CheckIndexes(UniValue& obj)
{
    Error err;
    bool ret = true;
    std::vector<NamespaceDef> nss;
    db->EnumNamespaces(nss, false);

    for (NamespaceDef& ns : nss) {
        if (std::find_if(ns.indexes.begin(), ns.indexes.end(), [&](const IndexDef& idx) { return idx.name_ == "txid"; }) == ns.indexes.end()) {
            continue;
        }
        //--------------------------
        QueryResults _res;
        err = db->Select(Query(ns.name).ReqTotal(), _res);
        ret = ret && err.ok();
        //--------------------------
        if (err.ok()) {
            UniValue tbl(UniValue::VARR);
            for (auto& it : _res) {
                Item itm(it.GetItem());
                tbl.push_back(itm["txid"].As<string>());
            }
            obj.pushKV(ns.name, tbl);
        }
    }
    //--------------------------
    return ret;
}

bool PocketDB::GetStatistic(std::string table, UniValue& obj)
{
    Error err;
    if (table != "") {
        QueryResults _res;
        err = db->Select(reindexer::Query(table).Sort("block", true), _res);
        if (err.ok()) {
            std::string retString;
            for (auto& it : _res) {
                retString += it.GetItem().GetJSON().ToString();
                retString += "\n";
            }
            obj = retString;
        }

        return err.ok();
    }
    //--------------------------
    bool ret = true;
    std::vector<NamespaceDef> nss;
    db->EnumNamespaces(nss, false);

    struct NamespaceDefSortComp {
        inline bool operator()(const NamespaceDef& ns1, const NamespaceDef& ns2)
        {
            return ns1.name < ns2.name;
        }
    };

    std::sort(nss.begin(), nss.end(), NamespaceDefSortComp());

    for (NamespaceDef& ns : nss) {
        if (ns.name[0] == '#') continue;
        //--------------------------
        QueryResults _res;
        err = db->Select(reindexer::Query(ns.name).ReqTotal(), _res);
        ret = ret && err.ok();
        if (err.ok()) obj.pushKV(ns.name, (uint64_t)_res.TotalCount());
    }

    return ret;
}

bool PocketDB::Exists(Query query)
{
    Item _itm;
    return SelectOne(query, _itm).ok();
}

size_t PocketDB::SelectTotalCount(std::string table)
{
    Error err;
    QueryResults _res;
    err = db->Select(Query(table).ReqTotal(), _res);
    if (err.ok())
        return _res.TotalCount();
    else
        return 0;
}

size_t PocketDB::SelectCount(Query query)
{
    QueryResults _res;
    if (db->Select(query, _res).ok())
        return _res.Count();
    else
        return 0;
}

Error PocketDB::Select(Query query, QueryResults& res)
{
    reindexer::Query _query(query);
    return db->Select(_query, res);
}

Error PocketDB::SelectOne(Query query, Item& item)
{
    reindexer::Query _query(query);
    _query.start = 0;
    _query.count = 1;
    QueryResults res;
    Error err = db->Select(_query, res);
    if (err.ok()) {
        if (res.Count() > 0) {
            item = res[0].GetItem();
        } else {
            return Error(13);
        }
    }

    return err;
}

Error PocketDB::SelectAggr(Query query, QueryResults& aggRes)
{
    Error err = db->Select(query, aggRes);
    if (err.ok()) {
        if (aggRes.aggregationResults.size() > 0) {
            return err;
        } else {
            return Error(13);
        }
    }

    return err;
}

Error PocketDB::SelectAggr(Query query, std::string aggId, AggregationResult& aggRes)
{
    QueryResults res;
    Error err = db->Select(query, res);
    if (err.ok()) {
        if (res.aggregationResults.size() > 0) {
            aggRes = std::find_if(res.aggregationResults.begin(), res.aggregationResults.end(),
                [&](const reindexer::AggregationResult& agg) { return agg.name == aggId; })[0];
        } else {
            return Error(13);
        }
    }

    return err;
}

Error PocketDB::Upsert(std::string table, Item& item)
{
    return db->Upsert(table, item);
}

Error PocketDB::UpsertWithCommit(std::string table, Item& item)
{
    Error err = db->Upsert(table, item);
    if (err.ok()) return db->Commit(table);
    return err;
}

Error PocketDB::Delete(Query query)
{
    QueryResults res;
    Error err = db->Delete(query, res);

    return err;
}

Error PocketDB::DeleteWithCommit(Query query)
{
    size_t deleted = 0;
    return DeleteWithCommit(query, deleted);
}

Error PocketDB::DeleteWithCommit(Query query, size_t& deleted)
{
    QueryResults res;

    Error err = db->Delete(query, res);
    deleted = res.Count();

    if (err.ok()) {
        deleted = res.Count();
        return db->Commit(query._namespace);
    }

    return err;
}


Error PocketDB::Update(std::string table, Item& item, bool commit)
{
    Error err = db->Update(table, item);
    if (err.ok() && commit) return db->Commit(table);
    return err;
}

Error PocketDB::UpdateUsersView(std::string address, int height)
{
    Item _user_itm;
    Error err;
    if (height >= Params().GetConsensus().checkpoint_split_content_video)
        err = SelectOne(Query("Users").Where("address", CondEq, address).Sort("block", true), _user_itm);
    else
        err = SelectOne(Query("Users").Where("address", CondEq, address).Sort("time", true), _user_itm);

    // For rollback
    if (err.code() == 13) return DeleteWithCommit(Query("UsersView").Where("address", CondEq, address));

    // In Users exists account - update view
    if (err.ok()) {

        // Save referrer from first transaction or from new first transaction
        auto referrer = _user_itm["referrer"].As<string>();
        reindexer::Item user_cur;
        if (g_pocketdb->SelectOne(reindexer::Query("UsersView").Where("address", CondEq, address), user_cur).ok()) {
            referrer = user_cur["referrer"].As<string>();
        }

        Item _view_itm = db->NewItem("UsersView");
        _view_itm["txid"] = _user_itm["txid"].As<string>();
        _view_itm["block"] = _user_itm["block"].As<int>();
        _view_itm["time"] = _user_itm["time"].As<int64_t>();
        _view_itm["address"] = _user_itm["address"].As<string>();
        _view_itm["name"] = _user_itm["name"].As<string>();
        _view_itm["birthday"] = _user_itm["birthday"].As<int>();
        _view_itm["gender"] = _user_itm["gender"].As<int>();
        _view_itm["regdate"] = _user_itm["regdate"].As<int64_t>();
        _view_itm["avatar"] = _user_itm["avatar"].As<string>();
        _view_itm["about"] = _user_itm["about"].As<string>();
        _view_itm["lang"] = _user_itm["lang"].As<string>();
        _view_itm["url"] = _user_itm["url"].As<string>();
        _view_itm["pubkey"] = _user_itm["pubkey"].As<string>();
        _view_itm["donations"] = _user_itm["donations"].As<string>();
        _view_itm["referrer"] = referrer;
        _view_itm["id"] = _user_itm["id"].As<int>();
        _view_itm["reputation"] = GetUserReputation(address, height);

        return UpsertWithCommit("UsersView", _view_itm);
    }

    return err;
}

Error PocketDB::UpdateSubscribesView(std::string address, std::string address_to, int height)
{
    DeleteWithCommit(Query("SubscribesView").Where("address", CondEq, address).Where("address_to", CondEq, address_to));

    QueryResults _res;
    Error err;

    if (height >= Params().GetConsensus().checkpoint_split_content_video)
        err = db->Select(Query("Subscribes", 0, 1).Where("address", CondEq, address).Where("address_to", CondEq, address_to).Sort("block", true), _res);
    else
        err = db->Select(Query("Subscribes", 0, 1).Where("address", CondEq, address).Where("address_to", CondEq, address_to).Sort("time", true), _res);

    if (err.ok() && _res.Count() > 0) {
        Item _itm = _res[0].GetItem();
        if (!_itm["unsubscribe"].As<bool>())
            return UpsertWithCommit("SubscribesView", _itm);
    }
    //-----------------
    return err;
}

Error PocketDB::UpdateBlockingView(std::string address, std::string address_to, int height)
{
    Item _blocking_itm;
    Error err;
    if (height >= Params().GetConsensus().checkpoint_split_content_video)
        err = SelectOne(Query("Blocking").Where("address", CondEq, address).Where("address_to", CondEq, address_to).Sort("block", true), _blocking_itm);
    else
        err = SelectOne(Query("Blocking").Where("address", CondEq, address).Where("address_to", CondEq, address_to).Sort("time", true), _blocking_itm);

    if (err.code() == 13) return DeleteWithCommit(Query("BlockingView").Where("address", CondEq, address).Where("address_to", CondEq, address_to));
    if (err.ok()) {
        if (_blocking_itm["unblocking"].As<bool>() == true) {
            return DeleteWithCommit(Query("BlockingView").Where("address", CondEq, address).Where("address_to", CondEq, address_to));
        } else {
            Item _blocking_view_itm = db->NewItem("BlockingView");
            _blocking_view_itm["txid"] = _blocking_itm["txid"].As<string>();
            _blocking_view_itm["block"] = _blocking_itm["block"].As<int>();
            _blocking_view_itm["time"] = _blocking_itm["time"].As<int64_t>();
            _blocking_view_itm["address"] = _blocking_itm["address"].As<string>();
            _blocking_view_itm["address_to"] = _blocking_itm["address_to"].As<string>();
            _blocking_view_itm["address_reputation"] = GetUserReputation(address, _blocking_itm["block"].As<int>());

            return UpsertWithCommit("BlockingView", _blocking_view_itm);
        }
    }

    return err;
}


Error PocketDB::CommitPostItem(Item& itm, int height)
{
    Error err;

    // Move exists Post to history table
    if (itm["txidEdit"].As<string>() != "") {
        Item cur_post_item;
        if (SelectOne(Query("Posts").Where("txid", CondEq, itm["txid"].As<string>()), cur_post_item).ok()) {
            Item hist_post_item = db->NewItem("PostsHistory");
            hist_post_item["txid"] = cur_post_item["txid"].As<string>();
            hist_post_item["txidEdit"] = cur_post_item["txidEdit"].As<string>();
            hist_post_item["txidRepost"] = cur_post_item["txidRepost"].As<string>();
            hist_post_item["block"] = cur_post_item["block"].As<int>();
            hist_post_item["time"] = cur_post_item["time"].As<int64_t>();
            hist_post_item["address"] = cur_post_item["address"].As<string>();
            hist_post_item["type"] = cur_post_item["type"].As<int>();
            hist_post_item["lang"] = cur_post_item["lang"].As<string>();
            hist_post_item["caption"] = cur_post_item["caption"].As<string>();
            hist_post_item["message"] = cur_post_item["message"].As<string>();
            hist_post_item["url"] = cur_post_item["url"].As<string>();
            hist_post_item["settings"] = cur_post_item["settings"].As<string>();

            VariantArray vaTags = cur_post_item["tags"];
            hist_post_item["tags"] = vaTags;

            VariantArray vaImages = cur_post_item["images"];
            hist_post_item["images"] = vaImages;

            err = UpsertWithCommit("PostsHistory", hist_post_item);
            if (!err.ok()) return err;

            err = DeleteWithCommit(Query("Posts").Where("txid", CondEq, itm["txid"].As<string>()));
            if (!err.ok()) return err;
        }

        // Restore rating for Edit Post
        int sum = 0;
        int cnt = 0;
        int rep = 0;
        GetPostRating(itm["txid"].As<string>(), sum, cnt, rep, height);
        itm["scoreSum"] = sum;
        itm["scoreCnt"] = cnt;
        itm["reputation"] = rep;
    }

    // Insert new Post
    err = UpsertWithCommit("Posts", itm);
    return err;
}

Error PocketDB::RestorePostItem(std::string posttxid, int height)
{
    // Move exists Post to history table
    Item hist_post_item;
    Error err = SelectOne(Query("PostsHistory").Where("txid", CondEq, posttxid).Sort("block", true), hist_post_item);
    if (err.ok()) {
        std::string posttxid_edit = hist_post_item["txidEdit"].As<string>();

        Item post_item = db->NewItem("Posts");
        post_item["txid"] = hist_post_item["txid"].As<string>();
        post_item["txidEdit"] = posttxid_edit;
        post_item["txidRepost"] = hist_post_item["txidRepost"].As<string>();
        post_item["block"] = hist_post_item["block"].As<int>();
        post_item["time"] = hist_post_item["time"].As<int64_t>();
        post_item["address"] = hist_post_item["address"].As<string>();
        post_item["type"] = hist_post_item["type"].As<int>();
        post_item["lang"] = hist_post_item["lang"].As<string>();
        post_item["caption"] = hist_post_item["caption"].As<string>();
        post_item["message"] = hist_post_item["message"].As<string>();
        post_item["url"] = hist_post_item["url"].As<string>();
        post_item["settings"] = hist_post_item["settings"].As<string>();

        VariantArray vaTags = hist_post_item["tags"];
        post_item["tags"] = vaTags;

        VariantArray vaImages = hist_post_item["images"];
        post_item["images"] = vaImages;

        std::string caption_decoded = UrlDecode(post_item["caption"].As<string>());
        post_item["caption_"] = ClearHtmlTags(caption_decoded);

        std::string message_decoded = UrlDecode(post_item["message"].As<string>());
        post_item["message_"] = ClearHtmlTags(message_decoded);

        // Restore rating for Post
        int sum = 0;
        int cnt = 0;
        int rep = 0;
        GetPostRating(posttxid, sum, cnt, rep, height);
        post_item["scoreSum"] = sum;
        post_item["scoreCnt"] = cnt;
        post_item["reputation"] = rep;

        // Before restore need delete current item
        err = DeleteWithCommit(Query("Posts").Where("txid", CondEq, posttxid));
        if (!err.ok()) return err;

        // Restore Post item
        err = UpsertWithCommit("Posts", post_item);
        if (!err.ok()) return err;

        // Clear history
        err = DeleteWithCommit(Query("PostsHistory").Where("txid", CondEq, posttxid).Where("txidEdit", CondEq, posttxid_edit));
        return err;
    } else if (err.code() == 13) {
        // History is empty - its normal, simple remove post
        return DeleteWithCommit(Query("Posts").Where("txid", CondEq, posttxid));
    } else {
        return err;
    }
}


Error PocketDB::CommitLastItem(std::string table, Item& itm, int height)
{
    // Disable all founded last items
    QueryResults all_res;
    Error err = db->Select(Query(table).Where("otxid", CondEq, itm["otxid"].As<string>()).Where("last", CondEq, true), all_res);
    if (!err.ok()) return err;
    for (auto& it : all_res) {
        Item _itm = it.GetItem();
        _itm["last"] = false;
        _itm["scoreUp"] = 0;
        _itm["scoreDown"] = 0;
        _itm["reputation"] = 0;
        err = UpsertWithCommit(table, _itm);
        if (!err.ok()) return err;
    }

    // Restore rating
    int up = 0;
    int down = 0;
    int rep = 0;
    GetCommentRating(itm["otxid"].As<string>(), up, down, rep, height);
    itm["scoreUp"] = up;
    itm["scoreDown"] = down;
    itm["reputation"] = rep;

    // Insert new item
    itm["last"] = true;
    err = UpsertWithCommit(table, itm);
    return err;
}

Error PocketDB::RestoreLastItem(std::string table, std::string txid, std::string otxid, int height)
{
    // delete last by txid
    Error err = DeleteWithCommit(Query(table).Where("txid", CondEq, txid));
    if (!err.ok()) return err;

    // select last
    QueryResults last_res;
    err = db->Select(Query(table, 0, 1).Where("otxid", CondEq, otxid).Sort("block", true), last_res);
    if (err.ok()) {
        if (last_res.Count() > 0) {
            Item last_item = last_res[0].GetItem();

            // Make this comment as lasted
            last_item["last"] = true;

            // Restore rating
            int up = 0;
            int down = 0;
            int rep = 0;

            // Warning! This method only for comments, not for posts or users
            GetCommentRating(otxid, up, down, rep, height);

            last_item["scoreUp"] = up;
            last_item["scoreDown"] = down;
            last_item["reputation"] = rep;

            err = UpsertWithCommit(table, last_item);
            return err;

        } else {
            return Error(errOK);
        }
    } else {
        return err;
    }
}


int64_t PocketDB::GetUserBalance(std::string _address, int height)
{
    AggregationResult aggRes;
    if (SelectAggr(
            Query("UTXO")
                .Where("address", CondEq, _address)
                .Where("block", CondLt, height)
                .Where("spent_block", CondEq, 0)
                .Aggregate("amount", AggSum),
            "amount", aggRes)
            .ok()) {
        return (int64_t)aggRes.value;
    } else {
        return 0;
    }
}

std::tuple<int, int> PocketDB::GetUserData(std::string address)
{
    Item itmUserView;
    if (SelectOne(Query("Users").Where("address", CondEq, address).Sort("block", false).Limit(1), itmUserView).ok())
        return std::make_tuple(itmUserView["id"].As<int>(), itmUserView["block"].As<int>());

    return std::make_tuple(-1, -1);
}

int PocketDB::GetUserReputation(std::string _address, int height)
{
    // Set to default if rating for user not found
    int rep = 0;

    // Sorting by block desc - last accumulating rating
    Item _itm_rating;
    if (SelectOne(
            Query("UserRatings")
                .Where("address", CondEq, _address)
                .Where("block", CondLe, height)
                .Sort("block", true),
            _itm_rating)
            .ok()) {
        rep = _itm_rating["reputation"].As<int>();
    }

    return rep;
}

int PocketDB::GetUserLikersCount(int userId, int height)
{
    return SelectCount(
        Query("Ratings")
            .Where("type", CondEq, (int)RatingType::RatingUserLikers)
            .Where("key", CondEq, userId)
            .Where("block", CondLe, height));
}

bool PocketDB::ExistsUserLiker(int userId, int likerId)
{
    return Exists(
        Query("Ratings")
            .Where("type", CondEq, (int)RatingType::RatingUserLikers)
            .Where("key", CondEq, userId)
            .Where("value", CondEq, likerId)
    );
}

bool PocketDB::SetUserReputation(std::string address, int rep)
{
    reindexer::QueryResults userViewRes;
    if (!db->Select(reindexer::Query("UsersView", 0, 1).Where("address", CondEq, address), userViewRes).ok()) return false;
    for (auto& r : userViewRes) {
        reindexer::Item userViewItm(r.GetItem());
        userViewItm["reputation"] = rep;
        if (!UpsertWithCommit("UsersView", userViewItm).ok()) return false;
    }

    return true;
}

bool PocketDB::UpdateUserReputation(std::string address, int height)
{
    int rep = GetUserReputation(address, height);
    return SetUserReputation(address, rep);
}


void PocketDB::GetPostRating(std::string posttxid, int& sum, int& cnt, int& rep, int height)
{
    // Set to default if rating for post not found
    sum = 0;
    cnt = 0;
    rep = 0;

    // Sorting by block desc - last accumulating rating
    Item _itm_rating_cur;
    if (SelectOne(
            Query("PostRatings")
                .Where("posttxid", CondEq, posttxid)
                .Where("block", CondLe, height)
                .Sort("block", true),
            _itm_rating_cur)
            .ok()) {
        sum = _itm_rating_cur["scoreSum"].As<int>();
        cnt = _itm_rating_cur["scoreCnt"].As<int>();
        rep = _itm_rating_cur["reputation"].As<int>();
    }
}

bool PocketDB::UpdatePostRating(std::string posttxid, int sum, int cnt, int& rep)
{
    reindexer::QueryResults postsRes;
    if (!db->Select(reindexer::Query("Posts", 0, 1).Where("txid", CondEq, posttxid), postsRes).ok()) return false;
    for (auto& p : postsRes) {
        reindexer::Item postItm(p.GetItem());
        postItm["scoreSum"] = sum;
        postItm["scoreCnt"] = cnt;
        postItm["reputation"] = rep;
        if (!UpsertWithCommit("Posts", postItm).ok()) return false;
    }

    return true;
}

bool PocketDB::UpdatePostRating(std::string posttxid, int height)
{
    int sum = 0;
    int cnt = 0;
    int rep = 0;
    GetPostRating(posttxid, sum, cnt, rep, height);

    return UpdatePostRating(posttxid, sum, cnt, rep);
}


void PocketDB::GetCommentRating(std::string commentid, int& up, int& down, int& rep, int height)
{
    // Set to default if rating for post not found
    up = 0;
    down = 0;
    rep = 0;

    // Sorting by block desc - last accumulating rating
    Item _itm_rating_cur;
    if (SelectOne(
            Query("CommentRatings")
                .Where("commentid", CondEq, commentid)
                .Where("block", CondLe, height)
                .Sort("block", true),
            _itm_rating_cur)
            .ok()) {
        up = _itm_rating_cur["scoreUp"].As<int>();
        down = _itm_rating_cur["scoreDown"].As<int>();
        rep = _itm_rating_cur["reputation"].As<int>();
    }
}

bool PocketDB::UpdateCommentRating(std::string commentid, int up, int down, int& rep)
{
    reindexer::QueryResults commentRes;
    if (!db->Select(reindexer::Query("Comment", 0, 1).Where("otxid", CondEq, commentid).Where("last", CondEq, true), commentRes).ok()) return false;
    for (auto& p : commentRes) {
        reindexer::Item postItm(p.GetItem());
        postItm["scoreUp"] = up;
        postItm["scoreDown"] = down;
        postItm["reputation"] = rep;
        if (!UpsertWithCommit("Comment", postItm).ok()) return false;
    }

    return true;
}

bool PocketDB::UpdateCommentRating(std::string commentid, int height)
{
    int up = 0;
    int down = 0;
    int rep = 0;
    GetCommentRating(commentid, up, down, rep, height);
    return UpdateCommentRating(commentid, up, down, rep);
}


void PocketDB::SearchTags(std::string search, int count, std::map<std::string, int>& tags, int& totalCount)
{
    if (search.size() < 3) return;
    int _count = (count > 1000 ? 1000 : count);

    reindexer::QueryResults res;
    if (g_pocketdb->Select(reindexer::Query("Tags", 0, _count).Where("tag", CondEq, (search.size() > 0 ? search : "*")), res).ok()) {
        totalCount = res.TotalCount();
        for (auto& it : res) {
            Item _itm = it.GetItem();
            tags.insert_or_assign(_itm["tag"].As<string>(), _itm["rating"].As<int>());
        }
    }
}

bool PocketDB::GetHashItem(Item& item, std::string table, std::string& out_hash)
{
    std::string data = "";
    //------------------------
    if (table == "Posts") {

        if (item["type"].As<int>() == (int)ContentType::ContentDelete)
        {
            // txid (original tx id) + settings (json as string)
            data += item["txid"].As<string>();
            data += item["settings"].As<string>();
        }
        else
        {
            // self.url.v + self.caption.v + self.message.v + self.tags.v.join(',') + self.images.v.join(',') + self.txid_edit.v
            data += item["url"].As<string>();
            data += item["caption"].As<string>();
            data += item["message"].As<string>();

            reindexer::VariantArray va_tags = item["tags"];
            for (size_t i = 0; i < va_tags.size(); ++i)
                data += (i ? "," : "") + va_tags[i].As<string>();

            reindexer::VariantArray va_images = item["images"];
            for (size_t i = 0; i < va_images.size(); ++i)
                data += (i ? "," : "") + va_images[i].As<string>();

            data += item["txidEdit"].As<string>().empty() ? "" : item["txid"].As<string>();
            data += item["txidRepost"].As<string>();
        }
    }

    if (table == "Scores") {
        // self.share.v + self.value.v
        data += item["posttxid"].As<string>();
        data += std::to_string(item["value"].As<int>());
    }

    if (table == "Complains") {
        // self.share.v + '_' + self.reason.v
        data += item["posttxid"].As<string>();
        data += "_";
        data += std::to_string(item["reason"].As<int>());
    }

    if (table == "Subscribes") {
        // self.address.v
        data += item["address_to"].As<string>();
    }

    if (table == "Blocking") {
        // self.address.v
        data += item["address_to"].As<string>();
    }

    if (table == "Users") {
        // self.name.v + self.site.v + self.language.v + self.about.v + self.image.v + JSON.stringify(self.addresses.v)
        data += item["name"].As<string>();
        data += item["url"].As<string>();
        data += item["lang"].As<string>();
        data += item["about"].As<string>();
        data += item["avatar"].As<string>();
        data += item["donations"].As<string>();
        data += item["referrer"].As<string>();
        data += item["pubkey"].As<string>();
    }

    if (table == "Comment") {
        data += item["postid"].As<string>();
        data += item["msg"].As<string>();
        data += item["parentid"].As<string>();
        data += item["answerid"].As<string>();
    }

    if (table == "CommentScores") {
        data += item["commentid"].As<string>();
        data += std::to_string(item["value"].As<int>());
    }

    if (table == "AccountSettings") {
        // data
        data += item["data"].As<string>();
    }
    //------------------------
    // Compute hash for serialized item data
    unsigned char hash[32] = {};
    CSHA256().Write((const unsigned char*)data.data(), data.size()).Finalize(hash);
    CSHA256().Write(hash, 32).Finalize(hash);
    std::vector<unsigned char> vec(hash, hash + sizeof(hash));
    out_hash = HexStr(vec);
    return true;
}
