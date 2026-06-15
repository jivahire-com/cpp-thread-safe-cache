#pragma once
#include <cstddef>
#include <list>
#include <mutex>
#include <optional>
#include <unordered_map>
#include <utility>

// LRUCache<K, V> — fixed-capacity, evicts least-recently-used when full.
// Thread-safe: every operation is serialized by a single mutex.
//
// Note on the workload hint ("read-heavy"): get() promotes the entry it
// returns (splice → list mutation), so it is NOT a read-only operation.
// A std::shared_mutex + shared_lock on get() would let concurrent "readers"
// mutate list_ → data race. We therefore take an *exclusive* lock in get().

template <typename K, typename V>
class LRUCache {
public:
    explicit LRUCache(size_t capacity) : capacity_(capacity) {}

    std::optional<V> get(const K& key) {
        std::lock_guard<std::mutex> lock(mu_);            // exclusive — get()
mutates the list
        auto it = map_.find(key);
        if (it == map_.end()) return std::nullopt;
        list_.splice(list_.begin(), list_, it->second);
        return it->second->second;
    }

    void put(const K& key, V value) {
        std::lock_guard<std::mutex> lock(mu_);
        if (capacity_ == 0) return;                       // capacity 0 → 
no-op store
        auto it = map_.find(key);
        if (it != map_.end()) {
            list_.splice(list_.begin(), list_, it->second);
            it->second->second = std::move(value);
            return;
        }
        while (list_.size() >= capacity_) {               // >= : evict BEFORE
inserting
            auto last = std::prev(list_.end());
            map_.erase(last->first);
            list_.erase(last);
        }
        list_.emplace_front(key, std::move(value));
        map_[key] = list_.begin();
    }

    size_t size() const {
        std::lock_guard<std::mutex> lock(mu_);
        return list_.size();
    }

    void clear() {
        std::lock_guard<std::mutex> lock(mu_);
        list_.clear();
        map_.clear();
    }

private:
    mutable std::mutex mu_;
    size_t capacity_;
    std::list<std::pair<K, V>> list_;
    std::unordered_map<K, typename std::list<std::pair<K, V>>::iterator> map_;
};


// i think this is it. 