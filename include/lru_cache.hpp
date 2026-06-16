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
// Contract (README.md is authoritative):
//   * Never holds more than `capacity` entries.
//   * get(key) returns the value if present and marks it most-recently-used.
//   * put(key, value) inserts or updates; at capacity it evicts the LRU entry.
//   * Every operation must be safe to call concurrently from multiple threads.
//
// Locking: both get() and put() mutate the internal list (splice moves the
// accessed node to the front), so BOTH operations need an exclusive lock.
// A shared_mutex with shared_lock on get() would be wrong — get() is a write
// to the list and would race with other concurrent gets.

template <typename K, typename V>
class LRUCache {
public:
    explicit LRUCache(size_t capacity) : capacity_(capacity) {}

    std::optional<V> get(const K& key) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = map_.find(key);
        if (it == map_.end()) return std::nullopt;
        list_.splice(list_.begin(), list_, it->second);
        return it->second->second;
    }

    void put(const K& key, V value) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = map_.find(key);
        if (it != map_.end()) {
            list_.splice(list_.begin(), list_, it->second);
            it->second->second = std::move(value);
            return;
        }
        // Evict LRU entries BEFORE inserting the new one, so we never
        // exceed capacity. Condition is >= (not >) because the new item
        // is not in the list yet — this was the planted off-by-one bug.
        while (list_.size() >= capacity_) {
            if (capacity_ == 0) return;  // capacity 0: never store anything
            auto last = std::prev(list_.end());
            map_.erase(last->first);
            list_.erase(last);
        }
        list_.emplace_front(key, std::move(value));
        map_[key] = list_.begin();
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
    std::list<std::pair<K, V>> list_;
    std::unordered_map<K, typename std::list<std::pair<K, V>>::iterator> map_;
    mutable std::mutex mutex_;
};
