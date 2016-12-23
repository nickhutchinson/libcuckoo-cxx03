#include <catch.hpp>

#include <algorithm>
#include <stdexcept>
#include <string>
#include <utility>

#include <boost/lexical_cast.hpp>
#include <boost/thread.hpp>

#include <libcuckoo/cuckoohash_map.hh>

#include "unit_test_util.hh"

TEST_CASE("bracket find empty table", "[bracket]") {
    IntIntTable table;
    IntIntTable::reference ref = table[10];
    REQUIRE_THROWS_AS((void) ((int) ref), std::out_of_range);
}

TEST_CASE("bracket find filled table", "[bracket]") {
    StringIntTable table;
    for (int i = 0; i < 10; ++i) {
        table.insert(boost::lexical_cast<std::string>(i), i);
    }

    for (int i = 0; i < 10; ++i) {
        REQUIRE(table[boost::lexical_cast<std::string>(i)] == i);
    }
}

TEST_CASE("bracket insert", "[bracket]") {
    IntIntTable table;
    for (int i = 0; i < 10; ++i) {
        table[i] = i + 1;
    }
    for (int i = 0; i < 10; ++i) {
        REQUIRE(table[i] == i + 1);
    }
}

TEST_CASE("bracket assign to reference", "[bracket]") {
    IntIntTable table;
    table.insert(0, 0);
    for (int i = 1; i < 10; ++i) {
        table[i] = table[i - 1];
    }
    for (int i = 0; i < 10; ++i) {
        REQUIRE(table[i] == 0);
    }
}

TEST_CASE("bracket assign updates", "[bracket]") {
    IntIntTable table;
    table.insert(0, 0);
    REQUIRE(table[0] == 0);
    table[0] = 10;
    REQUIRE(table[0] == 10);
}
