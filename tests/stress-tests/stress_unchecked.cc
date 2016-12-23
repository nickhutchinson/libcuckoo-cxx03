// Tests all operations and iterators concurrently. It doesn't check any
// operation for correctness, only making sure that everything completes without
// crashing.

#include <stdint.h>

#include <algorithm>
#include <iostream>
#include <limits>
#include <memory>
#include <utility>
#include <vector>

#include <libcuckoo/cuckoohash_config.hh>
#include <libcuckoo/cuckoohash_map.hh>
#include <test_util.hh>

#include <boost/atomic.hpp>
#include <boost/chrono.hpp>
#include <boost/container/vector.hpp>
#include <boost/foreach.hpp>
#include <boost/random.hpp>
#include <boost/thread.hpp>
typedef uint32_t KeyType;
typedef std::string KeyType2;
typedef uint32_t ValType;
typedef int32_t ValType2;

// The number of keys to size the table with, expressed as a power of
// 2. This can be set with the command line flag --power
size_t g_power = 24;
size_t g_numkeys; // Holds 2^power
// The number of threads spawned for each type of operation. This can
// be set with the command line flag --thread-num
size_t g_thread_num = 4;
// Whether to disable inserts or not. This can be set with the command
// line flag --disable-inserts
bool g_disable_inserts = false;
// Whether to disable deletes or not. This can be set with the command
// line flag --disable-deletes
bool g_disable_deletes = false;
// Whether to disable updates or not. This can be set with the command
// line flag --disable-updates
bool g_disable_updates = false;
// Whether to disable finds or not. This can be set with the command
// line flag --disable-finds
bool g_disable_finds = false;
// Whether to disable resizes operations or not. This can be set with
// the command line flag --disable-resizes
bool g_disable_resizes = false;
// Whether to disable iterator operations or not. This can be set with
// the command line flag --disable-iterators
bool g_disable_iterators = false;
// Whether to disable statistic operations or not. This can be set with
// the command line flag --disable-misc
bool g_disable_misc = false;
// Whether to disable clear operations or not. This can be set with
// the command line flag --disable-clears
bool g_disable_clears = false;
// How many seconds to run the test for. This can be set with the
// command line flag --time
size_t g_test_len = 10;
// The seed for the random number generator. If this isn't set to a
// nonzero value with the --seed flag, the current time is used
size_t g_seed = 0;
// Whether to use strings as the key
bool g_use_strings = false;

template <class KType>
class AllEnvironment {
public:
    AllEnvironment() : table(g_numkeys), table2(g_numkeys), finished(false) {
        // Sets up the random number generator
        if (g_seed == 0) {
            g_seed =
                boost::chrono::system_clock::now().time_since_epoch().count();
        }
        std::cout << "seed = " << g_seed << std::endl;
        gen_seed = g_seed;
        // Set minimum load factor and maximum hashpower to unbounded, so weird
        // operations don't throw exceptions.
        table.minimum_load_factor(0.0);
        table2.minimum_load_factor(0.0);
        table.maximum_hashpower(NO_MAXIMUM_HASHPOWER);
        table2.maximum_hashpower(NO_MAXIMUM_HASHPOWER);
    }

    cuckoohash_map<KType, ValType> table;
    cuckoohash_map<KType, ValType2> table2;
    size_t gen_seed;
    boost::atomic<bool> finished;
};

template <class KType>
void stress_insert_thread(AllEnvironment<KType> *env, size_t thread_seed) {
    boost::random::uniform_int_distribution<size_t> ind_dist;
    boost::random::uniform_int_distribution<ValType> val_dist;
    boost::random::uniform_int_distribution<ValType2> val_dist2;
    boost::random::mt19937_64 gen(thread_seed);
    while (!env->finished.load()) {
        // Insert a random key into the table
        KType k = generateKey<KType>(ind_dist(gen));
        ValType v = val_dist(gen);
        env->table.insert(k, v);
        env->table2.insert(k, val_dist2(gen));
    }
}

template <class KType>
void delete_thread(AllEnvironment<KType> *env, size_t thread_seed) {
    boost::random::uniform_int_distribution<size_t> ind_dist;
    boost::random::mt19937_64 gen(thread_seed);
    while (!env->finished.load()) {
        // Run deletes on a random key.
        const KType k = generateKey<KType>(ind_dist(gen));
        env->table.erase(k);
        env->table2.erase(k);
    }
}

