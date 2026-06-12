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
//   * Never holds more than `capacity` entries (capacity 0 → stores nothing).
//   * get(key) returns the value if present and marks it most-recently-used.
//   * put(key, value) inserts or updates; at capacity it evicts the LRU entry.
//   * Every operation is safe to call concurrently from multiple threads.
//
// Locking strategy: get() splices the accessed node to the front of list_,
// which mutates internal state — it is NOT a read-only operation. Therefore
// both get() and put() must hold an exclusive lock. A shared_mutex with
// shared_lock on get() would be a data race. A plain std::mutex is correct.

template <typename K, typename V>
class LRUCache {
public:
    explicit LRUCache(size_t capacity) : capacity_(capacity) {}

    // Returns the value for key and promotes it to most-recently-used.
    // Returns std::nullopt if not present.
    std::optional<V> get(const K& key) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = map_.find(key);
        if (it == map_.end()) return std::nullopt;
        // splice mutates list_ — needs exclusive lock, not shared
        list_.splice(list_.begin(), list_, it->second);
        return it->second->second;
    }

    // Inserts or updates key→value. Evicts the least-recently-used entry
    // when the cache is at capacity. No-op when capacity == 0.
    void put(const K& key, V value) {
        std::lock_guard<std::mutex> lock(mutex_);

        // capacity 0: never store anything
        if (capacity_ == 0) return;

        auto it = map_.find(key);
        if (it != map_.end()) {
            // Key already exists: update value and promote to MRU
            list_.splice(list_.begin(), list_, it->second);
            it->second->second = std::move(value);
            return;
        }

        // Evict LRU entries until we have room for one more.
        // Fix: condition is >= so we evict *before* inserting, keeping
        // size <= capacity at all times.
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
    std::list<std::pair<K, V>> list_;
    std::unordered_map<K, typename std::list<std::pair<K, V>>::iterator> map_;
    mutable std::mutex mutex_;  // mutable so const size() can lock it
};
