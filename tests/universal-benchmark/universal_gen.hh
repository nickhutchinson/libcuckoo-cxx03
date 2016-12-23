#ifndef _UNIVERSAL_GEN_HH
#define _UNIVERSAL_GEN_HH

#include <stdint.h>
#include <bitset>
#include <memory>
#include <string>

#include <boost/config.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>

/* A specialized functor for generating unique keys and values for various
 * types. Must define one for each type we want to use. */

template <typename T>
class Gen {
    // static T key(seq_t seq, thread_id_t thread_id, thread_id_t num_threads)
    // static T value()
};

template <>
class Gen<uint64_t> {
public:
    static uint64_t key(uint64_t num) {
        return num;
    }

    static uint64_t value() {
        return 0;
    }
};

template <>
class Gen<std::string> {
    BOOST_STATIC_CONSTEXPR size_t STRING_SIZE = 100;
public:
    static std::string key(uint64_t num) {
        return boost::lexical_cast<std::string>(Gen<uint64_t>::key(num));
    }

    static std::string value() {
        return std::string(STRING_SIZE, '0');
    }
};

// Should be 1KB. Bitset is nice since it already has std::hash specialized.
typedef std::bitset<8192> MediumBlob;

template <>
class Gen<MediumBlob> {
public:
    static MediumBlob key(uint64_t num) {
        return MediumBlob(Gen<uint64_t>::key(num));
    }

    static MediumBlob value() {
        return MediumBlob();
    }
};

// Should be 10KB
typedef std::bitset<81920> BigBlob;

template <>
class Gen<BigBlob> {
public:
    static BigBlob key(uint64_t num) {
        return BigBlob(Gen<uint64_t>::key(num));
    }

    static BigBlob value() {
        return BigBlob();
    }
};

template <typename T>
class Gen<boost::shared_ptr<T> > {
public:
    static boost::shared_ptr<T> key(uint64_t num) {
        return boost::make_shared<T>(Gen<T>::key(num));
    }

    static boost::shared_ptr<T> value() {
        return boost::make_shared<T>(Gen<T>::value());
    }
};

#endif // _UNIVERSAL_GEN_HH
