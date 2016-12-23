#include <catch.hpp>

#include <boost/lexical_cast.hpp>

#include <libcuckoo/cuckoohash_map.hh>
#include "unit_test_util.hh"

// Checks that the alt index function returns a different bucket, and can
// recover the old bucket when called with the alternate bucket as the index.
template <class CuckoohashMap>
void check_key(size_t hashpower,
               const typename CuckoohashMap::key_type& key) {
    typename CuckoohashMap::hasher hashfn;
    size_t hv = hashfn(key);
    typename UnitTestInternalAccess::partial_t<CuckoohashMap>::type partial =
        UnitTestInternalAccess::partial_key<CuckoohashMap>(hv);
    size_t bucket = UnitTestInternalAccess::index_hash<CuckoohashMap>(
        hashpower, hv);
    size_t alt_bucket = UnitTestInternalAccess::alt_index<CuckoohashMap>(
        hashpower, partial, bucket);
    size_t orig_bucket = UnitTestInternalAccess::alt_index<CuckoohashMap>(
        hashpower, partial, alt_bucket);

    REQUIRE(bucket != alt_bucket);
    REQUIRE(bucket == orig_bucket);
}

TEST_CASE("int alt index works correctly", "[hash properties]") {
    for (size_t hashpower = 10; hashpower < 15; ++hashpower) {
        for (int key = 0; key < 10000; ++key) {
            check_key<IntIntTable>(hashpower, key);
        }
    }
}

TEST_CASE("string alt index works correctly", "[hash properties]") {
    for (size_t hashpower = 10; hashpower < 15; ++hashpower) {
        for (int key = 0; key < 10000; ++key) {
            check_key<StringIntTable>(hashpower,
                                      boost::lexical_cast<std::string>(key));
        }
    }
}

TEST_CASE("hash with larger hashpower only adds top bits",
          "[hash properties]") {
    std::string key = "abc";
    size_t hv = StringIntTable::hasher()(key);
    for (size_t hashpower = 1; hashpower < 30; ++hashpower) {
        UnitTestInternalAccess::partial_t<StringIntTable>::type partial =
            UnitTestInternalAccess::partial_key<StringIntTable>(hv);
        size_t index_bucket1 = UnitTestInternalAccess::index_hash<
            StringIntTable>(hashpower, hv);
        size_t index_bucket2 = UnitTestInternalAccess::index_hash<
            StringIntTable>(hashpower+1, hv);
        CHECK((index_bucket2 & ~(1LL << hashpower)) == index_bucket1);

        size_t alt_bucket1 = UnitTestInternalAccess::alt_index<
            StringIntTable>(hashpower, partial, index_bucket1);
        size_t alt_bucket2 = UnitTestInternalAccess::alt_index<
            StringIntTable>(hashpower, partial, index_bucket2);

        CHECK((alt_bucket2 & ~(1LL << hashpower)) == alt_bucket1);
    }
}
