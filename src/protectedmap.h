#ifndef POCKETCOIN_PROTECTEDMAP_H
#define POCKETCOIN_PROTECTEDMAP_H

#include <mutex>
#include <map>
#include <utility>
#include <functional>

template<class Key, class Value>
class ProtectedMap
{
public:
    bool insert_or_assign(const Key& key, const Value& value)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_map.insert_or_assign(key, value).second;
    }

    bool insert_or_assign(const Key& key, Value&& value)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_map.insert_or_assign(key, std::forward<Value>(value)).second;
    }

    bool insert(const Key& key, Value&& value)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_map.insert({key, std::forward<Value>(value)}).second;
    }
    
    bool insert(const Key& key, const Value& value)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_map.insert({key, value}).second;
    }

    void erase(const Key& key)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_map.erase(key);
    }

    bool empty()
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_map.empty();
    }

    void Iterate(const std::function<void(std::pair<const Key, Value>&)>& func)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        for (auto& elem : m_map) {
            func(elem);
        }
    }

    bool has(const Key& key)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_map.find(key) != m_map.end();
    }

    bool exec_for_elem(const Key& key, const std::function<void(const Value& value)>& func)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto itr = m_map.find(key);
        if (itr == m_map.end()) {
            return false;
        }

        func(itr->second);
        return true;
    }

protected:
    std::map<Key, Value> m_map;
    std::mutex m_mutex;
};
#endif // POCKETCOIN_PROTECTEDMAP_H