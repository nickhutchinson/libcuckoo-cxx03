/** \file */

#ifndef _CUCKOOHASH_UTIL_HH
#define _CUCKOOHASH_UTIL_HH

#include <exception>

#include <boost/align/align.hpp>
#include <boost/align/alignment_of.hpp>
#include <boost/config.hpp>
#include <boost/exception/exception.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/thread.hpp>
#include "cuckoohash_config.hh" // for LIBCUCKOO_DEBUG

#if LIBCUCKOO_DEBUG
#define LIBCUCKOO_DBG(fmt, ...)                                              \
    fprintf(stderr, "\x1b[32m[libcuckoo:%s:%d:%s] " fmt "\x1b[0m", __FILE__, \
            __LINE__,                                                        \
            boost::lexical_cast<std::string>(boost::this_thread::get_id())   \
                .c_str(),                                                    \
            __VA_ARGS__)
#else
//! When \ref LIBCUCKOO_DEBUG is 0, LIBCUCKOO_DBG does nothing
#  define LIBCUCKOO_DBG(fmt, ...)  do {} while (0)
#endif

/**
 * At higher warning levels, MSVC produces an annoying warning that alignment
 * may cause wasted space: "structure was padded due to __declspec(align())".
 */
#ifdef _MSC_VER
#define LIBCUCKOO_SQUELCH_PADDING_WARNING __pragma(warning(suppress : 4324))
#else
#define LIBCUCKOO_SQUELCH_PADDING_WARNING
#endif

#ifdef _MSC_VER
#define LIBCUCKOO_THREAD_LOCAL __declspec(thread)
#else
#define LIBCUCKOO_THREAD_LOCAL __thread
#endif

#ifndef BOOST_NO_CXX11_FINAL
#define LIBCUCKOO_OVERRIDE override
#define LIBCUCKOO_FINAL final
#else
#define LIBCUCKOO_OVERRIDE
#define LIBCUCKOO_FINAL
#endif

#ifndef __has_feature
#define __has_feature(x) 0
#endif

#if __has_feature(thread_sanitizer)
extern "C" {
void AnnotateHappensAfter(const char* file, int line, const void* addr);
void AnnotateHappensBefore(const char* file, int line, const void* addr);
}
#define LIBCUCKOO_ANNOTATE_HAPPENS_BEFORE(addr) \
    AnnotateHappensBefore(__FILE__, __LINE__, addr)
#define LIBCUCKOO_ANNOTATE_HAPPENS_AFTER(addr) \
    AnnotateHappensAfter(__FILE__, __LINE__, addr)
#else
#define LIBCUCKOO_ANNOTATE_HAPPENS_BEFORE(addr)
#define LIBCUCKOO_ANNOTATE_HAPPENS_AFTER(addr)
#endif  // __has_feature(thread_sanitizer)

/**
 * Thrown when an automatic expansion is triggered, but the load factor of the
 * table is below a minimum threshold, which can be set by the \ref
 * cuckoohash_map::minimum_load_factor method. This can happen if the hash
 * function does not properly distribute keys, or for certain adversarial
 * workloads.
 */
class libcuckoo_load_factor_too_low : public std::exception,
                                      public boost::exception {
public:
    /**
     * Constructor
     *
     * @param lf the load factor of the table when the exception was thrown
     */
    libcuckoo_load_factor_too_low(const double lf)
        : load_factor_(lf) {}

    /**
     * @return a descriptive error message
     */
    virtual const char* what() const BOOST_NOEXCEPT_OR_NOTHROW
        LIBCUCKOO_OVERRIDE {
        return "Automatic expansion triggered when load factor was below "
               "minimum threshold";
    }

    /**
     * @return the load factor of the table when the exception was thrown
     */
    double load_factor() const {
        return load_factor_;
    }
private:
    const double load_factor_;
};

/**
 * Thrown when an expansion is triggered, but the hashpower specified is greater
 * than the maximum, which can be set with the \ref
 * cuckoohash_map::maximum_hashpower method.
 */
class libcuckoo_maximum_hashpower_exceeded : public std::exception,
                                             public boost::exception {
public:
    /**
     * Constructor
     *
     * @param hp the hash power we were trying to expand to
     */
    libcuckoo_maximum_hashpower_exceeded(const size_t hp)
        : hashpower_(hp) {}

    /**
     * @return a descriptive error message
     */
    virtual const char* what() const BOOST_NOEXCEPT_OR_NOTHROW
        LIBCUCKOO_OVERRIDE {
        return "Expansion beyond maximum hashpower";
    }

    /**
     * @return the hashpower we were trying to expand to
     */
    size_t hashpower() const {
        return hashpower_;
    }
private:
    const size_t hashpower_;
};

template <typename Alloc,
          size_t Alignment =
              boost::alignment::alignment_of<typename Alloc::value_type>::value>
class libcuckoo_aligned_allocator {
public:
    typedef typename Alloc::value_type value_type;

    template <typename U>
    struct rebind {
        typedef libcuckoo_aligned_allocator<U, Alignment> other;
    };

    libcuckoo_aligned_allocator() {}
    libcuckoo_aligned_allocator(const libcuckoo_aligned_allocator&) {}

    template <typename U>
    libcuckoo_aligned_allocator(
        const libcuckoo_aligned_allocator<U, Alignment>&) {}

    value_type* allocate(size_t n) const {
        typedef typename boost::container::allocator_traits<
            Alloc>::template rebind_traits<char>
            traits_t;
        typename traits_t::allocator_type alloc;

        const size_t size = n * sizeof(value_type);
        size_t paddedSize = size + Alignment - 1;

        char* base = traits_t::allocate(alloc, sizeof(char*) + paddedSize);
        void* p = base + sizeof(char*);
        boost::alignment::align(Alignment, size, p, paddedSize);
        *(static_cast<char**>(p) - 1) = base;
        return static_cast<value_type*>(p);
    }

    void deallocate(value_type* p, size_t n) const {
        typedef typename boost::container::allocator_traits<
            Alloc>::template rebind_traits<char>
            traits_t;
        typename traits_t::allocator_type alloc;

        traits_t::deallocate(
            alloc, *(reinterpret_cast<char**>(p) - 1),
            sizeof(char*) + n * sizeof(value_type) + Alignment - 1);
    }

    friend bool operator==(const libcuckoo_aligned_allocator&,
                           const libcuckoo_aligned_allocator&) {
        return true;
    }

    friend bool operator!=(const libcuckoo_aligned_allocator&,
                           const libcuckoo_aligned_allocator&) {
        return false;
    }
};
#endif // _CUCKOOHASH_UTIL_HH
