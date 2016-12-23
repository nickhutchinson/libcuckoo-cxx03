#include "unit_test_util.hh"

#include <stdint.h>

#include <boost/atomic.hpp>

boost::atomic<int64_t>& get_unfreed_bytes() {
    static boost::atomic<int64_t> unfreed_bytes(0L);
    return unfreed_bytes;
}
