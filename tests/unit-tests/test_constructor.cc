#include <catch.hpp>

#include <cmath>
#include <stdexcept>

#include <boost/functional/hash.hpp>

#include <libcuckoo/cuckoohash_map.hh>
#include "unit_test_util.hh"

#if defined(_MSC_VER) && _MSC_VER < 1900
#define log2(x) (log(static_cast<double>(x)) / log(2.0))
#endif  // defined(_MSC_VER) && _MSC_VER < 1900

TEST_CASE("default size", "[constructor]") {
    IntIntTable tbl;
    REQUIRE(tbl.size() == 0);
    REQUIRE(tbl.empty());
    REQUIRE(tbl.hashpower() == (size_t) log2(DEFAULT_SIZE / 4));
    REQUIRE(tbl.bucket_count() == DEFAULT_SIZE / 4);
    REQUIRE(tbl.load_factor() == 0);
}

TEST_CASE("given size", "[constructor]") {
    IntIntTable tbl(1);
    REQUIRE(tbl.size() == 0);
    REQUIRE(tbl.empty());
    REQUIRE(tbl.hashpower() == 1);
    REQUIRE(tbl.bucket_count() == 2);
    REQUIRE(tbl.load_factor() == 0);
}

TEST_CASE("frees even with exceptions", "[constructor]") {
    typedef IntIntTableWithAlloc<TrackingAllocator<int, 0> >::type
        no_space_table;
    // Should throw when allocating the TableInfo struct
    REQUIRE_THROWS_AS(no_space_table(1), std::bad_alloc);
    REQUIRE(get_unfreed_bytes() == 0);

    typedef IntIntTableWithAlloc<TrackingAllocator<
        int, UnitTestInternalAccess::IntIntBucketSize * 2> >::type
        some_space_table;
    // Should throw when allocating the counters, after the buckets
    REQUIRE_THROWS_AS(some_space_table(1), std::bad_alloc);
    REQUIRE(get_unfreed_bytes() == 0);
}

struct CustomHashFn {
    size_t operator()(size_t) const {
        return 0;
    }
};

TEST_CASE("custom hasher", "[constructor]") {
    cuckoohash_map<size_t, size_t, CustomHashFn> map(
        DEFAULT_SIZE, DEFAULT_MINIMUM_LOAD_FACTOR, NO_MAXIMUM_HASHPOWER);
    for (int i = 0; i < 1000; ++i) {
        REQUIRE(map.hash_function()(i) == 0);
    }
}

struct CustomEqFn {
    bool operator()(size_t, size_t) const {
        return false;
    }
};

TEST_CASE("custom equality", "[constructor]") {
    cuckoohash_map<size_t, size_t, boost::hash<size_t>, CustomEqFn> map(
        DEFAULT_SIZE, DEFAULT_MINIMUM_LOAD_FACTOR, NO_MAXIMUM_HASHPOWER,
        boost::hash<size_t>(), CustomEqFn());

    for (int i = 0; i < 1000; ++i) {
        REQUIRE(map.key_eq()(i, i) == false);
        REQUIRE(map.key_eq()(i, i+1) == false);
    }
}
