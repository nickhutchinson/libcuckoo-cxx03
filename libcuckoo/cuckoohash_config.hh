/** \file */

#ifndef _CUCKOOHASH_CONFIG_HH
#define _CUCKOOHASH_CONFIG_HH

#include <cstddef>
#include <boost/config.hpp>

//! The default maximum number of keys per bucket
BOOST_STATIC_CONSTEXPR size_t LIBCUCKOO_DEFAULT_SLOT_PER_BUCKET = 4;

//! The default number of elements in an empty hash table
BOOST_STATIC_CONSTEXPR size_t LIBCUCKOO_DEFAULT_SIZE =
    (1ULL << 16) * LIBCUCKOO_DEFAULT_SLOT_PER_BUCKET;

//! On a scale of 0 to 16, the memory granularity of the locks array. 0 is the
//! least granular, meaning the array is a contiguous array and thus offers the
//! best performance but the greatest memory overhead. 16 is the most granular,
//! offering the least memory overhead but worse performance.
BOOST_STATIC_CONSTEXPR size_t LIBCUCKOO_LOCK_ARRAY_GRANULARITY = 0;

//! The default minimum load factor that the table allows for automatic
//! expansion. It must be a number between 0.0 and 1.0. The table will throw
//! libcuckoo_load_factor_too_low if the load factor falls below this value
//! during an automatic expansion.
BOOST_STATIC_CONSTEXPR double LIBCUCKOO_DEFAULT_MINIMUM_LOAD_FACTOR = 0.05;

//! An alias for the value that sets no limit on the maximum hashpower. If this
//! value is set as the maximum hashpower limit, there will be no limit. Since 0
//! is the only hashpower that can never occur, it should stay at 0.
BOOST_STATIC_CONSTEXPR size_t LIBCUCKOO_NO_MAXIMUM_HASHPOWER = 0;

//! set LIBCUCKOO_DEBUG to 1 to enable debug output
#ifndef LIBCUCKOO_DEBUG
#define LIBCUCKOO_DEBUG 0
#endif

#endif // _CUCKOOHASH_CONFIG_HH
