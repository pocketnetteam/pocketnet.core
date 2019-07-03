// Copyright (c) 2018 PocketNet developers
// PocketDB general wrapper
//-----------------------------------------------------
#include "pocketdb/pocketdb.h"
#include "html.h"
#include "tools/logger.h"
//-----------------------------------------------------
std::unique_ptr<PocketDB> g_pocketdb;
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
}
//-----------------------------------------------------

//-----------------------------------------------------
bool findInVector(std::vector<reindexer::NamespaceDef> defs, std::string name)
{
    return std::find_if(defs.begin(), defs.end(), [&](const reindexer::NamespaceDef& nsDef) { return nsDef.name == name; }) != defs.end();
}

bool PocketDB::Init()
{
    db = new Reindexer();

    Error err;
    err = db->Connect("builtin://" + (GetDataDir() / "pocketdb").string());
    if (!err.ok()) {
        LogPrintf("Error loading Reindexer DB (%s) - %s\n", (GetDataDir() / "pocketdb").string(), err.what());
        return false;
    }

    InitDB();
    LogPrintf("Loaded Reindexer DB (%s)\n", (GetDataDir() / "pocketdb").string());

    return true;
}

bool PocketDB::InitDB(std::string table)
{
	// RI Mempool
    if (table == "Mempool" || table == "ALL") {
        db->OpenNamespace("Mempool", StorageOpts().Enabled().CreateIfMissing());
        db->AddIndex("Mempool", {"txid", "hash", "string", IndexOpts().PK()});
        db->AddIndex("Mempool", {"txid_source", "hash", "string", IndexOpts()});
        db->AddIndex("Mempool", {"table", "", "string", IndexOpts()});
        db->AddIndex("Mempool", {"data", "", "string", IndexOpts()});
        db->Commit("Mempool");
    }

    // Users
    if (table == "UsersView" || table == "ALL") {
        db->OpenNamespace("UsersView", StorageOpts().Enabled().CreateIfMissing());
        db->AddIndex("UsersView", {"txid", "hash", "string", IndexOpts()});
        db->AddIndex("UsersView", {"block", "tree", "int", IndexOpts()});
        db->AddIndex("UsersView", {"time", "", "int64", IndexOpts()});
        db->AddIndex("UsersView", {"address", "hash", "string", IndexOpts().PK()});
        db->AddIndex("UsersView", {"name", "hash", "string", IndexOpts().SetCollateMode(CollateUTF8)});
        db->AddIndex("UsersView", {"birthday", "", "int", IndexOpts()});
        db->AddIndex("UsersView", {"gender", "", "int", IndexOpts()});
        db->AddIndex("UsersView", {"regdate", "tree", "int64", IndexOpts()});
        db->AddIndex("UsersView", {"avatar", "", "string", IndexOpts()});
        db->AddIndex("UsersView", {"about", "", "string", IndexOpts().SetCollateMode(CollateUTF8)});
        db->AddIndex("UsersView", {"lang", "", "string", IndexOpts().SetCollateMode(CollateUTF8)});
        db->AddIndex("UsersView", {"url", "", "string", IndexOpts().SetCollateMode(CollateUTF8)});
        db->AddIndex("UsersView", {"pubkey", "", "string", IndexOpts()});
        db->AddIndex("UsersView", {"donations", "", "string", IndexOpts()});
        db->AddIndex("UsersView", {"referrer", "hash", "string", IndexOpts()});
        db->AddIndex("UsersView", {"id", "tree", "int", IndexOpts()});
        db->AddIndex("UsersView", {"scoreSum", "", "int", IndexOpts()});
        db->AddIndex("UsersView", {"scoreCnt", "", "int", IndexOpts()});
        db->AddIndex("UsersView", {"reputation", "", "int", IndexOpts()});
        db->AddIndex("UsersView", {"name+about", {"name","about"}, "text", "composite", IndexOpts().SetCollateMode(CollateUTF8) });
        db->AddIndex("UsersView", {"name_text", {"name"}, "text", "composite", IndexOpts().SetCollateMode(CollateUTF8) });
        db->Commit("UsersView");
    }

    // RI UsersHistory
    if (table == "Users" || table == "ALL") {
        db->OpenNamespace("Users", StorageOpts().Enabled().CreateIfMissing());
        db->AddIndex("Users", {"txid", "hash", "string", IndexOpts().PK()});
        db->AddIndex("Users", {"block", "tree", "int", IndexOpts()});
        db->AddIndex("Users", {"time", "tree", "int64", IndexOpts()});
        db->AddIndex("Users", {"address", "hash", "string", IndexOpts()});
        db->AddIndex("Users", {"name", "", "string", IndexOpts().SetCollateMode(CollateUTF8)});
        db->AddIndex("Users", {"birthday", "", "int", IndexOpts()});
        db->AddIndex("Users", {"gender", "", "int", IndexOpts()});
        db->AddIndex("Users", {"regdate", "", "int64", IndexOpts()});
        db->AddIndex("Users", {"avatar", "", "string", IndexOpts()});
        db->AddIndex("Users", {"about", "", "string", IndexOpts().SetCollateMode(CollateUTF8)});
        db->AddIndex("Users", {"lang", "", "string", IndexOpts().SetCollateMode(CollateUTF8)});
        db->AddIndex("Users", {"url", "", "string", IndexOpts().SetCollateMode(CollateUTF8)});
        db->AddIndex("Users", {"pubkey", "", "string", IndexOpts()});
        db->AddIndex("Users", {"donations", "", "string", IndexOpts()});
        db->AddIndex("Users", {"referrer", "", "string", IndexOpts()});
        db->AddIndex("Users", {"id", "", "int", IndexOpts()});
        db->Commit("Users");
    }

    // UserRatings
    if (table == "UserRatings" || table == "ALL") {
        db->OpenNamespace("UserRatings", StorageOpts().Enabled().CreateIfMissing());
        db->AddIndex("UserRatings", {"block", "tree", "int", IndexOpts()});
        db->AddIndex("UserRatings", {"address", "hash", "string", IndexOpts()});
        db->AddIndex("UserRatings", {"scoreSum", "", "int", IndexOpts()});
        db->AddIndex("UserRatings", {"scoreCnt", "", "int", IndexOpts()});
        db->AddIndex("UserRatings", {"address+block", {"address", "block"}, "hash", "composite", IndexOpts().PK()});
        db->Commit("UserRatings");
    }

    // Tags
    if (table == "Tags" || table == "ALL") {
        db->OpenNamespace("Tags", StorageOpts().Enabled().CreateIfMissing());
        db->AddIndex("Tags", {"tag", "text", "string", IndexOpts().SetCollateMode(CollateUTF8).PK()});
        db->AddIndex("Tags", {"rating", "tree", "int", IndexOpts()});
        db->Commit("Tags");
    }

    // Posts
    if (table == "Posts" || table == "ALL") {
        db->OpenNamespace("Posts", StorageOpts().Enabled().CreateIfMissing());
        db->AddIndex("Posts", {"txid", "hash", "string", IndexOpts().PK()});
        db->AddIndex("Posts", {"txidEdit", "hash", "string", IndexOpts()});
        db->AddIndex("Posts", {"block", "tree", "int", IndexOpts()});
        db->AddIndex("Posts", {"time", "tree", "int64", IndexOpts()});
        db->AddIndex("Posts", {"address", "hash", "string", IndexOpts()});
        // Types:
        // 0 - simple post
        // 1 - video post
        // 2 - image post
        db->AddIndex("Posts", {"type", "tree", "int", IndexOpts()});
        db->AddIndex("Posts", {"lang", "hash", "string", IndexOpts()});
        db->AddIndex("Posts", {"caption", "", "string", IndexOpts().SetCollateMode(CollateUTF8)});
        db->AddIndex("Posts", {"caption_", "text", "string", IndexOpts().SetCollateMode(CollateUTF8)});
        db->AddIndex("Posts", {"message", "", "string", IndexOpts().SetCollateMode(CollateUTF8)});
        db->AddIndex("Posts", {"message_", "text", "string", IndexOpts().SetCollateMode(CollateUTF8)});
        db->AddIndex("Posts", {"tags", "", "string", IndexOpts().Array().SetCollateMode(CollateUTF8)});
        db->AddIndex("Posts", {"url", "text", "string", IndexOpts().SetCollateMode(CollateUTF8)});
        db->AddIndex("Posts", {"images", "", "string", IndexOpts().Array().SetCollateMode(CollateUTF8)});
        db->AddIndex("Posts", {"settings", "text", "string", IndexOpts().SetCollateMode(CollateUTF8)});
        db->AddIndex("Posts", {"scoreSum", "", "int", IndexOpts()});
        db->AddIndex("Posts", {"scoreCnt", "", "int", IndexOpts()});
        db->AddIndex("Posts", {"reputation", "", "int", IndexOpts()});
        db->AddIndex("Posts", {"caption+message", {"caption_", "message_"}, "text", "composite", IndexOpts().SetCollateMode(CollateUTF8)});
        db->Commit("Posts");
    }

    // Posts Hitstory
    if (table == "PostsHistory" || table == "ALL") {
        db->OpenNamespace("PostsHistory", StorageOpts().Enabled().CreateIfMissing());
        db->AddIndex("PostsHistory", {"txid", "hash", "string", IndexOpts()});
        db->AddIndex("PostsHistory", {"txidEdit", "hash", "string", IndexOpts()});
        db->AddIndex("PostsHistory", {"block", "tree", "int", IndexOpts()});
        db->AddIndex("PostsHistory", {"time", "", "int64", IndexOpts()});
        db->AddIndex("PostsHistory", {"address", "", "string", IndexOpts()});
        db->AddIndex("PostsHistory", {"type", "tree", "int", IndexOpts()});
        db->AddIndex("PostsHistory", {"lang", "", "string", IndexOpts()});
        db->AddIndex("PostsHistory", {"caption", "", "string", IndexOpts().SetCollateMode(CollateUTF8)});
        db->AddIndex("PostsHistory", {"message", "", "string", IndexOpts().SetCollateMode(CollateUTF8)});
        db->AddIndex("PostsHistory", {"tags", "", "string", IndexOpts().Array().SetCollateMode(CollateUTF8)});
        db->AddIndex("PostsHistory", {"url", "", "string", IndexOpts().SetCollateMode(CollateUTF8)});
        db->AddIndex("PostsHistory", {"images", "", "string", IndexOpts().Array().SetCollateMode(CollateUTF8)});
        db->AddIndex("PostsHistory", {"settings", "", "string", IndexOpts().SetCollateMode(CollateUTF8)});
        db->AddIndex("PostsHistory", {"txid+block", {"txid", "block"}, "hash", "composite", IndexOpts().PK()});
        db->Commit("PostsHistory");
    }

    // PostRatings
    if (table == "PostRatings" || table == "ALL") {
        db->OpenNamespace("PostRatings", StorageOpts().Enabled().CreateIfMissing());
        db->AddIndex("PostRatings", {"block", "tree", "int", IndexOpts()});
        db->AddIndex("PostRatings", {"posttxid", "hash", "string", IndexOpts()});
        db->AddIndex("PostRatings", {"scoreSum", "", "int", IndexOpts()});
        db->AddIndex("PostRatings", {"scoreCnt", "", "int", IndexOpts()});
        db->AddIndex("PostRatings", {"reputation", "", "int", IndexOpts()});
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
        db->AddIndex("Scores", {"value", "tree", "int", IndexOpts()});
        db->AddIndex("Scores", {"address+posttxid", {"address", "posttxid"}, "hash", "composite", IndexOpts()});
        db->Commit("Scores");
    }

    // Subscribes
    if (table == "SubscribesView" || table == "ALL") {
        db->OpenNamespace("SubscribesView", StorageOpts().Enabled().CreateIfMissing());
        db->AddIndex("SubscribesView", {"txid", "hash", "string", IndexOpts()});
        db->AddIndex("SubscribesView", {"block", "tree", "int", IndexOpts()});
        db->AddIndex("SubscribesView", {"time", "", "int64", IndexOpts()});
        db->AddIndex("SubscribesView", {"address", "hash", "string", IndexOpts()});
        db->AddIndex("SubscribesView", {"address_to", "hash", "string", IndexOpts()});
        db->AddIndex("SubscribesView", {"private", "", "bool", IndexOpts()});
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
        db->AddIndex("Subscribes", {"private", "", "bool", IndexOpts()});
        db->AddIndex("Subscribes", {"unsubscribe", "", "bool", IndexOpts()});
        db->AddIndex("Subscribes", {"address+address_to", {"address", "address_to"}, "hash", "composite", IndexOpts()});
        db->Commit("Subscribes");
    }

	// Blocking
    if (table == "BlockingView" || table == "ALL") {
        db->OpenNamespace("BlockingView", StorageOpts().Enabled().CreateIfMissing());
        db->AddIndex("BlockingView", {"txid", "hash", "string", IndexOpts()});
        db->AddIndex("BlockingView", {"block", "tree", "int", IndexOpts()});
        db->AddIndex("BlockingView", {"time", "", "int64", IndexOpts()});
        db->AddIndex("BlockingView", {"address", "hash", "string", IndexOpts()});
        db->AddIndex("BlockingView", {"address_to", "hash", "string", IndexOpts()});
        db->AddIndex("BlockingView", {"address_reputation", "", "int", IndexOpts()});
        db->AddIndex("BlockingView", {"address+address_to", {"address", "address_to"}, "hash", "composite", IndexOpts().PK()});
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
        db->AddIndex("Blocking", {"unblocking", "", "bool", IndexOpts()});
        db->AddIndex("Blocking", {"address+address_to", {"address", "address_to"}, "hash", "composite", IndexOpts()});
        db->Commit("Blocking");
    }

    // Reposts
    // if (table == "Reposts" || table == "ALL") {
    //     db->OpenNamespace("Reposts", StorageOpts().Enabled().CreateIfMissing());
    //     db->AddIndex("Reposts", {"txid", "hash", "string", IndexOpts().PK()});
    //     db->AddIndex("Reposts", {"block", "tree", "int", IndexOpts()});
    //     db->AddIndex("Reposts", {"address", "hash", "string", IndexOpts()});
    //     db->AddIndex("Reposts", {"posttxid", "hash", "string", IndexOpts()});
    //     db->AddIndex("Reposts", {"address+posttxid", {"address", "posttxid"}, "hash", "composite", IndexOpts()});
    //     db->Commit("Reposts");
    // }

    // Complains
    if (table == "Complains" || table == "ALL") {
        db->OpenNamespace("Complains", StorageOpts().Enabled().CreateIfMissing());
        db->AddIndex("Complains", {"txid", "hash", "string", IndexOpts().PK()});
        db->AddIndex("Complains", {"block", "tree", "int", IndexOpts()});
        db->AddIndex("Complains", {"time", "tree", "int64", IndexOpts()});
        db->AddIndex("Complains", {"posttxid", "hash", "string", IndexOpts()});
        db->AddIndex("Complains", {"address", "hash", "string", IndexOpts()});
        db->AddIndex("Complains", {"reason", "tree", "int", IndexOpts()});
        db->AddIndex("Complains", {"address+posttxid", {"address", "posttxid"}, "hash", "composite", IndexOpts()});
        db->Commit("Complains");
    }

    // UTXO
    if (table == "UTXO" || table == "ALL") {
        db->OpenNamespace("UTXO", StorageOpts().Enabled().CreateIfMissing());
        db->AddIndex("UTXO", {"txid", "", "string", IndexOpts()});
        db->AddIndex("UTXO", {"txout", "", "int", IndexOpts()});
        db->AddIndex("UTXO", {"time", "tree", "int64", IndexOpts()});
        db->AddIndex("UTXO", {"block", "tree", "int", IndexOpts()});
        db->AddIndex("UTXO", {"address", "hash", "string", IndexOpts()});
        db->AddIndex("UTXO", {"amount", "", "int64", IndexOpts()});
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

    // Comments
    if (table == "Comments" || table == "ALL") {
        db->OpenNamespace("Comments", StorageOpts().Enabled().CreateIfMissing());
        db->AddIndex("Comments", {"id", "tree", "string", IndexOpts().PK()});
        db->AddIndex("Comments", {"postid", "tree", "string", IndexOpts()});
        db->AddIndex("Comments", {"address", "tree", "string", IndexOpts()});
        db->AddIndex("Comments", {"pubkey", "", "string", IndexOpts()});
        db->AddIndex("Comments", {"signature", "", "string", IndexOpts()});
        db->AddIndex("Comments", {"time", "tree", "int64", IndexOpts()});
        db->AddIndex("Comments", {"block", "tree", "int", IndexOpts()});
        db->AddIndex("Comments", {"msg", "", "string", IndexOpts().SetCollateMode(CollateUTF8)});
        db->AddIndex("Comments", {"parentid", "tree", "string", IndexOpts()});
        db->AddIndex("Comments", {"answerid", "tree", "string", IndexOpts()});
        db->AddIndex("Comments", {"timeupd", "tree", "int64", IndexOpts()});
        db->Commit("Comments");
    }

    return true;
}

bool PocketDB::DropTable(std::string table)
{
    Error err = db->DropNamespace(table).ok();
    if (!InitDB(table)) return false;
    return true;
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
        err = db->Select(reindexer::Query(table).ReqTotal(), _res);
        if (err.ok()) {
            obj.pushKV("total", (uint64_t)_res.TotalCount());

            UniValue t_arr(UniValue::VARR);
            for (auto& it : _res) {
                t_arr.push_back(it.GetItem().GetJSON().ToString());
            }
            obj.pushKV("items", t_arr);
        }

        return err.ok();
    }
    //--------------------------
    bool ret = true;
    std::vector<NamespaceDef> nss;
    db->EnumNamespaces(nss, false);

    struct NamespaceDefSortComp
    {
        inline bool operator() (const NamespaceDef& ns1, const NamespaceDef& ns2)
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
    // TODO (brangr): Its not funny! :D
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
    QueryResults res;
    Error err = db->Delete(query, res);

    if (err.ok()) {
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
    Error err = SelectOne(Query("Users").Where("address", CondEq, address).Sort("time", true), _user_itm);
    if (err.code() == 13) return DeleteWithCommit(Query("UsersView").Where("address", CondEq, address));
    if (err.ok()) {
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
        _view_itm["referrer"] = _user_itm["referrer"].As<string>();
        _view_itm["id"] = _user_itm["id"].As<int>();

        int sum = 0;
        int cnt = 0;
        GetUserRating(address, sum, cnt, height);
        _view_itm["scoreSum"] = sum;
        _view_itm["scoreCnt"] = cnt;
        _view_itm["reputation"] = sum - (cnt * 3);

        return UpsertWithCommit("UsersView", _view_itm);
    }

    return err;
}

Error PocketDB::UpdateSubscribesView(std::string address, std::string address_to)
{
    QueryResults _res;
    Error err = db->Select(Query("Subscribes", 0, 1).Where("address", CondEq, address).Where("address_to", CondEq, address_to).Sort("time", true), _res);
    if (_res.Count() <= 0) return DeleteWithCommit(Query("SubscribesView").Where("address", CondEq, address).Where("address_to", CondEq, address_to));
    if (err.ok()) {
        Item _itm = _res[0].GetItem();
        if (_itm["unsubscribe"].As<bool>() == true)
            return DeleteWithCommit(Query("SubscribesView").Where("address", CondEq, address).Where("address_to", CondEq, address_to));
        else
            return UpsertWithCommit("SubscribesView", _itm);
    }
    //-----------------
    return err;
}

Error PocketDB::UpdateBlockingView(std::string address, std::string address_to)
{
    Item _blocking_itm;
    Error err = SelectOne(Query("Blocking").Where("address", CondEq, address).Where("address_to", CondEq, address_to).Sort("time", true), _blocking_itm);
    if (err.code() == 13) return DeleteWithCommit(Query("BlockingView").Where("address", CondEq, address).Where("address_to", CondEq, address_to));
    if (err.ok()) {
        if (_blocking_itm["unblocking"].As<bool>() == true) {
            return DeleteWithCommit(Query("BlockingView").Where("address", CondEq, address).Where("address_to", CondEq, address_to));
        }
        else {
            Item _blocking_view_itm = db->NewItem("BlockingView");
            _blocking_view_itm["txid"] = _blocking_itm["txid"].As<string>();
            _blocking_view_itm["block"] = _blocking_itm["block"].As<int>();
            _blocking_view_itm["time"] = _blocking_itm["time"].As<int64_t>();
            _blocking_view_itm["address"] = _blocking_itm["address"].As<string>();
            _blocking_view_itm["address_to"] = _blocking_itm["address_to"].As<string>();

            int sum = 0;
            int cnt = 0;
            GetUserRating(address, sum, cnt, _blocking_itm["block"].As<int>());
            _blocking_view_itm["address_reputation"] = sum - (cnt * 3);
                
            return UpsertWithCommit("BlockingView", _blocking_view_itm);
        }
    }

    return err;
}

Error PocketDB::CommitPostItem(Item& itm) {
    // Move exists Post to history table
    if (itm["txidEdit"].As<string>() != "") {
        Item cur_post_item;
        if (SelectOne(Query("Posts").Where("txid", CondEq, itm["txid"].As<string>()), cur_post_item).ok()) {
            Item hist_post_item = db->NewItem("PostsHistory");
            hist_post_item["txid"] = cur_post_item["txid"].As<string>();
            hist_post_item["txidEdit"] = cur_post_item["txidEdit"].As<string>();
            hist_post_item["block"] = cur_post_item["block"].As<int>();
            hist_post_item["time"] = cur_post_item["time"].As<int64_t>();
            hist_post_item["address"] = cur_post_item["address"].As<string>();
            hist_post_item["type"] = cur_post_item["type"].As<int>();
            hist_post_item["lang"] = cur_post_item["lang"].As<string>();
            hist_post_item["caption"] = cur_post_item["caption"].As<string>();
            hist_post_item["message"] = cur_post_item["message"].As<string>();
            hist_post_item["tags"] = cur_post_item["tags"];
            hist_post_item["url"] = cur_post_item["url"].As<string>();
            hist_post_item["images"] = cur_post_item["images"];
            hist_post_item["settings"] = cur_post_item["settings"].As<string>();
            
            Error err = UpsertWithCommit("PostsHistory", hist_post_item);
            if (!err.ok()) return err;

            err = DeleteWithCommit(Query("Posts").Where("txid", CondEq, itm["txid"].As<string>()));
            if (!err.ok()) return err;
        }
    }

    // Insert new Post
    return UpsertWithCommit("Posts", itm);
}

Error PocketDB::RestorePostItem(std::string posttxid, int height) {
    // Move exists Post to history table
    Item hist_post_item;
    Error err = SelectOne(Query("PostsHistory").Where("txid", CondEq, posttxid).Sort("block", true), hist_post_item);
    if (err.ok()) {
        std::string posttxid_edit = hist_post_item["txidEdit"].As<string>();

        Item post_item = db->NewItem("Posts");
        post_item["txid"] = hist_post_item["txid"].As<string>();
        post_item["txidEdit"] = posttxid_edit;
        post_item["block"] = hist_post_item["block"].As<int>();
        post_item["time"] = hist_post_item["time"].As<int64_t>();
        post_item["address"] = hist_post_item["address"].As<string>();
        post_item["type"] = hist_post_item["type"].As<int>();
        post_item["lang"] = hist_post_item["lang"].As<string>();
        post_item["caption"] = hist_post_item["caption"].As<string>();
        post_item["message"] = hist_post_item["message"].As<string>();
        post_item["tags"] = hist_post_item["tags"];
        post_item["url"] = hist_post_item["url"].As<string>();
        post_item["images"] = hist_post_item["images"];
        post_item["settings"] = hist_post_item["settings"].As<string>();

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

void PocketDB::GetUserRating(std::string address, int& sum, int& cnt, int height) {
    // Set to default if rating for user not found
    sum = 0;
    cnt = 0;

    // Sorting by block desc - last accumulating rating
    Item _itm_rating;
    if (SelectOne(
            Query("UserRatings")
            .Where("address", CondEq, address)
            .Where("block", CondLe, height)
            .Sort("block", true)
            , _itm_rating
        ).ok()
    ) {
        sum = _itm_rating["scoreSum"].As<int>();
        cnt = _itm_rating["scoreCnt"].As<int>();
    }
}

int PocketDB::GetUserReputation(std::string _address, int height)
{
    int sum = 0;
    int cnt = 0;
    GetUserRating(_address, sum, cnt, height);

    return sum - (cnt * 3);
}

int64_t PocketDB::GetUserBalance(std::string _address, int height)
{
    AggregationResult aggRes;
    if (SelectAggr(
        Query("UTXO")
        .Where("address", CondEq, _address)
        .Where("block", CondLt, height)
        .Where("spent_block", CondEq, 0)
        .Aggregate("amount", AggSum)
        ,"amount"
        ,aggRes
    ).ok()) {
        return (int64_t)aggRes.value;
    } else {
        return 0;
    }
}

bool PocketDB::UpdateUserRating(std::string address, int sum, int cnt)
{
    reindexer::QueryResults userViewRes;
    if (!db->Select(reindexer::Query("UsersView", 0, 1).Where("address", CondEq, address), userViewRes).ok()) return false;
    for (auto& r : userViewRes) {
        reindexer::Item userViewItm(r.GetItem());
        userViewItm["scoreSum"] = sum;
        userViewItm["scoreCnt"] = cnt;
        userViewItm["reputation"] = sum - (cnt * 3);
        if (!UpsertWithCommit("UsersView", userViewItm).ok()) return false;
    }

    return true;
}

bool PocketDB::UpdateUserRating(std::string address, int height)
{
    int sum = 0;
    int cnt = 0;
    GetUserRating(address, sum, cnt, height);
    return UpdateUserRating(address, sum, cnt);
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
            .Sort("block", true)
            , _itm_rating_cur
        ).ok()
    ) {
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

bool PocketDB::GetHashItem(Item& item, std::string table, bool with_referrer, std::string& out_hash)
{
    std::string data = "";
    //------------------------
    if (table == "Posts") {
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

        data += item["txidEdit"].As<string>();
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
        if (with_referrer) data += item["referrer"].As<string>();
        data += item["pubkey"].As<string>();
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