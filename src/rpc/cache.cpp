// Copyright (c) 2021 The Pocketcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <rpc/cache.h>
#include <rpc/server.h>

std::string RPCCache::MakeHashKey(const JSONRPCRequest& req)
{
    std::string hashKey = req.strMethod;

    hashKey += req.params.write();

    return hashKey;
}

void RPCCache::Clear()
{
    LOCK(CacheMutex);
    m_cache.clear();
    m_blockHeight = chainActive.Height();

    LogPrint(BCLog::RPC, "RPC cache cleared.\n");
}

void RPCCache::Put(const std::string& path, const UniValue& content)
{
    if (chainActive.Height() > m_blockHeight)
        this->Clear();

    LOCK(CacheMutex);
    if (m_cache.find(path) == m_cache.end()) {
        LogPrint(BCLog::RPC, "RPC cache put '%s'\n", path);
        m_cache.emplace(path, content);
    } else {
        LogPrint(BCLog::RPC, "RPC cache put update '%s'\n", path);
        m_cache[path] = content;
    }
}

UniValue RPCCache::Get(const std::string& path)
{
    if (m_cache.empty()) {
        return UniValue();
    }

    if (chainActive.Height() > m_blockHeight) {
        this->Clear();
        return UniValue();
    }

    LOCK(CacheMutex);
    if (m_cache.find(path) != m_cache.end()) {
        LogPrint(BCLog::RPC, "RPC Cache get found %s in cache\n", path);
        return m_cache.at(path);
    }

    /* Return empty UniValue if nothing found in cache. */
    return UniValue();
}

UniValue RPCCache::GetRpcCache(const JSONRPCRequest& req)
{
    // Return empty UniValue if method not supported for caching.
    if (std::find(supportedMethods.begin(), supportedMethods.end(), req.strMethod) == supportedMethods.end())
        return UniValue();

    return Get(MakeHashKey(req));
}

void RPCCache::PutRpcCache(const JSONRPCRequest& req, const UniValue& content)
{
    if (std::find(supportedMethods.begin(), supportedMethods.end(), req.strMethod) == supportedMethods.end())
        return;

    Put(MakeHashKey(req), content);
}

std::tuple<int64_t, int64_t> RPCCache::Statistic()
{
    LOCK(CacheMutex);

    int64_t size;
    for (auto& it : m_cache)
        size += it.first.size() + it.second.size();
        
    return { m_cache.size(), size * sizeof(std::string::value_type) };
}