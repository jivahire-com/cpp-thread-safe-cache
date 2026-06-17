#include <catch2/catch_test_macros.hpp>
#include <string>
#include <stdexcept>

#include "lru_cache.hpp"

TEST_CASE("basic get and put", "[basic]") {
    LRUCache<int, std::string> cache(3);

    cache.put(1, "one");
    cache.put(2, "two");
    cache.put(3, "three");

    REQUIRE(cache.get(1).value() == "one");
    REQUIRE(cache.get(2).value() == "two");
    REQUIRE(cache.get(3).value() == "three");
    REQUIRE_FALSE(cache.get(99).has_value());
}

TEST_CASE("LRU eviction order", "[lru]") {
    LRUCache<int, int> cache(2);

    cache.put(1, 10);
    cache.put(2, 20);

    cache.get(1); // make 1 MRU

    cache.put(3, 30); // evicts 2

    REQUIRE_FALSE(cache.get(2).has_value());
    REQUIRE(cache.get(1).value() == 10);
    REQUIRE(cache.get(3).value() == 30);
}

TEST_CASE("update existing key does not change size", "[update]") {
    LRUCache<int, int> cache(2);

    cache.put(1, 1);
    cache.put(2, 2);

    REQUIRE(cache.size() == 2);

    cache.put(1, 100);

    REQUIRE(cache.size() == 2);
    REQUIRE(cache.get(1).value() == 100);
    REQUIRE(cache.get(2).value() == 2);
}

TEST_CASE("updating existing key promotes it to MRU", "[update][lru]") {
    LRUCache<int, int> cache(2);

    cache.put(1, 10);
    cache.put(2, 20);

    cache.put(1, 100); // 1 becomes MRU

    cache.put(3, 30);

    REQUIRE_FALSE(cache.get(2).has_value());
    REQUIRE(cache.get(1).value() == 100);
    REQUIRE(cache.get(3).value() == 30);
}

TEST_CASE("clear empties the cache", "[clear]") {
    LRUCache<std::string, int> cache(3);

    cache.put("a", 1);
    cache.put("b", 2);

    cache.clear();

    REQUIRE(cache.size() == 0);
    REQUIRE(cache.empty());

    REQUIRE_FALSE(cache.get("a").has_value());
    REQUIRE_FALSE(cache.get("b").has_value());
}

TEST_CASE("cache can be reused after clear", "[clear]") {
    LRUCache<int, int> cache(2);

    cache.put(1, 10);
    cache.put(2, 20);

    cache.clear();

    cache.put(3, 30);

    REQUIRE(cache.size() == 1);
    REQUIRE(cache.get(3).value() == 30);
}

TEST_CASE("capacity reports constructor value", "[capacity]") {
    LRUCache<int, int> cache(5);

    REQUIRE(cache.capacity() == 5);
}

TEST_CASE("empty reports correct state", "[empty]") {
    LRUCache<int, int> cache(2);

    REQUIRE(cache.empty());

    cache.put(1, 10);

    REQUIRE_FALSE(cache.empty());

    cache.clear();

    REQUIRE(cache.empty());
}

TEST_CASE("zero capacity is rejected", "[constructor]") {
    REQUIRE_THROWS_AS(
        LRUCache<int, int>(0),
        std::invalid_argument
    );
}