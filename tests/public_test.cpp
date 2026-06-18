#include <atomic>
#include <optional>
#include <string>
#include <thread>
#include <vector>
#include <catch2/catch_test_macros.hpp>
#include "lru_cache.hpp"

// ────────────────────────────────────────────────────────────────────────────
// [basic] — single-threaded correctness
// ────────────────────────────────────────────────────────────────────────────

// @doc: Round-trips values — what put() stores comes back from get(),
//       and an absent key returns nullopt.
TEST_CASE("basic get and put", "[basic]") {
    LRUCache<int, std::string> cache(3);
    cache.put(1, "one");
    cache.put(2, "two");
    cache.put(3, "three");

    REQUIRE(cache.get(1) == std::optional<std::string>("one"));
    REQUIRE(cache.get(2) == std::optional<std::string>("two"));
    REQUIRE(cache.get(3) == std::optional<std::string>("three"));
    REQUIRE(cache.get(99) == std::nullopt);
}

// @doc: get() promotes an entry to most-recently-used, so the next insert
//       at capacity evicts the other key.
TEST_CASE("LRU eviction order", "[basic]") {
    LRUCache<int, int> cache(2);
    cache.put(1, 10);
    cache.put(2, 20);
    // Access 1 — makes 2 the LRU
    cache.get(1);
    // Insert 3 — must evict 2
    cache.put(3, 30);

    REQUIRE(cache.get(2) == std::nullopt);
    REQUIRE(cache.get(1) == std::optional<int>(10));
    REQUIRE(cache.get(3) == std::optional<int>(30));
}

// @doc: Re-putting an existing key updates its value in place — size stays
//       the same and nothing is evicted.
TEST_CASE("update existing key does not change size", "[basic]") {
    LRUCache<int, int> cache(2);
    cache.put(1, 1);
    cache.put(2, 2);
    REQUIRE(cache.size() == 2);
    cache.put(1, 100);
    REQUIRE(cache.size() == 2);
    REQUIRE(cache.get(2) == std::optional<int>(2));
    REQUIRE(cache.get(1) == std::optional<int>(100));
}

// @doc: clear() drops every entry — size() returns to 0 and previously-stored
//       keys miss.
TEST_CASE("clear empties the cache", "[basic]") {
    LRUCache<std::string, int> cache(3);
    cache.put("a", 1);
    cache.put("b", 2);
    cache.clear();
    REQUIRE(cache.size() == 0);
    REQUIRE(cache.get("a") == std::nullopt);
}

// @doc: Inserting far past capacity must never grow the cache beyond its limit.
//       Exercises the >= eviction threshold fix.
TEST_CASE("size never exceeds capacity under heavy inserts", "[basic]") {
    LRUCache<int, int> cache(3);
    for (int i = 0; i < 20; ++i) cache.put(i, i);
    REQUIRE(cache.size() == 3);
}

// @doc: Updating a key that is already in the cache promotes it to MRU,
//       so it is the LAST to be evicted.
TEST_CASE("update promotes key to most-recently-used", "[basic]") {
    LRUCache<int, int> cache(2);
    cache.put(1, 1);
    cache.put(2, 2);
    // Update key 1 — it should now be MRU; key 2 becomes LRU
    cache.put(1, 99);
    // Insert key 3 — must evict key 2, not key 1
    cache.put(3, 3);
    REQUIRE(cache.get(2) == std::nullopt);      // evicted
    REQUIRE(cache.get(1) == std::optional<int>(99)); // survived, updated value
    REQUIRE(cache.get(3) == std::optional<int>(3));
}

// @doc: A single-slot cache evicts its only entry on every new insert.
TEST_CASE("capacity one always keeps only the latest entry", "[basic]") {
    LRUCache<int, int> cache(1);
    cache.put(1, 10);
    REQUIRE(cache.size() == 1);
    cache.put(2, 20);
    REQUIRE(cache.size() == 1);
    REQUIRE(cache.get(1) == std::nullopt);
    REQUIRE(cache.get(2) == std::optional<int>(20));
}

// ────────────────────────────────────────────────────────────────────────────
// [edge] — boundary and unusual inputs
// ────────────────────────────────────────────────────────────────────────────

// @doc: A capacity-0 cache is a no-op store — put() keeps it empty and every
//       get() misses.
TEST_CASE("capacity zero never stores entries", "[edge]") {
    LRUCache<int, int> cache(0);
    cache.put(1, 1);
    cache.put(2, 2);
    REQUIRE(cache.size() == 0);
    REQUIRE(cache.get(1) == std::nullopt);
    REQUIRE(cache.get(2) == std::nullopt);
}

// @doc: clear() on an already-empty cache is a no-op — no crash, size stays 0.
TEST_CASE("clear on empty cache is safe", "[edge]") {
    LRUCache<int, int> cache(4);
    cache.clear();   // must not crash
    REQUIRE(cache.size() == 0);
}

