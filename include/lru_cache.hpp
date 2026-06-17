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
// Workload: read-heavy — get() is called far more often than put().
//
// Locking rationale:
//   A std::mutex (exclusive lock) is used for ALL operations, including get().
//   Although get() looks like a read, it calls list_.splice() to promote the
//   accessed entry to most-recently-used — that mutates the list. Using a
//   shared_lock on get() would allow multiple threads to mutate list_
//   simultaneously, which is a data race. An exclusive lock on every operation
//   is the correct and safe choice here.

template <typename K, typename V>
class LRUCache {
public:
    explicit LRUCache(size_t capacity) : capacity_(capacity) {}

    std::optional<V> get(const K& key) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = map_.find(key);
        if (it == map_.end()) return std::nullopt;
        // Promote to most-recently-used (mutates list_ — needs exclusive lock)
        list_.splice(list_.begin(), list_, it->second);
        return it->second->second;
    }

    void put(const K& key, V value) {
        std::lock_guard<std::mutex> lock(mutex_);

        // capacity_ == 0: never store anything (valid no-op cache)
        if (capacity_ == 0) return;

        auto it = map_.find(key);
        if (it != map_.end()) {
            // Update existing key: move to front, update value, size unchanged
            list_.splice(list_.begin(), list_, it->second);
            it->second->second = std::move(value);
            return;
        }

        // Evict LRU entries until there is room for the new one.
        // Fix: use >= (not >) so we evict before inserting, keeping size <= capacity_
        while (list_.size() >= capacity_) {
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
    std::list<std::pair<K, V>>                                    list_;
    std::unordered_map<K, typename std::list<std::pair<K,V>>::iterator> map_;
    mutable std::mutex mutex_;
};
