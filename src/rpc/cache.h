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
    std::map<std::string, int> m_supportedMethods = RPCCacheInfoGenerator(
                                                { 
                                                    { 1,
                                                        {
                                                            "getlastcomments",
                                                            "getcomments",
                                                            "getuseraddress",
                                                            "getaddressregistration",
                                                            "search",
                                                            "searchlinks",
                                                            "searchusers",
                                                            "gettags",
                                                            "getrawtransactionwithmessagebyid",
                                                            "getrawtransactionwithmessage",
                                                            "getrawtransaction",
                                                            "getusercontents",
                                                            "gethistoricalfeed",
                                                            "gethistoricalstrip",
                                                            "gethierarchicalfeed"
                                                        }
                                                    },
                                                    { 2,
                                                        {
                                                            "gethierarchicalstrip",
                                                            "gethotposts",
                                                            "gettopfeed",
                                                            "getuserprofile",
                                                            "getprofilefeed",
                                                            "getsubscribesfeed",
                                                            "txunspent",
                                                            "getaddressid",
                                                            "getuserstate",
                                                            "getpagescores",
                                                            "getcontent",
                                                            "getcontents",
                                                            "getaccountsetting",
                                                            "getcontentsstatistic",
                                                            "getusersubscribes",
                                                            "getusersubscribers",
                                                            "getuserblockings",
                                                            "gettopaccounts",
                                                            "getaddressscores",
                                                            "getpostscores",
                                                            "getstatisticbyhours",
                                                            "getstatisticbydays",
                                                            "getstatisticcontentbyhours"
                                                        }
                                                    },
                                                    { 3,
                                                        {
                                                            "getstatisticcontentbydays",
                                                            "getrecommendedcontentbyaddress",
                                                            "getrecommendedaccountbyaddress",
                                                            "getaddressinfo",
                                                            "getcompactblock",
                                                            "getlastblocks",
                                                            "searchbyhash",
                                                            "gettransactions",
                                                            "getaddresstransactions",
                                                            "getblocktransactions",
                                                            "getbalancehistory",
                                                            "getcoininfo",
                                                            "estimatesmartfee"
                                                        }
                                                    }
                                                }).Generate();

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