template <typename ValType, ValType Delta>
struct UpdateFn {
    void operator()(ValType& v) {
        v += Delta;
    }
};

template <class KType>
void update_thread(AllEnvironment<KType> *env, size_t thread_seed) {
    boost::random::uniform_int_distribution<size_t> ind_dist;
    boost::random::uniform_int_distribution<ValType> val_dist;
    boost::random::uniform_int_distribution<ValType2> val_dist2;
    boost::random::uniform_int_distribution<size_t> third(0, 2);
    boost::random::mt19937_64 gen(thread_seed);
    UpdateFn<ValType, 3> updatefn;
    while (!env->finished.load()) {
        // Run updates, update_funcs, or upserts on a random key.
        const KType k = generateKey<KType>(ind_dist(gen));
        switch (third(gen)) {
        case 0:
            // update
            env->table.update(k, val_dist(gen));
            env->table2.update(k, val_dist2(gen));
            break;
        case 1:
            // update_fn
            env->table.update_fn(k, updatefn);
            env->table2.update_fn(k, UpdateFn<ValType2, 10>());
            break;
        case 2:
            env->table.upsert(k, updatefn, val_dist(gen));
            env->table2.upsert(k, UpdateFn<ValType2, -50>(), val_dist2(gen));
        }
    }
}

template <class KType>
void find_thread(AllEnvironment<KType> *env, size_t thread_seed) {
    boost::random::uniform_int_distribution<size_t> ind_dist;
    boost::random::mt19937_64 gen(thread_seed);
    ValType v;
    while (!env->finished.load()) {
        // Run finds on a random key.
        const KType k = generateKey<KType>(ind_dist(gen));
        env->table.find(k, v);
        try {
            env->table2.find(k);
        } catch (...) {}
    }
}

template <class KType>
void resize_thread(AllEnvironment<KType> *env, size_t thread_seed) {
    boost::random::mt19937_64 gen(thread_seed);
    // Resizes at a random time
    const size_t sleep_time = gen() % g_test_len;
    boost::this_thread::sleep_for(boost::chrono::seconds(sleep_time));
    if (env->finished.load()) {
        return;
    }
    const size_t hashpower = env->table2.hashpower();
    if (gen() & 1) {
        env->table.rehash(hashpower + 1);
        env->table.rehash(hashpower / 2);
    } else {
        env->table2.reserve((1ULL << (hashpower+1)) * DEFAULT_SLOT_PER_BUCKET);
        env->table2.reserve((1ULL << hashpower) * DEFAULT_SLOT_PER_BUCKET);
    }
}

template <class KType>
void iterator_thread(AllEnvironment<KType> *env, size_t thread_seed) {
    boost::random::mt19937_64 gen(thread_seed);
    // Runs an iteration operation at a random time
    const size_t sleep_time = gen() % g_test_len;
    boost::this_thread::sleep_for(boost::chrono::seconds(sleep_time));
    if (env->finished.load()) {
        return;
    }
    typedef cuckoohash_map<KType, ValType2> table_t;
    typename table_t::locked_table lt = env->table2.lock_table();
    BOOST_FOREACH (typename table_t::value_type &item, lt) {
        if (gen() & 1) {
            item.second++;
        }
    }
}

template <class KType>
void misc_thread(AllEnvironment<KType> *env) {
    // Runs all the misc functions
    boost::random::mt19937_64 gen(g_seed);
    while (!env->finished.load()) {
        env->table.size();
        env->table.empty();
        env->table.bucket_count();
        env->table.load_factor();
        env->table.hash_function();
        env->table.key_eq();
    }
}

template <class KType>
void clear_thread(AllEnvironment<KType> *env, size_t thread_seed) {
    boost::random::mt19937_64 gen(thread_seed);
    // Runs a clear operation at a random time
    const size_t sleep_time = gen() % g_test_len;
    boost::this_thread::sleep_for(boost::chrono::seconds(sleep_time));
    if (env->finished.load()) {
        return;
    }
    env->table.clear();
}

