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
// Locking: get() splices the accessed node to the front of the list, which
// mutates internal state — it is NOT a read-only operation. Therefore both
// get() and put() hold a std::unique_lock (exclusive) on mutex_. Using a
// shared_lock on get() would be incorrect because two concurrent get() calls
// would both mutate list_ under a shared lock, producing a data race.

template <typename K, typename V>
class LRUCache {
public:
    explicit LRUCache(size_t capacity) : capacity_(capacity) {}

    std::optional<V> get(const K& key) {
        std::unique_lock<std::mutex> lock(mutex_);
        auto it = map_.find(key);
        if (it == map_.end()) return std::nullopt;
        list_.splice(list_.begin(), list_, it->second);
        return it->second->second;
    }

    void put(const K& key, V value) {
        std::unique_lock<std::mutex> lock(mutex_);
        auto it = map_.find(key);
        if (it != map_.end()) {
            // Key already exists: update value and move to MRU position.
            it->second->second = std::move(value);
            list_.splice(list_.begin(), list_, it->second);
            return;
        }
        // Evict LRU entries until there is room for the new entry.
        // Condition is >= so we drain to capacity-1 before inserting,
        // ensuring size never exceeds capacity.
        while (list_.size() >= capacity_) {
            if (capacity_ == 0) return;   // capacity 0: never store anything
            auto last = std::prev(list_.end());
            map_.erase(last->first);
            list_.erase(last);
        }
        list_.emplace_front(key, std::move(value));
        map_[key] = list_.begin();
    }

    size_t size() const {
        std::unique_lock<std::mutex> lock(mutex_);
        return list_.size();
    }

    void clear() {
        std::unique_lock<std::mutex> lock(mutex_);
        list_.clear();
        map_.clear();
    }

private:
    size_t capacity_;
    std::list<std::pair<K, V>>                                    list_;
    std::unordered_map<K, typename std::list<std::pair<K, V>>::iterator> map_;
    mutable std::mutex mutex_;
};
