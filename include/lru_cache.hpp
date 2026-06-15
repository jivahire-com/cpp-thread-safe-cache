#pragma once

#include <cstddef>
#include <list>
#include <mutex>
#include <optional>
#include <unordered_map>
#include <utility>

template <typename K, typename V>
class LRUCache {
public:
    explicit LRUCache(size_t capacity)
        : capacity_(capacity) {}

    std::optional<V> get(const K& key) {
        std::lock_guard<std::mutex> lock(mutex_);

        auto it = map_.find(key);
        if (it == map_.end()) {
            return std::nullopt;
        }

        // Move to front (most recently used)
        list_.splice(list_.begin(), list_, it->second);

        return it->second->second;
    }

    void put(const K& key, V value) {
        std::lock_guard<std::mutex> lock(mutex_);

        // Capacity 0 cache stores nothing
        if (capacity_ == 0) {
            return;
        }

        auto it = map_.find(key);

        // Update existing entry
        if (it != map_.end()) {
            it->second->second = std::move(value);

            // Mark as most recently used
            list_.splice(list_.begin(), list_, it->second);

            return;
        }

        // Evict LRU item if at capacity
        if (list_.size() >= capacity_) {
            auto last = std::prev(list_.end());

            map_.erase(last->first);
            list_.pop_back();
        }

        // Insert new item
        list_.emplace_front(key, std::move(value));
        map_[list_.front().first] = list_.begin();
    }

    size_t size() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return list_.size();
    }

    void clear() {
        std::lock_guard<std::mutex> lock(mutex_);

        list_.clear();
        map_.clear();
    }

private:
    size_t capacity_;

    mutable std::mutex mutex_;

    std::list<std::pair<K, V>> list_;

    std::unordered_map<
        K,
        typename std::list<std::pair<K, V>>::iterator
    > map_;
};

// I will add a comment here and let'see if it works. 