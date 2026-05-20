#pragma once
#include <cstddef>
#include <list>
#include <optional>
#include <unordered_map>
#include <utility>
#include <mutex> // Include mutex for thread safety

template <typename K, typename V>
class LRUCache {
public:
explicit LRUCache(size_t capacity) : capacity_(capacity) {}

std::optional<V> get(const K& key) {
std::lock_guard<std::mutex> lock(mutex_); // Lock for thread safety
auto it = map_.find(key);
if (it == map_.end()) return std::nullopt;
list_.splice(list_.begin(), list_, it->second);
return it->second->second;
}

void put(const K& key, V value) {
std::lock_guard<std::mutex> lock(mutex_); // Lock for thread safety
if (capacity_ == 0) return; // Handle capacity == 0 safely

auto it = map_.find(key);
if (it != map_.end()) {
list_.splice(list_.begin(), list_, it->second);
it->second->second = std::move(value);
return;
}
while (list_.size() >= capacity_) { // Correct eviction check
auto last = std::prev(list_.end());
map_.erase(last->first);
list_.erase(last);
}
list_.emplace_front(key, std::move(value));
map_[key] = list_.begin();
}

size_t size() const {
std::lock_guard<std::mutex> lock(mutex_); // Lock for thread safety
return list_.size();
}

void clear() {
std::lock_guard<std::mutex> lock(mutex_); // Lock for thread safety
list_.clear();
map_.clear();
}

private:
size_t capacity_;
std::list<std::pair<K, V>> list_;
std::unordered_map<K, typename std::list<std::pair<K, V>>::iterator> map_;
mutable std::mutex mutex_; // Mutex for synchronization
};