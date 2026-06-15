#pragma once

#include <cstddef>
#include <list>
#include <mutex>
#include <optional>
#include <unordered_map>
#include <utility>
#include <atomic>
#include <functional>

template <typename K, typename V>
class LRUCache {
public:
    using EvictionCallback = std::function<void(const K&, const V&)>;

    explicit LRUCache(size_t capacity)
        : capacity_(capacity),
          hits_(0),
          misses_(0),
          evictions_(0) {}

    std::optional<V> get(const K& key) {
        std::lock_guard<std::mutex> lock(mutex_);

        auto it = map_.find(key);
        if (it == map_.end()) {
            ++misses_;
            return std::nullopt;
        }

        ++hits_;

        // Move to front (most recently used)
        list_.splice(list_.begin(), list_, it->second);

        return it->second->second;
    }

    void put(const K& key, V value) {
        std::lock_guard<std::mutex> lock(mutex_);

        // Capacity 0 means never store anything
        if (capacity_ == 0) {
            return;
        }

        auto it = map_.find(key);

        // Update existing key
        if (it != map_.end()) {
            it->second->second = std::move(value);

            // Mark as recently used
            list_.splice(list_.begin(), list_, it->second);
            return;
        }

        // Evict if at capacity
        if (list_.size() >= capacity_) {
            auto last = std::prev(list_.end());

            K evicted_key = last->first;
            V evicted_value = last->second;

            map_.erase(evicted_key);
            list_.pop_back();

            ++evictions_;

            if (eviction_callback_) {
                eviction_callback_(evicted_key, evicted_value);
            }
        }

        // Insert new item at front
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

    // ----- Bonus: Metrics -----

    size_t hits() const {
        return hits_.load();
    }

    size_t misses() const {
        return misses_.load();
    }

    size_t evictions() const {
        return evictions_.load();
    }

    double hit_rate() const {
        size_t h = hits_.load();
        size_t m = misses_.load();

        size_t total = h + m;
        return total == 0
            ? 0.0
            : static_cast<double>(h) / total;
    }

    // ----- Bonus: Eviction notifications -----

    void set_eviction_callback(EvictionCallback callback) {
        std::lock_guard<std::mutex> lock(mutex_);
        eviction_callback_ = std::move(callback);
    }

private:
    size_t capacity_;

    // Front = MRU, Back = LRU
    std::list<std::pair<K, V>> list_;

    std::unordered_map<
        K,
        typename std::list<std::pair<K, V>>::iterator
    > map_;

    mutable std::mutex mutex_;

    // Bonus metrics
    std::atomic<size_t> hits_;
    std::atomic<size_t> misses_;
    std::atomic<size_t> evictions_;

    // Bonus notification
    EvictionCallback eviction_callback_;
};