// Spawns thread_num threads for each type of operation
template <class KType>
void StressTest(AllEnvironment<KType> *env) {
    boost::container::vector<boost::thread> threads;
    for (size_t i = 0; i < g_thread_num; i++) {
        if (!g_disable_inserts) {
            void (*f)(AllEnvironment<KType> *, size_t) =
                stress_insert_thread<KType>;
            threads.emplace_back(f, env, env->gen_seed++);
        }
        if (!g_disable_deletes) {
            void (*f)(AllEnvironment<KType> *, size_t) = delete_thread<KType>;
            threads.emplace_back(f, env, env->gen_seed++);
        }
        if (!g_disable_updates) {
            void (*f)(AllEnvironment<KType> *, size_t) = update_thread<KType>;
            threads.emplace_back(f, env, env->gen_seed++);
        }
        if (!g_disable_finds) {
            void (*f)(AllEnvironment<KType> *, size_t) = find_thread<KType>;
            threads.emplace_back(f, env, env->gen_seed++);
        }
        if (!g_disable_resizes) {
            void (*f)(AllEnvironment<KType> *, size_t) = resize_thread<KType>;
            threads.emplace_back(f, env, env->gen_seed++);
        }
        if (!g_disable_iterators) {
            void (*f)(AllEnvironment<KType> *, size_t) = iterator_thread<KType>;
            threads.emplace_back(f, env, env->gen_seed++);
        }
        if (!g_disable_misc) {
            void (*f)(AllEnvironment<KType> *) = misc_thread<KType>;
            threads.emplace_back(f, env);
        }
        if (!g_disable_clears) {
            void (*f)(AllEnvironment<KType> *, size_t) = clear_thread<KType>;
            threads.emplace_back(f, env, env->gen_seed++);
        }
    }
    // Sleeps before ending the threads
    boost::this_thread::sleep_for(boost::chrono::seconds(g_test_len));
    env->finished.store(true);
    for (size_t i = 0; i < threads.size(); i++) {
        threads[i].join();
    }
    std::cout << "----------Results----------" << std::endl;
    std::cout << "Final size:\t" << env->table.size() << std::endl;
    std::cout << "Final load factor:\t" << env->table.load_factor() << std::endl;
}

int main(int argc, char** argv) {
#ifdef _WIN32
    win32_disable_error_dialogs();
#endif
    const char* args[] = {"--power", "--thread-num", "--time", "--seed"};
    size_t* arg_vars[] = {&g_power, &g_thread_num, &g_test_len, &g_seed};
    const char* arg_help[] = {
        "The number of keys to size the table with, expressed as a power of 2",
        "The number of threads to spawn for each type of operation",
        "The number of seconds to run the test for",
        "The seed for the random number generator"
    };
    const char* flags[] = {
        "--disable-inserts", "--disable-deletes", "--disable-updates",
        "--disable-finds", "--disable-resizes", "--disable-iterators",
        "--disable-misc", "--disable-clears", "--use-strings"
    };
    bool* flag_vars[] = {&g_disable_inserts, &g_disable_deletes, &g_disable_updates,
                         &g_disable_finds, &g_disable_resizes, &g_disable_iterators,
                         &g_disable_misc, &g_disable_clears, &g_use_strings};
    const char* flag_help[] = {
        "If set, no inserts will be run",
        "If set, no deletes will be run",
        "If set, no updates will be run",
        "If set, no finds will be run",
        "If set, no resize operations will be run",
        "If set, no iterator operations will be run",
        "If set, no misc functions will be run",
        "If set, no clears will be run",
        "If set, the key type of the map will be std::string"
    };
    parse_flags(argc, argv, "Runs a stress test on inserts, deletes, and finds",
                args, arg_vars, arg_help, sizeof(args)/sizeof(const char*),
                flags, flag_vars, flag_help, sizeof(flags)/sizeof(const char*));
    g_numkeys = 1U << g_power;

    if (g_use_strings) {
        AllEnvironment<KeyType2> *env = new AllEnvironment<KeyType2>;
        StressTest(env);
        delete env;
    } else {
        AllEnvironment<KeyType> *env = new AllEnvironment<KeyType>;
        StressTest(env);
        delete env;
    }
    return main_return_value;
}
