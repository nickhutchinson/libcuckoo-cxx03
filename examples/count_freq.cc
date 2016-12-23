/* A simple example of using the hash table that counts the
 * frequencies of a sequence of random numbers. */

#include <stdint.h>
#include <algorithm>
#include <iostream>
#include <limits>
#include <utility>
#include <vector>

#include <boost/chrono.hpp>
#include <boost/container/vector.hpp>
#include <boost/foreach.hpp>
#include <boost/random.hpp>
#include <boost/thread.hpp>

#include "../src/cuckoohash_map.hh"

typedef uint32_t KeyType;
typedef cuckoohash_map<KeyType, size_t> Table;
const size_t thread_num = 8;
const size_t total_inserts = 10000000;

struct UpdateFn {
    void operator()(size_t& num) const {
        ++num;
    }
};

void do_inserts(Table& freq_map) {
    boost::random::mt19937_64 gen(
        boost::chrono::system_clock::now().time_since_epoch().count());
    boost::random::uniform_int_distribution<KeyType> dist(
        std::numeric_limits<KeyType>::min(),
        std::numeric_limits<KeyType>::max());
    for (size_t i = 0; i < total_inserts/thread_num; i++) {
        KeyType num = dist(gen);
        // If the number is already in the table, it will increment
        // its count by one. Otherwise it will insert a new entry in
        // the table with count one.
        freq_map.upsert(num, UpdateFn(), 1);
    }
}

int main() {
    Table freq_map;
    freq_map.reserve(total_inserts);
    // Run the inserts in thread_num threads
    boost::container::vector<boost::thread> threads;
    for (size_t i = 0; i < thread_num; i++) {
        threads.emplace_back(&do_inserts, boost::ref(freq_map));
    }
    for (size_t i = 0; i < thread_num; i++) {
        threads[i].join();
    }

    // We iterate through the table and print out the element with the
    // maximum number of occurrences.
    KeyType maxkey = 0;
    size_t maxval = 0;
    {
        Table::locked_table lt = freq_map.lock_table();
        BOOST_FOREACH (const Table::value_type& it, lt) {
            if (it.second > maxval) {
                maxkey = it.first;
                maxval = it.second;
            }
        }
    }

    std::cout << maxkey << " occurred " << maxval << " times." << std::endl;

    // Print some information about the table
    std::cout << "Table size: " << freq_map.size() << std::endl;
    std::cout << "Bucket count: " << freq_map.bucket_count() << std::endl;
    std::cout << "Load factor: " << freq_map.load_factor() << std::endl;
}
