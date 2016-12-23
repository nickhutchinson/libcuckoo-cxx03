#ifndef _DEFAULT_HASHER_HH
#define _DEFAULT_HASHER_HH

#include <boost/core/enable_if.hpp>
#include <boost/functional/hash.hpp>
#include <boost/type_traits/is_integral.hpp>

/*! DefaultHasher is the default hash class used in the table. It overloads a
 *  few types that boost::hash does badly on (namely integers), and falls back to
 *  boost::hash for anything else. */

template <typename Key, typename Enable = void>
class DefaultHasher : public boost::hash<Key> {};

template <class Key>
class DefaultHasher<
    Key, typename boost::enable_if_c<boost::is_integral<Key>::value>::type> {
public:
    size_t operator()(const Key& k) const {
        // This constant is found in the CityHash code
        return k * 0x9ddfea08eb382d69ULL;
    }
};

#endif // _DEFAULT_HASHER_HH
