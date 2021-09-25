// Copyright (c) 2021 The Pocketcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <rpc/cache.h>
#include <rpc/server.h>

static const unsigned int MAX_CACHE_SIZE_MB = 64;

RPCCache::RPCCache() 
{
    m_blockHeight = chainActive.Height();
    m_maxCacheSize = gArgs.GetArg("-rpccachesize", MAX_CACHE_SIZE_MB) * 1024 * 1024;
}

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
    m_cacheSize = 0;
    m_blockHeight = chainActive.Height();

    LogPrint(BCLog::RPC, "RPC cache cleared.\n");
}

void RPCCache::Put(const std::string& path, const UniValue& content)
{
    if (chainActive.Height() > m_blockHeight)
        this->Clear();

    int size = path.size() + content.write().size();

    if (m_maxCacheSize < size + m_cacheSize) {
        LogPrint(BCLog::RPC, "RPC cache over size limit: current = %d, max = %d\n", size + m_cacheSize, m_maxCacheSize);
        return;
    }

    LOCK(CacheMutex);
    if (m_cache.find(path) == m_cache.end()) {
        LogPrint(BCLog::RPC, "RPC cache put '%s', size %d\n", path, size);
        m_cacheSize += size;
        m_cache.emplace(path, content);
    } else {
        LogPrint(BCLog::RPC, "RPC cache put update '%s'\n", path);
        // Adjust cache size, remove old element size, add new element size
        m_cacheSize -= m_cache[path].size();
        m_cacheSize += content.write().size();
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

    // Return empty UniValue if nothing found in cache.
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
    LogPrint(BCLog::RPC, "RPC cache Statistic num elements = %d, size in bytes = %d", m_cache.size(), m_cacheSize);

    return { (int64_t) m_cache.size(), (int64_t) m_cacheSize };
}