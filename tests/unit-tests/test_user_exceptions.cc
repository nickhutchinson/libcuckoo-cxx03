#include <catch.hpp>

#include <stdexcept>
#include <boost/functional/hash.hpp>

#include <libcuckoo/cuckoohash_map.hh>
#include "unit_test_util.hh"

void maybeThrow(bool throwException) {
    if (throwException) {
        throw std::runtime_error("exception");
    }
}

bool constructorThrow, moveThrow, hashThrow, equalityThrow;

class ExceptionInt {
public:
    ExceptionInt() {
        maybeThrow(constructorThrow);
        val = 0;
    }

    ExceptionInt(size_t x) {
        maybeThrow(constructorThrow);
        val = x;
    }

    ExceptionInt(const ExceptionInt& i) {
        maybeThrow(constructorThrow);
        val = static_cast<size_t>(i);
    }

    ExceptionInt(BOOST_RV_REF(ExceptionInt) i) {
        maybeThrow(constructorThrow || moveThrow);
        val = static_cast<size_t>(i);
    }

    ExceptionInt& operator=(BOOST_COPY_ASSIGN_REF(ExceptionInt) i) {
        maybeThrow(constructorThrow);
        val = static_cast<size_t>(i);
        return *this;
    }

    ExceptionInt& operator=(BOOST_RV_REF(ExceptionInt) i) {
        maybeThrow(constructorThrow || moveThrow);
        val = static_cast<size_t>(i);
        return *this;
    }

    operator size_t() const {
        return val;
    }

    friend size_t hash_value(const ExceptionInt& x) {
        maybeThrow(hashThrow);
        return x;
    }

private:
    BOOST_COPYABLE_AND_MOVABLE(ExceptionInt);
    size_t val;
};

namespace std {
    template <>
    struct equal_to<ExceptionInt> {
        bool operator()(const ExceptionInt& lhs,
                        const ExceptionInt& rhs) const {
            maybeThrow(equalityThrow);
            return static_cast<size_t>(lhs) == static_cast<size_t>(rhs);
        }
    };
}

typedef cuckoohash_map<ExceptionInt, size_t, boost::hash<ExceptionInt>,
                       std::equal_to<ExceptionInt> > exceptionTable;

void checkIterTable(exceptionTable& tbl, size_t expectedSize) {
    exceptionTable::locked_table lockedTable = tbl.lock_table();
    size_t actualSize = 0;
    for (exceptionTable::locked_table::iterator it = lockedTable.begin();
         it != lockedTable.end(); ++it) {
        ++actualSize;
    }
    REQUIRE(actualSize == expectedSize);
}

struct UpdateFn {
    void operator()(size_t& val) const {
        val++;
    }
};

