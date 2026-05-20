#pragma once
#include <cstddef>
#include <list>
#include <optional>
#include <unordered_map>
#include <utility>
#include <shared_mutex>

template <typename K, typename V>
class LRUCache {
public:
explicit LRUCache(size_t capacity) : capacity_(capacity) {}

std::optional<V> get(const K& key) {
std::shared_lock<std::shared_mutex> lock(mutex_);
auto it = map_.find(key);
if (it == map_.end()) return std::nullopt;

// Promote the entry to the front of the list.
// Already within a shared lock, there's no need to upgrade.
lock.unlock();
std::lock_guard<std::shared_mutex> exclusive_lock(mutex_);
list_.splice(list_.begin(), list_, it->second);
return it->second->second;
}

void put(const K& key, V value) {
std::lock_guard<std::shared_mutex> lock(mutex_);
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
}
list_.emplace_front(key, std::move(value));
map_[key] = list_.begin();
}

size_t size() const {
std::shared_lock<std::shared_mutex> lock(mutex_);
return list_.size();
}

void clear() {
std::lock_guard<std::shared_mutex> lock(mutex_);
list_.clear();
map_.clear();
}

private:
size_t capacity_;
std::list<std::pair<K, V>> list_;
std::unordered_map<K, typename std::list<std::pair<K, V>>::iterator> map_;
mutable std::shared_mutex mutex_; // Changed to shared_mutex for better read concurrency.
};