// @doc: Repeated puts of the same key never grow the cache past 1 item.
TEST_CASE("repeated update of same key keeps size at one", "[edge]") {
    LRUCache<int, int> cache(4);
    for (int i = 0; i < 50; ++i) cache.put(42, i);
    REQUIRE(cache.size() == 1);
    REQUIRE(cache.get(42) == std::optional<int>(49));
}

// @doc: get() on an empty cache returns nullopt cleanly — no crash.
TEST_CASE("get on empty cache returns nullopt", "[edge]") {
    LRUCache<int, int> cache(4);
    REQUIRE(cache.get(1) == std::nullopt);
}

// @doc: Exact-capacity fill — size equals capacity, nothing evicted yet.
TEST_CASE("fill to exact capacity then check size", "[edge]") {
    LRUCache<int, int> cache(5);
    for (int i = 0; i < 5; ++i) cache.put(i, i * 10);
    REQUIRE(cache.size() == 5);
    // All keys still present
    for (int i = 0; i < 5; ++i)
        REQUIRE(cache.get(i) == std::optional<int>(i * 10));
}

// ────────────────────────────────────────────────────────────────────────────
// [thread] — concurrent correctness
// ────────────────────────────────────────────────────────────────────────────

// @doc: Many threads put() distinct keys — every write must survive (no lost
//       updates from data races).
TEST_CASE("concurrent puts produce correct final size", "[thread]") {
    constexpr int THREADS = 8;
    constexpr int OPS     = 100;
    // Capacity large enough that nothing is evicted — every insert must survive.
    LRUCache<int, int> cache(THREADS * OPS);

    std::vector<std::thread> threads;
    for (int t = 0; t < THREADS; ++t) {
        threads.emplace_back([&, t] {
            for (int i = 0; i < OPS; ++i)
                cache.put(t * OPS + i, i);
        });
    }
    for (auto& th : threads) th.join();
    REQUIRE(cache.size() == THREADS * OPS);
}

// @doc: Interleaved get()/put() from many threads must never return a torn
//       value — only the value that was put(), never garbage.
TEST_CASE("concurrent get and put return consistent values", "[thread]") {
    LRUCache<int, int> cache(32);
    for (int i = 0; i < 32; ++i) cache.put(i, i);

    std::atomic<int> bad{0};
    std::vector<std::thread> threads;
    for (int t = 0; t < 8; ++t) {
        threads.emplace_back([&, t] {
            for (int i = 0; i < 200; ++i) {
                int k = (t * 200 + i) % 32;
                cache.put(k, k * 2);
                auto v = cache.get(k);
                // Must be either the old value (k) or the new one (k*2)
                if (v.has_value() && *v != k && *v != k * 2) ++bad;
            }
        });
    }
    for (auto& th : threads) th.join();
    REQUIRE(bad.load() == 0);
}

// @doc: Pure concurrent gets on pre-filled cache — every key must be found.
//       get() mutates list_ (splice), so exclusive lock is required;
//       a shared_lock on get() would race here.
TEST_CASE("concurrent gets on pre-filled cache never miss", "[thread]") {
    LRUCache<int, int> cache(128);
    for (int i = 0; i < 128; ++i) cache.put(i, i);

    std::atomic<int> misses{0};
    std::vector<std::thread> threads;
    for (int t = 0; t < 8; ++t) {
        threads.emplace_back([&] {
            for (int i = 0; i < 500; ++i)
                if (!cache.get(i % 128).has_value()) ++misses;
        });
    }
    for (auto& th : threads) th.join();
    REQUIRE(misses.load() == 0);
}

// @doc: Size never exceeds capacity under concurrent inserts from many threads.
TEST_CASE("size never exceeds capacity under concurrent inserts", "[thread]") {
    constexpr size_t CAP = 16;
    LRUCache<int, int> cache(CAP);

    std::vector<std::thread> threads;
    for (int t = 0; t < 8; ++t) {
        threads.emplace_back([&, t] {
            for (int i = 0; i < 200; ++i)
                cache.put(t * 1000 + i, i);
        });
    }
    for (auto& th : threads) th.join();
    REQUIRE(cache.size() <= CAP);
}

// @doc: clear() called concurrently with puts must not crash or corrupt.
//       After all threads finish, size must be <= capacity.
TEST_CASE("concurrent clear and put does not crash", "[thread]") {
    constexpr size_t CAP = 32;
    LRUCache<int, int> cache(CAP);

    std::atomic<bool> go{false};
    std::vector<std::thread> threads;

    // 4 writer threads
    for (int t = 0; t < 4; ++t) {
        threads.emplace_back([&, t] {
            while (!go.load()) {}
            for (int i = 0; i < 100; ++i)
                cache.put(t * 100 + i, i);
        });
    }
    // 2 clear threads
    for (int t = 0; t < 2; ++t) {
        threads.emplace_back([&] {
            while (!go.load()) {}
            for (int i = 0; i < 20; ++i)
                cache.clear();
        });
    }

    go.store(true);
    for (auto& th : threads) th.join();

    // Must not have crashed and size must be within bounds
    REQUIRE(cache.size() <= CAP);
}
