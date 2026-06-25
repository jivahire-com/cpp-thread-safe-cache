#pragma once
#include <cstddef>
#include <list>
#include <optional>
#include <unordered_map>
#include <utility>
#include <shared_mutex>
#include <mutex>

// LRUCache<K, V> — a fixed-capacity cache that evicts the least-recently-used
// (LRU) entry when full. The starter is single-threaded.
//
// Contract (README.md is authoritative):
//   * Never holds more than `capacity` entries.
//   * get(key) returns the value if present and marks it most-recently-used.
//   * put(key, value) inserts or updates; at capacity it evicts the LRU entry.
//   * Every operation must be safe to call concurrently from multiple threads.
//
// Workload: read-heavy — get() is called far more often than put().

template <typename K, typename V>
class LRUCache {
public:
    struct Stats {
        uint64_t hits;
        uint64_t misses;
        uint64_t evictions;
    };

    explicit LRUCache(size_t capacity) : capacity_(capacity) {}

    std::optional<V> get(const K& key) {
        std::unique_lock<std::shared_mutex> lock(mutex_);
        auto it = map_.find(key);
        if (it == map_.end()) {
            misses_.fetch_add(1, std::memory_order_relaxed);
            return std::nullopt;
        }
        hits_.fetch_add(1, std::memory_order_relaxed);
        list_.splice(list_.begin(), list_, it->second);
        return it->second->second;
    }

    void put(const K& key, V value) {
        if (capacity_ == 0) return;
        std::unique_lock<std::shared_mutex> lock(mutex_);
        auto it = map_.find(key);
        if (it != map_.end()) {
            list_.splice(list_.begin(), list_, it->second);
            it->second->second = std::move(value);
            return;
        }
        while (list_.size() >= capacity_) {
            auto last = std::prev(list_.end());
            map_.erase(last->first);
            list_.erase(last);
            evictions_.fetch_add(1, std::memory_order_relaxed);
        }
        list_.emplace_front(key, std::move(value));
        map_[key] = list_.begin();
    }

    size_t size() const {
        std::shared_lock<std::shared_mutex> lock(mutex_);
        return list_.size();
    }

    void clear() {
        std::unique_lock<std::shared_mutex> lock(mutex_);
        list_.clear();
        map_.clear();
        hits_.store(0, std::memory_order_relaxed);
        misses_.store(0, std::memory_order_relaxed);
        evictions_.store(0, std::memory_order_relaxed);
    }

    Stats get_stats() const {
        return {
            hits_.load(std::memory_order_relaxed),
            misses_.load(std::memory_order_relaxed),
            evictions_.load(std::memory_order_relaxed)
        };
    }

private:
    size_t capacity_;
    std::list<std::pair<K, V>> list_;
    std::unordered_map<K, typename std::list<std::pair<K, V>>::iterator> map_;
    mutable std::shared_mutex mutex_;

    mutable std::atomic<uint64_t> hits_{0};
    mutable std::atomic<uint64_t> misses_{0};
    std::atomic<uint64_t> evictions_{0};
};
