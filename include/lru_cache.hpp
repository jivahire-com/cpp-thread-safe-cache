#pragma once
#include <cstddef>
#include <list>
#include <mutex>
#include <optional>
#include <unordered_map>
#include <utility>

// LRUCache<K, V> — a fixed-capacity, thread-safe cache that evicts the
// least-recently-used (LRU) entry when full.
//
// Concurrency design:
// A single std::mutex guards every operation. get() looks like a read but it
// promotes the entry it returns (splicing the node to the front), so it MUTATES
// internal state — it cannot run under a shared/read lock. One exclusive mutex
// is therefore both the simplest and the correct choice here; a shared_mutex on
// get() would be a data race.

template <typename K, typename V>
class LRUCache {
public:
    explicit LRUCache(size_t capacity) : capacity_(capacity) {}

    // Returns the value for key and promotes it to most-recently-used.
    // Returns std::nullopt if not present.
    // Thread-safe: holds exclusive lock (get() mutates the list).
    std::optional<V> get(const K& key) {
        std::lock_guard<std::mutex> lk(mutex_);
        auto it = map_.find(key);
        if (it == map_.end()) return std::nullopt;
        list_.splice(list_.begin(), list_, it->second);
        return it->second->second;
    }

    // Inserts or updates key→value. Evicts the least-recently-used entry
    // when the cache is at capacity.
    // Thread-safe: holds exclusive lock for the entire operation.
    void put(const K& key, V value) {
        std::lock_guard<std::mutex> lk(mutex_);
        
        // Handle capacity == 0: never store anything
        if (capacity_ == 0) return;
        
        auto it = map_.find(key);
        if (it != map_.end()) {
            // Update existing key: move to front and update value
            list_.splice(list_.begin(), list_, it->second);
            it->second->second = std::move(value);
            return;
        }
        
        // Evict LRU entries until there's room for the new one.
        // Use >= to ensure we never exceed capacity.
        while (list_.size() >= capacity_) {
            auto last = std::prev(list_.end());
            map_.erase(last->first);
            list_.erase(last);
        }
        
        list_.emplace_front(key, std::move(value));
        map_[key] = list_.begin();
    }

    size_t size() const {
        std::lock_guard<std::mutex> lk(mutex_);
        return list_.size();
    }

    void clear() {
        std::lock_guard<std::mutex> lk(mutex_);
        list_.clear();
        map_.clear();
    }

private:
    mutable std::mutex mutex_;
    size_t capacity_;
    std::list<std::pair<K, V>> list_;
    std::unordered_map<K, typename std::list<std::pair<K, V>>::iterator> map_;
};


// okay this looks like alright.

// private:
//     mutable std::mutex mutex_;
//     size_t capacity_;
//     std::list<std::pair<K, V>> list_;
//     std::unordered_map<K, typename std::list<std::pair<K, V>>::iterator> map_;
// };


// // okay this looks like alright.