#pragma once
#include <cstddef>
#include <list>
#include <mutex>
#include <optional>
#include <unordered_map>
#include <utility>

//Key fixes applied: ye me type kar rha hu

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
        if (capacity_ == 0) return;  // capacity=0: no-op
        auto it = map_.find(key);
        if (it != map_.end()) {
            list_.splice(list_.begin(), list_, it->second);
            it->second->second = std::move(value);
            return;
        }
        // Evict LRU entries until we have room
        while (list_.size() >= capacity_) {
            auto last = std::prev(list_.end());
            map_.erase(last->first);
            list_.erase(last);
        }
        list_.emplace_front(key, std::move(value));
        map_[key] = list_.begin();
    }

    size_t size() const { return list_.size(); }

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
