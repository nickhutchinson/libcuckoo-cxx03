#include <catch.hpp>

#include <algorithm>
#include <stdexcept>
#include <string>
#include <utility>

#include <boost/chrono.hpp>
#include <boost/move/move.hpp>
#include <boost/thread.hpp>

#include <libcuckoo/cuckoohash_map.hh>
#include "unit_test_util.hh"

TEST_CASE("empty table iteration", "[iterator]") {
    IntIntTable table;
    {
        IntIntTable::locked_table lt = table.lock_table();
        IntIntTable::locked_table::iterator it = lt.begin();

        REQUIRE(lt.begin() == lt.begin());
        REQUIRE(lt.begin() == lt.end());

        REQUIRE(lt.cbegin() == lt.begin());
        REQUIRE(lt.begin() == lt.end());

        REQUIRE(lt.cbegin() == lt.begin());
        REQUIRE(lt.cend() == lt.end());
    }
}

template <class Iterator>
void AssertIteratorIsReleased(Iterator& it) {
    REQUIRE_THROWS_AS(*it, std::runtime_error);
    REQUIRE_THROWS_AS((void) it->first, std::runtime_error);

    REQUIRE_THROWS_AS(it++, std::runtime_error);
    REQUIRE_THROWS_AS(++it, std::runtime_error);
    REQUIRE_THROWS_AS(it--, std::runtime_error);
    REQUIRE_THROWS_AS(--it, std::runtime_error);
}

template <class LockedTable>
void AssertLockedTableIsReleased(LockedTable& lt) {
    REQUIRE_THROWS_AS(lt.begin(), std::runtime_error);
    REQUIRE_THROWS_AS(lt.end(), std::runtime_error);
    REQUIRE_THROWS_AS(lt.cbegin(), std::runtime_error);
    REQUIRE_THROWS_AS(lt.cend(), std::runtime_error);
}

TEST_CASE("iterator release", "[iterator]") {
    IntIntTable table;
    table.insert(10, 10);

    SECTION("explicit release") {
        IntIntTable::locked_table lt = table.lock_table();
        IntIntTable::locked_table::iterator it = lt.begin();
        lt.release();
        AssertIteratorIsReleased(it);
        AssertLockedTableIsReleased(lt);
    }

    SECTION("release through destructor") {
        IntIntTable::locked_table::iterator it = table.lock_table().begin();
        AssertIteratorIsReleased(it);
    }

    SECTION("released iterator equality") {
        IntIntTable::locked_table lt = table.lock_table();
        IntIntTable::locked_table::iterator it1 = lt.begin();
        IntIntTable::locked_table::iterator it2 = lt.begin();
        REQUIRE(it1 == it2);
        lt.release();
        REQUIRE(it1 != it2);
    }

    SECTION("iterators compare after table is moved") {
        IntIntTable::locked_table lt1 = table.lock_table();
        IntIntTable::locked_table::iterator it1 = lt1.begin();
        IntIntTable::locked_table::iterator it2 = lt1.begin();
        REQUIRE(it1 == it2);
        IntIntTable::locked_table lt2(boost::move(lt1));
        REQUIRE(it1 == it2);
        lt2.release();
    }
}

TEST_CASE("iterator walkthrough", "[iterator]") {
    IntIntTable table;
    for (int i = 0; i < 10; ++i) {
        table.insert(i, i);
    }

    SECTION("forward postfix walkthrough") {
        IntIntTable::locked_table lt = table.lock_table();
        IntIntTable::locked_table::const_iterator it = lt.cbegin();
        for (size_t i = 0; i < table.size(); ++i) {
            REQUIRE((*it).first == (*it).second);
            REQUIRE(it->first == it->second);
            IntIntTable::locked_table::const_iterator old_it = it;
            REQUIRE(old_it == it++);
        }
        REQUIRE(it == lt.end());
    }

    SECTION("forward prefix walkthrough") {
        IntIntTable::locked_table lt = table.lock_table();
        IntIntTable::locked_table::const_iterator it = lt.cbegin();
        for (size_t i = 0; i < table.size(); ++i) {
            REQUIRE((*it).first == (*it).second);
            REQUIRE(it->first == it->second);
            ++it;
        }
        REQUIRE(it == lt.end());
    }

    SECTION("backwards postfix walkthrough") {
        IntIntTable::locked_table lt = table.lock_table();
        IntIntTable::locked_table::const_iterator it = lt.cend();
        for (size_t i = 0; i < table.size(); ++i) {
            IntIntTable::locked_table::const_iterator old_it = it;
            REQUIRE(old_it == it--);
            REQUIRE((*it).first == (*it).second);
            REQUIRE(it->first == it->second);
        }
        REQUIRE(it == lt.begin());
    }

    SECTION("backwards prefix walkthrough") {
        IntIntTable::locked_table lt = table.lock_table();
        IntIntTable::locked_table::const_iterator it = lt.cend();
        for (size_t i = 0; i < table.size(); ++i) {
            --it;
            REQUIRE((*it).first == (*it).second);
            REQUIRE(it->first == it->second);
        }
        REQUIRE(it == lt.begin());
    }

    SECTION("walkthrough works after move") {
        IntIntTable::locked_table lt = table.lock_table();
        IntIntTable::locked_table::const_iterator it = lt.cend();
        IntIntTable::locked_table lt2 = boost::move(lt);
        for (size_t i = 0; i < table.size(); ++i) {
            --it;
            REQUIRE((*it).first == (*it).second);
            REQUIRE(it->first == it->second);
        }
        REQUIRE(it == lt2.begin());

    }
}

TEST_CASE("iterator modification", "[iterator]") {
    IntIntTable table;
    for (int i = 0; i < 10; ++i) {
        table.insert(i, i);
    }

    IntIntTable::locked_table lt = table.lock_table();
    for (IntIntTable::locked_table::iterator it = lt.begin(); it != lt.end();
         ++it) {
        it->second = it->second + 1;
    }

    IntIntTable::locked_table::const_iterator it = lt.cbegin();
    for (size_t i = 0; i < table.size(); ++i) {
        REQUIRE(it->first == it->second - 1);
        ++it;
    }
    REQUIRE(it == lt.end());
}

static void ThreadFn(IntIntTable* table) {
    for (int i = 0; i < 10; ++i) {
        table->insert(i, i);
    }
}

TEST_CASE("lock table blocks inserts", "[iterator]") {
    IntIntTable table;
    IntIntTable::locked_table lt = table.lock_table();
    boost::thread thread(ThreadFn, &table);
    boost::this_thread::sleep_for(boost::chrono::milliseconds(100));
    REQUIRE(table.size() == 0);
    lt.release();
    thread.join();

    REQUIRE(table.size() == 10);
}
