// Copyright (c) 2021 The Pocketcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef POCKETCOIN_RPC_CACHE_H
#define POCKETCOIN_RPC_CACHE_H
#include <univalue.h>
#include <sync.h>
#include <logging.h>
#include <validation.h>

class JSONRPCRequest;

class RPCCacheInfoGroup
{
public:
    RPCCacheInfoGroup(int lifeTime, std::set<std::string> methods);
    int lifeTime;
    std::set<std::string> methods;
};

class RPCCacheInfoGenerator
{
public:
    explicit RPCCacheInfoGenerator(std::vector<RPCCacheInfoGroup> groups);
    std::map<std::string, int> Generate() const;
private:
    std::vector<RPCCacheInfoGroup> m_groups;
};

class RPCCacheEntry
{
public:
    RPCCacheEntry(UniValue data, int validUntill);
    const UniValue& GetData() const;
    const int& GetValidUntill() const;
private:
    int m_validUntill;
    UniValue m_data;
};

class RPCCache
{
private:
    Mutex CacheMutex;
    std::map<std::string, RPCCacheEntry> m_cache;
    int m_cacheSize;
    int m_maxCacheSize;
    // <methodName, lifeTime>
    std::map<std::string, int> m_supportedMethods = {
        { "getlastcomments", 1 },
        { "getcomments", 1 },
        { "getuseraddress", 60 },
        { "gettags", 60 },
        { "getnodeinfo", 1 },
        { "getrawtransactionwithmessagebyid", 1 },
        { "getrawtransactionwithmessage", 1 },
        { "getrawtransaction", 1 },
        { "getusercontents", 10 },
        { "gethierarchicalstrip", 1 },
        { "getboostfeed", 1 },
        { "getprofilefeed", 1 },
        { "getsubscribesfeed", 1 },
        { "gethistoricalstrip", 1 },
        { "gethotposts", 1 },
        { "getuserprofile", 1 },
        { "getuserstate", 1 },
        { "getpagescores", 1 },
        { "getcontent", 60 },
        { "getcontents", 60 },
        { "getmissedinfo", 1 },
        { "getcontentsstatistic", 10 },
        { "search", 60 },
        { "searchlinks", 1 },
        { "searchusers", 1 },
        { "getaccountsetting", 1 },
        { "getstatisticbyhours", 10 },
        { "getstatisticbydays", 100 },
        { "getstatisticcontentbyhours", 10 },
        { "getstatisticcontentbydays", 100 },
        { "getcontentactions", 60 },
        { "gettopfeed", 60 },
        { "getmostcommentedfeed", 1 },
        { "gettopaccounts", 60 },
        { "getrecommendedcontentbyaddress", 60 },
        { "getrecommendedaccountbyaddress", 60 },

    };
    
    /* Make a key for the unordered hash map by concatenating together the methodname and
     * params.  TODO: We will likely need to improve this methodology in the future in
     * case parameters are delivered in out of order by the front-end clients.
     */
    std::string MakeHashKey(const JSONRPCRequest& req);
    void ClearOverdue(int height);

public:
    RPCCache();

    void Clear();

    void Put(const std::string& path, const UniValue& content, const int& lifeTime);

    UniValue Get(const std::string& path);

    UniValue GetRpcCache(const JSONRPCRequest& req);

    void PutRpcCache(const JSONRPCRequest& req, const UniValue& content);

    std::tuple<int64_t, int64_t> Statistic();

};

#endif // POCKETCOIN_RPC_CACHE_H
