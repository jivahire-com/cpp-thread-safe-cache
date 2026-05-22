#pragma once
#include <cstddef>
#include <list>
#include <mutex>
#include <optional>
#include <shared_mutex>
#include <unordered_map>
#include <utility>

// Thread-safe LRU (Least Recently Used) Cache
//
// Example usage:
//   LRUCache<int, std::string> cache(3);
//   cache.put(1, "one");   // cache: {1->one}
//   cache.put(2, "two");   // cache: {2->two, 1->one}
//   cache.put(3, "three"); // cache: {3->three, 2->two, 1->one}
//   cache.get(1);          // cache: {1->one, 3->three, 2->two}  (1 promoted to MRU)
//   cache.put(4, "four");  // cache: {4->four, 1->one, 3->three} (2 evicted as LRU)
//   cache.get(2);          // returns std::nullopt (evicted)

template <typename K, typename V>
class LRUCache {
public:
    explicit LRUCache(size_t capacity) : capacity_(capacity) {}

    std::optional<V> get(const K& key) {
        std::unique_lock lock(mutex_);
        auto it = map_.find(key);
        if (it == map_.end()) return std::nullopt;
        list_.splice(list_.begin(), list_, it->second);
        return it->second->second;
    }

    void put(const K& key, V value) {
        std::unique_lock lock(mutex_);
        if (capacity_ == 0) return;

        auto it = map_.find(key);
        if (it != map_.end()) {
            list_.splice(list_.begin(), list_, it->second);
            it->second->second = std::move(value);
            return;
        }
        // Fix: evict when AT capacity (>= instead of >), before inserting
        while (list_.size() >= capacity_) {
            auto last = std::prev(list_.end());
            map_.erase(last->first);
            list_.erase(last);
        }
        list_.emplace_front(key, std::move(value));
        map_[key] = list_.begin();
    }

    size_t size() const {
        std::shared_lock lock(mutex_);
        return list_.size();
    }

    void clear() {
        std::unique_lock lock(mutex_);
        list_.clear();
        map_.clear();
    }

private:
    size_t capacity_;
    std::list<std::pair<K, V>> list_;
    std::unordered_map<K, typename std::list<std::pair<K, V>>::iterator> map_;
    mutable std::shared_mutex mutex_;
};
