#include <catch.hpp>

#include <string>
#include <utility>

#include <boost/foreach.hpp>
#include <boost/move/unique_ptr.hpp>

#include <libcuckoo/cuckoohash_map.hh>
#include "unit_test_util.hh"

typedef boost::movelib::unique_ptr<int> uptr;
struct uptr_hash {
    size_t operator()(const uptr& ptr) const {
        return (*ptr) * 0xc6a4a7935bd1e995;
    }
};

struct uptr_eq {
    bool operator()(const uptr& ptr1, const uptr& ptr2) const {
        return *ptr1 == *ptr2;
    }
};

typedef cuckoohash_map<uptr, uptr, uptr_hash, uptr_eq> uptr_tbl;

const size_t TBL_INIT = 1;
const size_t TBL_SIZE = TBL_INIT * uptr_tbl::slot_per_bucket() * 2;

struct CheckKeyFn {
    explicit CheckKeyFn(int expected_val) : expected_val(expected_val) {}
    void operator()(const uptr& ptr) const {
        REQUIRE(*ptr == expected_val);
    }
    int expected_val;
};

void check_key_eq(uptr_tbl& tbl, int key, int expected_val) {
    uptr up(new int(key));
    REQUIRE(tbl.contains(uptr(new int(key))));
    tbl.update_fn(boost::move(up), CheckKeyFn(expected_val));
}

TEST_CASE("noncopyable insert and update", "[noncopyable]") {
    uptr_tbl tbl(TBL_INIT);
    for (size_t i = 0; i < TBL_SIZE; ++i) {
        uptr up1(new int(i)), up2(new int(i));
        tbl.insert(boost::move(up1), boost::move(up2));
    }
    for (size_t i = 0; i < TBL_SIZE; ++i) {
        check_key_eq(tbl, i, i);
    }
    for (size_t i = 0; i < TBL_SIZE; ++i) {
        uptr up1(new int(i)), up2(new int(i + 1));
        tbl.update(boost::move(up1), boost::move(up2));
    }
    for (size_t i = 0; i < TBL_SIZE; ++i) {
        check_key_eq(tbl, i, i + 1);
    }
}

struct IncrementFn {
    void operator()(uptr& ptr) const {
        *ptr += 1;
    }
};

TEST_CASE("noncopyable upsert", "[noncopyable]") {
    uptr_tbl tbl(TBL_INIT);
    for (size_t i = 0; i < TBL_SIZE; ++i) {
        uptr up1(new int(i)), up2(new int(i));
        tbl.upsert(boost::move(up1), IncrementFn(), boost::move(up2));
    }
    for (size_t i = 0; i < TBL_SIZE; ++i) {
        check_key_eq(tbl, i, i);
    }
    for (size_t i = 0; i < TBL_SIZE; ++i) {
        uptr up1(new int(i)), up2(new int(i));
        tbl.upsert(boost::move(up1), IncrementFn(), boost::move(up2));
    }
    for (size_t i = 0; i < TBL_SIZE; ++i) {
        check_key_eq(tbl, i, i + 1);
    }
}

TEST_CASE("noncopyable iteration", "[noncopyable]") {
    uptr_tbl tbl(TBL_INIT);
    for (size_t i = 0; i < TBL_SIZE; ++i) {
        uptr up1(new int(i)), up2(new int(i));
        tbl.insert(boost::move(up1), boost::move(up2));
    }
    {
        uptr_tbl::locked_table locked_tbl = tbl.lock_table();
        BOOST_FOREACH (uptr_tbl::value_type& kv, locked_tbl) {
            REQUIRE(*kv.first == *kv.second);
            *kv.second += 1;
        }
    }
    {
        uptr_tbl::locked_table locked_tbl = tbl.lock_table();
        BOOST_FOREACH (uptr_tbl::value_type& kv, locked_tbl) {
            REQUIRE(*kv.first == *kv.second - 1);
        }
    }
}

struct UpdateFn {
    UpdateFn(std::string& k) : k(k) {}

    template <typename T>
    void operator()(T& t) const {
        BOOST_FOREACH (char c, k) {
            t->insert(c, std::string(k));
        }
    }
    std::string& k;
};

struct VerifyFn {
    VerifyFn(std::string& k) : k(k) {}

    template <typename T>
    void operator()(T& t) const {
        BOOST_FOREACH (char c, k) {
            REQUIRE(t->find(c) == k);
        }
    }
    std::string& k;
};

TEST_CASE("nested table", "[noncopyable]") {
    typedef cuckoohash_map<char, std::string> inner_tbl;
    typedef cuckoohash_map<std::string, boost::movelib::unique_ptr<inner_tbl> >
        nested_tbl;
    nested_tbl tbl;
    std::string keys[] = {"abc", "def"};
    BOOST_FOREACH (std::string& k, keys) {
        nested_tbl::mapped_type tbl2(new inner_tbl);
        tbl.insert(std::string(k), boost::move(tbl2));
        tbl.update_fn(k, UpdateFn(k));
    }
    BOOST_FOREACH (std::string& k, keys) {
        REQUIRE(tbl.contains(k));
        tbl.update_fn(k, VerifyFn(k));
    }
}
