#include <iostream>
#include <string>
#include <optional>
#include "lru_cache.hpp"

static int g_failures = 0;
#define REQUIRE(cond) do { if (!(cond)) { std::cerr << "REQUIRE failed: " #cond << std::endl; ++g_failures; } } while(0)

int main() {
    // basic get and put
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

    // LRU eviction order
    {
        LRUCache<int, int> cache(2);
        cache.put(1, 10);
        cache.put(2, 20);
        cache.get(1);
        cache.put(3, 30);

        REQUIRE(cache.get(2) == std::nullopt);
        REQUIRE(cache.get(1) == std::optional<int>(10));
        REQUIRE(cache.get(3) == std::optional<int>(30));
    }

    // update existing key does not change size
    {
        LRUCache<int, int> cache(2);
        cache.put(1, 1);
        cache.put(2, 2);
        REQUIRE(cache.size() == 2);
        cache.put(1, 100);
        REQUIRE(cache.size() == 2);
        REQUIRE(cache.get(2) == std::optional<int>(2));
        REQUIRE(cache.get(1) == std::optional<int>(100));
    }

    // clear empties the cache
    {
        LRUCache<std::string, int> cache(3);
        cache.put("a", 1);
        cache.put("b", 2);
        cache.clear();
        REQUIRE(cache.size() == 0);
        REQUIRE(cache.get("a") == std::nullopt);
    }

    if (g_failures == 0) {
        std::cout << "ALL TESTS PASSED\n";
    } else {
        std::cerr << g_failures << " TEST(S) FAILED\n";
    }
    return g_failures != 0;
}
