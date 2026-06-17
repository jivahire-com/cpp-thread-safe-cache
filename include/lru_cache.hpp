#pragma once

#include <cstddef>
#include <list>
#include <mutex>
#include <optional>
#include <unordered_map>
#include <utility>

// LRUCache<K, V> — a fixed-capacity cache that evicts the least-recently-used
// (LRU) entry when full.
//
// Contract:
//   * Never holds more than `capacity` entries.
//   * get(key) returns the value if present and marks it most-recently-used.
//   * put(key, value) inserts or updates; at capacity it evicts the LRU entry.
//   * Every operation is safe to call concurrently from multiple threads.

template <typename K, typename V>
class LRUCache
{
public:
    explicit LRUCache(size_t capacity)
        : capacity_(capacity) {}

    std::optional<V> get(const K &key)
    {
        std::lock_guard<std::mutex> lock(mutex_);

        auto it = map_.find(key);
        if (it == map_.end())
        {
            return std::nullopt;
        }

        // Move accessed item to the front (MRU position)
        list_.splice(list_.begin(), list_, it->second);

        return it->second->second;
    }

    void put(const K &key, V value)
    {
        std::lock_guard<std::mutex> lock(mutex_);

        // Capacity zero cache never stores anything.
        if (capacity_ == 0)
        {
            return;
        }

        auto it = map_.find(key);

        // Update existing entry.
        if (it != map_.end())
        {
            list_.splice(list_.begin(), list_, it->second);
            it->second->second = std::move(value);
            return;
        }

        // Evict LRU item if cache is full.
        if (list_.size() >= capacity_)
        {
            auto last = std::prev(list_.end());
            map_.erase(last->first);
            list_.erase(last);
        }

        // Insert new MRU entry.
        list_.emplace_front(key, std::move(value));
        map_[key] = list_.begin();
    }

    size_t size() const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return list_.size();
    }

    void clear()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        list_.clear();
        map_.clear();
    }

private:
    size_t capacity_;

    std::list<std::pair<K, V>> list_;
    std::unordered_map<
        K,
        typename std::list<std::pair<K, V>>::iterator>
        map_;
    mutable std::mutex mutex_;
}
