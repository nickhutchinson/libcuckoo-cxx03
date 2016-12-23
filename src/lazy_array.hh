/** \file */

#ifndef _LAZY_ARRAY_HH
#define _LAZY_ARRAY_HH

#include <stdint.h>
#include <algorithm>
#include <cassert>
#include <memory>

#include <boost/array.hpp>
#include <boost/config.hpp>
#include <boost/move/move.hpp>

#include "cuckoohash_util.hh"

// lazy array. A fixed-size array, broken up into segments that are dynamically
// allocated, only when requested. The array size and segment size are
// pre-defined, and are powers of two. The user must make sure the necessary
// segments are allocated before accessing the array.
template <uint8_t OFFSET_BITS, uint8_t SEGMENT_BITS,
          class T, class Alloc = std::allocator<T>
          >
class lazy_array {
    BOOST_STATIC_ASSERT_MSG(SEGMENT_BITS + OFFSET_BITS <= sizeof(size_t) * 8,
                            "The number of segment and offset bits cannot "
                            "exceed  the number of bits in a size_t");

private:
    BOOST_STATIC_CONSTEXPR size_t SEGMENT_SIZE = 1ULL << OFFSET_BITS;
    BOOST_STATIC_CONSTEXPR size_t NUM_SEGMENTS = 1ULL << SEGMENT_BITS;
    // The segments array itself is mutable, so that the const subscript
    // operator can still add segments
    mutable boost::array<T*, NUM_SEGMENTS> segments_;

    void move_other_array(BOOST_RV_REF(lazy_array) arr) {
        clear();
        std::copy(arr.segments_.begin(), arr.segments_.end(),
                  segments_.begin());
        std::fill(arr.segments_.begin(), arr.segments_.end(), NULL);
    }

    inline size_t get_segment(size_t i) const {
        return i >> OFFSET_BITS;
    }

    BOOST_STATIC_CONSTEXPR size_t OFFSET_MASK = ((1ULL << OFFSET_BITS) - 1);
    inline size_t get_offset(size_t i) const {
        return i & OFFSET_MASK;
    }

    // No copying
    BOOST_MOVABLE_BUT_NOT_COPYABLE(lazy_array);

public:
    lazy_array() : segments_() {}

    // Moving is allowed
    lazy_array(BOOST_RV_REF(lazy_array) arr) : segments_() {
        move_other_array(boost::move(arr));
    }
    lazy_array& operator=(BOOST_RV_REF(lazy_array) arr) {
        move_other_vector(boost::move(arr));
        return *this;
    }

    ~lazy_array() {
        clear();
    }

    void clear() {
        for (size_t i = 0; i < segments_.size(); ++i) {
            if (segments_[i] != NULL) {
                destroy_array<T, Alloc>(segments_[i], SEGMENT_SIZE);
                segments_[i] = NULL;
            }
        }
    }

    T& operator[](size_t i) {
        assert(segments_[get_segment(i)] != NULL);
        return segments_[get_segment(i)][get_offset(i)];
    }

    const T& operator[](size_t i) const {
        assert(segments_[get_segment(i)] != NULL);
        return segments_[get_segment(i)][get_offset(i)];
    }

    // Ensures that the array has enough segments to index target elements, not
    // exceeding the total size. The user must ensure that the array is properly
    // allocated before accessing a certain index. This saves having to check
    // every index operation.
    void allocate(size_t target) {
        assert(target <= size());
        if (target == 0) {
            return;
        }
        const size_t last_segment = get_segment(target - 1);
        for (size_t i = 0; i <= last_segment; ++i) {
            if (segments_[i] == NULL) {
                segments_[i] = create_array<T, Alloc>(SEGMENT_SIZE);
            }
        }
    }

    // Returns the number of elements in the array that can be indexed, starting
    // contiguously from the beginning.
    size_t allocated_size() const {
        size_t num_allocated_segments = 0;
        for (;
             (num_allocated_segments < NUM_SEGMENTS &&
              segments_[num_allocated_segments] != NULL);
             ++num_allocated_segments) {}
        return num_allocated_segments * SEGMENT_SIZE;
    }

    static BOOST_CONSTEXPR size_t size() {
        return 1ULL << (OFFSET_BITS + SEGMENT_BITS);
    }
};

#endif // _LAZY_ARRAY_HH
