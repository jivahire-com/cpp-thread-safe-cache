#include <catch2/catch_test_macros.hpp>
#include <optional>
#include <string>
#include <thread>
#include <vector>
#include "lru_cache.hpp"

TEST_CASE("basic get and put", "[basic]")
{
    LRUCache<int, std::string> cache(3);
    cache.put(1, "one");
    cache.put(2, "two");
    cache.put(3, "three");

    REQUIRE(cache.get(1) == std::optional<std::string>("one"));
    REQUIRE(cache.get(2) == std::optional<std::string>("two"));
    REQUIRE(cache.get(3) == std::optional<std::string>("three"));
    REQUIRE(cache.get(99) == std::nullopt);
}

TEST_CASE("LRU eviction order", "[basic]")
{
    LRUCache<int, int> cache(2);
    cache.put(1, 10);
    cache.put(2, 20);
    // Access 1, making 2 the LRU
    cache.get(1);
    // Insert 3 — should evict 2
    cache.put(3, 30);

    REQUIRE(cache.get(2) == std::nullopt);
    REQUIRE(cache.get(1) == std::optional<int>(10));
    REQUIRE(cache.get(3) == std::optional<int>(30));
}

TEST_CASE("update existing key does not change size", "[basic]")
{
    LRUCache<int, int> cache(2);
    cache.put(1, 1);
    cache.put(2, 2);
    REQUIRE(cache.size() == 2);
    // Update key 1 — size must stay 2, key 2 must still be present
    cache.put(1, 100);
    REQUIRE(cache.size() == 2);
    REQUIRE(cache.get(2) == std::optional<int>(2));
    REQUIRE(cache.get(1) == std::optional<int>(100));
}

TEST_CASE("clear empties the cache", "[basic]")
{
    LRUCache<std::string, int> cache(3);
    cache.put("a", 1);
    cache.put("b", 2);
    cache.clear();
    REQUIRE(cache.size() == 0);
    REQUIRE(cache.get("a") == std::nullopt);
}

TEST_CASE("capacity is never exceeded", "[capacity]")
{
    LRUCache<int, int> cache(2);

    cache.put(1, 1);
    cache.put(2, 2);
    cache.put(3, 3);

    REQUIRE(cache.size() == 2);

    int found = 0;
    if (cache.get(1))
        ++found;
    if (cache.get(2))
        ++found;
    if (cache.get(3))
        ++found;

    REQUIRE(found == 2);
}

TEST_CASE("capacity zero cache stores nothing", "[capacity]")
{
    LRUCache<int, int> cache(0);

    cache.put(1, 10);
    cache.put(2, 20);

    REQUIRE(cache.size() == 0);
    REQUIRE(cache.get(1) == std::nullopt);
    REQUIRE(cache.get(2) == std::nullopt);
}

TEST_CASE("updating existing key makes it most recently used", "[lru]")
{
    LRUCache<int, int> cache(2);

    cache.put(1, 10);
    cache.put(2, 20);

    // Updating key 1 should move it to MRU position.
    cache.put(1, 100);

    cache.put(3, 30);

    REQUIRE(cache.get(2) == std::nullopt);
    REQUIRE(cache.get(1) == std::optional<int>(100));
    REQUIRE(cache.get(3) == std::optional<int>(30));
}

TEST_CASE("get operation updates recency", "[lru]")
{
    LRUCache<int, int> cache(3);

    cache.put(1, 10);
    cache.put(2, 20);
    cache.put(3, 30);

    REQUIRE(cache.get(1) == std::optional<int>(10));

    cache.put(4, 40);

    REQUIRE(cache.get(2) == std::nullopt);
    REQUIRE(cache.get(1) == std::optional<int>(10));
    REQUIRE(cache.get(3) == std::optional<int>(30));
    REQUIRE(cache.get(4) == std::optional<int>(40));
}

TEST_CASE("concurrent access does not crash", "[threading]")
{
    LRUCache<int, int> cache(100);

    constexpr int iterations = 10000;

    auto writer = [&cache]()
    {
        for (int i = 0; i < iterations; ++i)
        {
            cache.put(i % 200, i);
        }
    };

    auto reader = [&cache]()
    {
        for (int i = 0; i < iterations; ++i)
        {
            (void)cache.get(i % 200);
        }
    };

    std::vector<std::thread> threads;

    for (int i = 0; i < 4; ++i)
    {
        threads.emplace_back(writer);
        threads.emplace_back(reader);
    }

    for (auto &t : threads)
    {
        t.join();
    }

    REQUIRE(cache.size() <= 100);
}
