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

class RPCCache
{
private:
    Mutex CacheMutex;
    std::map<std::string, UniValue> m_cache;
    int m_blockHeight;
    int m_cacheSize;
    int m_maxCacheSize;
    const std::vector<std::string> supportedMethods = { "getlastcomments",
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
                                                        "gethierarchicalfeed",
                                                        "gethierarchicalstrip",
                                                        "gethotposts",
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
                                                        "getaddressscores",
                                                        "getpostscores",
                                                        "getstatisticbyhours",
                                                        "getstatisticbydays",
                                                        "getstatisticcontentbyhours",
                                                        "getstatisticcontentbydays",
                                                        "getrecomendedaccountsbytags",
                                                        "getrecomendedcontentsbyscoresonsimilarcontents",
                                                        "getaddressinfo",
                                                        "getcompactblock",
                                                        "getlastblocks",
                                                        "searchbyhash",
                                                        "gettransactions",
                                                        "getaddresstransactions",
                                                        "getblocktransactions",
                                                        "getbalancehistory",
                                                        "getcoininfo",
                                                        "estimatesmartfee" };

    /* Make a key for the unordered hash map by concatenating together the methodname and
     * params.  TODO: We will likely need to improve this methodology in the future in
     * case parameters are delivered in out of order by the front-end clients.
     */
    std::string MakeHashKey(const JSONRPCRequest& req);

public:
    RPCCache();

    void Clear();

    void Put(const std::string& path, const UniValue& content);

    UniValue Get(const std::string& path);

    UniValue GetRpcCache(const JSONRPCRequest& req);

    void PutRpcCache(const JSONRPCRequest& req, const UniValue& content);

    std::tuple<int64_t, int64_t> Statistic();

};

#endif // POCKETCOIN_RPC_CACHE_H
