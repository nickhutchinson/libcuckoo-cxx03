// Utilities for unit testing
#ifndef UNIT_TEST_UTIL_HH_
#define UNIT_TEST_UTIL_HH_

#include <stdint.h>

#include <cstddef>
#include <functional>
#include <memory>
#include <new>
#include <string>
#include <utility>

#include <boost/atomic.hpp>
#include <boost/config.hpp>
#include <boost/container/allocator_traits.hpp>
#include <boost/functional/hash.hpp>
#include <boost/move/move.hpp>

#include "../../src/cuckoohash_map.hh"

// Returns a statically allocated value used to keep track of how many unfreed
// bytes have been allocated. This value is shared across all threads.
boost::atomic<int64_t>& get_unfreed_bytes();

// We define a a allocator class that keeps track of how many unfreed bytes have
// been allocated. Users can specify an optional bound for how many bytes can be
// unfreed, and the allocator will fail if asked to allocate above that bound
// (note that behavior with this bound with concurrent allocations will be hard
// to deal with). A bound below 0 is inactive (the default is -1).
template <class T, int64_t BOUND = -1>
class TrackingAllocator {
public:
    typedef T value_type;

    template <class U>
    struct rebind {
        typedef TrackingAllocator<U, BOUND> other;
    };

    TrackingAllocator() {}
    template <class U> TrackingAllocator(
        const TrackingAllocator<U, BOUND>&) {}

    T* allocate(size_t n) {
        const size_t bytes_to_allocate = sizeof(T) * n;
        if (BOUND >= 0 && get_unfreed_bytes() + bytes_to_allocate > BOUND) {
            throw std::bad_alloc();
        }
        get_unfreed_bytes() += bytes_to_allocate;
        base_alloc_t alloc;
        return boost::container::allocator_traits<base_alloc_t>::allocate(alloc,
                                                                          n);
    }

    void deallocate(T* p, size_t n) {
        get_unfreed_bytes() -= (sizeof(T) * n);
        base_alloc_t alloc;
        return boost::container::allocator_traits<base_alloc_t>::deallocate(
            alloc, p, n);
    }

    friend bool operator==(const TrackingAllocator<T, BOUND>&,
                           const TrackingAllocator<T, BOUND>&) {
        return true;
    }

    friend bool operator!=(const TrackingAllocator<T, BOUND>&,
                           const TrackingAllocator<T, BOUND>&) {
        return false;
    }

private:
    typedef std::allocator<T> base_alloc_t;
};

typedef cuckoohash_map<
    int,
    int,
    boost::hash<int>,
    std::equal_to<int>,
    std::allocator<std::pair<const int, int> >,
    4> IntIntTable;

template <class Alloc>
struct IntIntTableWithAlloc {
    typedef cuckoohash_map<
        int,
        int,
        boost::hash<int>,
        std::equal_to<int>,
        Alloc,
        4> type;
};

typedef cuckoohash_map<
    std::string,
    int,
    boost::hash<std::string>,
    std::equal_to<std::string>,
    std::allocator<std::pair<const std::string, int> >,
    4> StringIntTable;

// Returns the number of slots the table has to store key-value pairs.
template <class CuckoohashMap>
size_t table_capacity(const CuckoohashMap& table) {
    return CuckoohashMap::slot_per_bucket * (1ULL << table.hashpower());
}

// Some unit tests need access into certain private data members of the table.
// This class is a friend of the table, so it can access those.
class UnitTestInternalAccess {
public:
    static const size_t IntIntBucketSize = sizeof(IntIntTable::Bucket);

    template <class CuckoohashMap>
    static size_t old_table_info_size(const CuckoohashMap& table) {
        // This is not thread-safe
        return table.old_table_infos.size();
    }

    template <class CuckoohashMap>
    static typename CuckoohashMap::SnapshotNoLockResults snapshot_table_nolock(
        const CuckoohashMap& table) {
        return table.snapshot_table_nolock();
    }

    template <class CuckoohashMap>
    static typename CuckoohashMap::partial_t partial_key(const size_t hv) {
        return CuckoohashMap::partial_key(hv);
    }

    template <class CuckoohashMap>
    static size_t index_hash(const size_t hashpower, const size_t hv) {
        return CuckoohashMap::index_hash(hashpower, hv);
    }

    template <class CuckoohashMap>
    static size_t alt_index(const size_t hashpower,
                            const typename CuckoohashMap::partial_t partial,
                            const size_t index) {
        return CuckoohashMap::alt_index(hashpower, partial, index);
    }

    template <class CuckoohashMap>
    static size_t reserve_calc(size_t n) {
        return CuckoohashMap::reserve_calc(n);
    }
};

#endif // UNIT_TEST_UTIL_HH_
