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
    auto insert_or_assign(const Key& key, const Value& value)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_map.insert_or_assign(key, value);
    }

    auto insert_or_assign(const Key& key, Value&& value)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_map.insert_or_assign(key, std::forward<Value>(value));
    }

    auto erase(const Key& key)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_map.erase(key);
    }

    auto empty()
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

protected:
    std::map<Key, Value> m_map;
    std::mutex m_mutex;
};
#endif // POCKETCOIN_PROTECTEDMAP_H