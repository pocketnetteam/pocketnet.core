// Copyright (c) 2021 The Pocketcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <rpc/cache.h>
#include <rpc/server.h>

static const unsigned int MAX_CACHE_SIZE_MB = 64;

RPCCacheInfoGroup::RPCCacheInfoGroup(int lifeTime, std::set<std::string> methods)
    : lifeTime(std::move(lifeTime)),
      methods(std::move(methods))
{}


RPCCacheInfoGenerator::RPCCacheInfoGenerator(std::vector<RPCCacheInfoGroup> groups)
    : m_groups(std::move(groups))
{}
std::map<std::string, int> RPCCacheInfoGenerator::Generate() const
{
    std::map<std::string, int> result;
    std::set<int> lifeTimes;
    for (const auto& group: m_groups) {
        // Do not allow different groups with same lifetime
        assert(lifeTimes.emplace(group.lifeTime).second);
        for(const auto& method: group.methods) {
            // Do not allow same method in different groups
            assert(result.emplace(method, group.lifeTime).second);
        }
    }

    return result;
}

RPCCacheEntry::RPCCacheEntry(UniValue data, int validUntill)
    : m_data(std::move(data)),
      m_validUntill(std::move(validUntill)),
      m_size(m_data.write().size())
{
}
const UniValue& RPCCacheEntry::GetData() const
{
    return m_data;
}
const int& RPCCacheEntry::GetValidUntill() const
{
    return m_validUntill;
}
const size_t& RPCCacheEntry::Size() const
{
    return m_size;
}

RPCCache::RPCCache() 
{
    m_maxCacheSize = gArgs.GetArg("-rpccachesize", MAX_CACHE_SIZE_MB) * 1024 * 1024;
    m_cacheSize = 0;
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

    LogPrint(BCLog::RPC, "RPC cache cleared.\n");
}

void RPCCache::ClearOverdue(int height)
{
    for (auto itr = m_cache.begin(); itr != m_cache.end();) {
        if(itr->second.GetValidUntill() <= height) {
            m_cacheSize -= (itr->first.size() + itr->second.Size()); // Decreasing cache size 
            itr = m_cache.erase(itr);
        } else {
            itr++;
        }
    }
}

void RPCCache::Put(const std::string& path, const UniValue& content, const int& lifeTime)
{
    auto currentHeight = ChainActiveSafeHeight();

    auto validUntill = currentHeight + lifeTime;

    LOCK(CacheMutex);

    int size = path.size() + content.write().size();

    ClearOverdue(currentHeight);

    if (m_maxCacheSize < size + m_cacheSize) {
        LogPrint(BCLog::RPC, "RPC cache over size limit: current = %d, max = %d\n", m_cacheSize, m_maxCacheSize);
        return;
    }

    if (auto entry = m_cache.find(path); entry != m_cache.end()) {
        LogPrint(BCLog::RPC, "RPC cache put update '%s'\n", path);
        // Adjust cache size, remove old element size, add new element size
        m_cacheSize -= entry->second.Size();
        m_cacheSize += content.write().size();
    } else {
        LogPrint(BCLog::RPC, "RPC cache put '%s', size %d\n", path, size);
        m_cacheSize += size;
    }

    m_cache.insert_or_assign(path, RPCCacheEntry(content, validUntill));
}

UniValue RPCCache::Get(const std::string& path)
{
    LOCK(CacheMutex);

    ClearOverdue(ChainActiveSafeHeight());

    if (auto entry = m_cache.find(path); entry != m_cache.end()) {
        LogPrint(BCLog::RPC, "RPC Cache get found %s in cache\n", path);
        return entry->second.GetData();
    }

    // Return empty UniValue if nothing found in cache.
    return UniValue();
}

UniValue RPCCache::GetRpcCache(const JSONRPCRequest& req)
{
    // Return empty UniValue if method not supported for caching.
    if (m_supportedMethods.find(req.strMethod) == m_supportedMethods.end())
        return UniValue();

    return Get(MakeHashKey(req));
}

void RPCCache::PutRpcCache(const JSONRPCRequest& req, const UniValue& content)
{
    if (auto group = m_supportedMethods.find(req.strMethod); group != m_supportedMethods.end()) {
        Put(MakeHashKey(req), content, group->second);
    }
}

std::tuple<int64_t, int64_t> RPCCache::Statistic()
{
    LOCK(CacheMutex);
    // Return number of elements in cache and size of cache in bytes
    return { (int64_t) m_cache.size(), (int64_t) m_cacheSize };
}