TEST_CASE("user exceptions", "[user_exceptions]") {
    constructorThrow = hashThrow = equalityThrow = moveThrow = false;
    // We don't use sub-sections because CATCH is not exactly thread-safe

    // "find/contains"
    {
        exceptionTable tbl;
        tbl.insert(1, 1);
        tbl.insert(2, 2);
        tbl.insert(3, 3);
        hashThrow = true;
        REQUIRE_THROWS_AS(tbl.find(3), std::runtime_error);
        REQUIRE_THROWS_AS(tbl.contains(3), std::runtime_error);
        hashThrow = false;
        equalityThrow = true;
        REQUIRE_THROWS_AS(tbl.find(3), std::runtime_error);
        REQUIRE_THROWS_AS(tbl.contains(3), std::runtime_error);
        equalityThrow = false;
        REQUIRE(tbl.find(3) == 3);
        REQUIRE(tbl.contains(3));
        checkIterTable(tbl, 3);
    }

    // "insert"
    {
        exceptionTable tbl;
        constructorThrow = true;
        REQUIRE_THROWS_AS(tbl.insert(100, 100), std::runtime_error);
        constructorThrow = false;
        REQUIRE(tbl.insert(100, 100));
        checkIterTable(tbl, 1);
    }

    // "erase"
    {
        exceptionTable tbl;
        for (int i = 0; i < 10; ++i) {
            tbl.insert(i, i);
        }
        hashThrow = true;
        REQUIRE_THROWS_AS(tbl.erase(5), std::runtime_error);
        hashThrow = false;
        equalityThrow = true;
        REQUIRE_THROWS_AS(tbl.erase(5), std::runtime_error);
        equalityThrow = false;
        REQUIRE(tbl.erase(5));
        checkIterTable(tbl, 9);
    }

    // "update"
    {
        exceptionTable tbl;
        tbl.insert(9, 9);
        tbl.insert(10, 10);
        hashThrow = true;
        REQUIRE_THROWS_AS(tbl.update(9, 10), std::runtime_error);
        hashThrow = false;
        equalityThrow = true;
        REQUIRE_THROWS_AS(tbl.update(9, 10), std::runtime_error);
        equalityThrow = false;
        REQUIRE(tbl.update(9, 10));
        checkIterTable(tbl, 2);
    }

    // "update_fn"
    {
        exceptionTable tbl;
        tbl.insert(9, 9);
        tbl.insert(10, 10);
        hashThrow = true;
        REQUIRE_THROWS_AS(tbl.update_fn(9, UpdateFn()), std::runtime_error);
        hashThrow = false;
        equalityThrow = true;
        REQUIRE_THROWS_AS(tbl.update_fn(9, UpdateFn()), std::runtime_error);
        equalityThrow = false;
        REQUIRE(tbl.update_fn(9, UpdateFn()));
        checkIterTable(tbl, 2);
    }

    // "upsert"
    {
        exceptionTable tbl;
        tbl.insert(9, 9);
        hashThrow = true;
        REQUIRE_THROWS_AS(tbl.upsert(9, UpdateFn(), 10), std::runtime_error);
        hashThrow = false;
        equalityThrow = true;
        REQUIRE_THROWS_AS(tbl.upsert(9, UpdateFn(), 10), std::runtime_error);
        equalityThrow = false;
        tbl.upsert(9, UpdateFn(), 10);
        constructorThrow = true;
        REQUIRE_THROWS_AS(tbl.upsert(10, UpdateFn(), 10), std::runtime_error);
        constructorThrow = false;
        tbl.upsert(10, UpdateFn(), 10);
        checkIterTable(tbl, 2);
    }

    // rehash
    {
        exceptionTable tbl;
        for (int i = 0; i < 10; ++i) {
            tbl.insert(i, i);
        }
        size_t original_hashpower = tbl.hashpower();
        constructorThrow = true;
        REQUIRE_THROWS_AS(tbl.rehash(2), std::runtime_error);
        constructorThrow = false;
        REQUIRE(tbl.hashpower() == original_hashpower);
        hashThrow = true;
        REQUIRE_THROWS_AS(tbl.rehash(2), std::runtime_error);
        hashThrow = false;
        REQUIRE(tbl.hashpower() == original_hashpower);
        // This shouldn't throw, because the partial keys are different between
        // the different hash values, which means they shouldn't be compared for
        // actual equality.
        equalityThrow = true;
        REQUIRE(tbl.rehash(2));
        REQUIRE(tbl.hashpower() == 2);
        equalityThrow = false;
        checkIterTable(tbl, 10);
    }

    // "reserve"
    {
        exceptionTable tbl;
        for (int i = 0; i < 10; ++i) {
            tbl.insert(i, i);
        }
        size_t original_hashpower = tbl.hashpower();
        constructorThrow = true;
        REQUIRE_THROWS_AS(tbl.reserve(10), std::runtime_error);
        constructorThrow = false;
        REQUIRE(tbl.hashpower() == original_hashpower);
        hashThrow = true;
        REQUIRE_THROWS_AS(tbl.reserve(10), std::runtime_error);
        hashThrow = false;
        REQUIRE(tbl.hashpower() == original_hashpower);
        // This shouldn't throw, because the partial keys are different between
        // the different hash values, which means they shouldn't be compared for
        // actual equality.
        equalityThrow = true;
        REQUIRE(tbl.reserve(10));
        REQUIRE(tbl.hashpower() ==
                UnitTestInternalAccess::reserve_calc<exceptionTable>(10));
        checkIterTable(tbl, 10);
    }

    // "insert resize"
    {
        exceptionTable tbl;
        REQUIRE(tbl.rehash(1));
        // Fill up the entire table
        for (size_t i = 0; i < exceptionTable::slot_per_bucket * 2; ++i) {
            tbl.insert(i * 2, 0);
        }
        // Only throw on move, which should be triggered when we do a resize.
        moveThrow = true;
        REQUIRE_THROWS_AS(
            tbl.insert((exceptionTable::slot_per_bucket * 2) * 2, 0),
            std::runtime_error);
        moveThrow = false;
        REQUIRE(tbl.insert((exceptionTable::slot_per_bucket * 2) * 2, 0));
        checkIterTable(tbl, exceptionTable::slot_per_bucket * 2 + 1);
    }

    // "insert cuckoohash" -- broken?
    // {
    //     exceptionTable tbl(0);
    //     REQUIRE(tbl.rehash(2));
    //     size_t cuckooKey = 0;
    //     size_t cuckooKeyHash = std::hash<ExceptionInt>()(cuckooKey);
    //     size_t cuckooKeyIndex = UnitTestInternalAccess::index_hash<
    //         exceptionTable>(tbl.hashpower(), cuckooKeyHash);
    //     size_t cuckooKeyPartial = UnitTestInternalAccess::partial_key<
    //         exceptionTable>(tbl.hashpower(), cuckooKeyHash);
    //     size_t cuckooKeyAltIndex = UnitTestInternalAccess::alt_index<
    //         exceptionTable>(tbl.hashpower(), cuckooKeyPartial, cuckooKeyIndex);

    //     if (cuckooKeyIndex == cuckooKeyAltIndex) {
    //         // Fill up one bucket with elements that have the same value as
    //         // cuckooKeyIndex mod tbl.hashpower()
    //         for (size_t i = 0; i < exceptionTable::slot_per_bucket; ++i) {
    //             tbl.insert(tbl.hashpower() * (i + 1) + cuckooKeyIndex, 0);
    //         }
    //     } else {
    //         // Fill up one bucket on index cuckooKeyIndex, and another bucket on
    //         // cuckooKeyAlt
    //         for (size_t i = 0; i < exceptionTable::slot_per_bucket; ++i) {
    //             tbl.insert(tbl.hashpower() * (i + 1) + cuckooKeyIndex, 0);
    //             tbl.insert(tbl.hashpower() * (i + 1) + cuckooKeyAltIndex, 0);
    //         }
    //     }

    //     // Now inserting cuckooKey should trigger a cuckoo hash, which moves
    //     // elements around
    //     moveThrow = true;
    //     REQUIRE_THROWS_AS(tbl.insert(cuckooKey, 0), std::runtime_error);
    //     moveThrow = false;
    //     REQUIRE(tbl.insert(cuckooKey, 0));
    //     checkIterTable(tbl,
    //                    exceptionTable::slot_per_bucket * (
    //                        cuckooKeyIndex == cuckooKeyAltIndex ? 1 : 2) + 1);
    // }
}
