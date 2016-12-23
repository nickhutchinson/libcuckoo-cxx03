// Tests the throughput (queries/sec) of reads and inserts inserts between a
// specific load range in a partially-filled table

#include <stdint.h>
#include <algorithm>
#include <iostream>
#include <limits>
#include <memory>
#include <utility>
#include <vector>

#include <libcuckoo/cuckoohash_map.hh>
#include <test_util.hh>

#include <boost/atomic.hpp>
#include <boost/bind.hpp>
#include <boost/chrono.hpp>
#include <boost/container/vector.hpp>
#include <boost/random.hpp>
#include <boost/random/random_device.hpp>
#include <boost/ref.hpp>
#include <boost/thread.hpp>

typedef uint32_t KeyType;
typedef std::string KeyType2;
typedef uint32_t ValType;

// The number of keys to size the table with, expressed as a power of
// 2. This can be set with the command line flag --power
size_t g_power = 25;
// The initial capacity of the table, expressed as a power of 2. If 0, the table
// is initialized to the number of keys. This can be set with the command line
// flag --table-capacity
size_t g_table_capacity = 0;
// The number of threads spawned for inserts. This can be set with the
// command line flag --thread-num
size_t g_thread_num = boost::thread::hardware_concurrency();
// The load factor to fill the table up to before testing throughput.
// This can be set with the command line flag --begin-load.
size_t g_begin_load = 0;
// The maximum load factor to fill the table up to when testing
// throughput. This can be set with the command line flag
// --end-load.
size_t g_end_load = 90;
// The seed which the random number generator uses. This can be set
// with the command line flag --seed
size_t g_seed = 0;
// The percentage of operations that should be inserts. This should be
// at least 10. This can be set with the command line flag
// --insert-percent
size_t g_insert_percent = 10;
// Whether to use strings as the key
bool use_strings = false;

template <class T>
class ReadInsertEnvironment {
    typedef typename T::key_type KType;
public:
    ReadInsertEnvironment()
        : numkeys(1ULL << g_power),
          table(g_table_capacity ? g_table_capacity : numkeys), keys(numkeys),
          gen(boost::random::random_device()()) {
        // Sets up the random number generator
        if (g_seed == 0) {
            std::cout << "seed = random" << std::endl;
	} else {
            std::cout << "seed = " << g_seed << std::endl;
            gen.seed(g_seed);
	}

        // We fill the keys array with integers between numkeys and
        // 2*numkeys, shuffled randomly
        keys[0] = numkeys;
        for (size_t i = 1; i < numkeys; i++) {
            const size_t swapind = gen() % i;
            keys[i] = keys[swapind];
            keys[swapind] = generateKey<KType>(i+numkeys);
        }

        // We prefill the table to g_begin_load with g_thread_num threads,
        // giving each thread enough keys to insert
        boost::container::vector<boost::thread> threads;
        size_t keys_per_thread = numkeys * (g_begin_load / 100.0) / g_thread_num;
        for (size_t i = 0; i < g_thread_num; i++) {
            threads.emplace_back(
                boost::bind(insert_thread<T>::func, boost::ref(table),
                            keys.begin() + i * keys_per_thread,
                            keys.begin() + (i + 1) * keys_per_thread));
        }
        for (size_t i = 0; i < threads.size(); i++) {
            threads[i].join();
        }

        init_size = table.size();
        ASSERT_TRUE(init_size == keys_per_thread * g_thread_num);

        std::cout << "Table with capacity " << numkeys <<
            " prefilled to a load factor of " << g_begin_load << "%" << std::endl;
    }

    size_t numkeys;
    T table;
    boost::container::vector<KType> keys;
    boost::random::mt19937_64 gen;
    size_t init_size;
};

template <class T>
void ReadInsertThroughputTest(ReadInsertEnvironment<T> *env) {
    const size_t start_seed = (boost::chrono::system_clock::now()
                               .time_since_epoch().count());
    boost::atomic<size_t> counter(0);
    boost::container::vector<boost::thread> threads;
    size_t keys_per_thread = env->numkeys * ((g_end_load-g_begin_load) / 100.0) /
        g_thread_num;
    boost::chrono::steady_clock::time_point t1, t2;
    t1 = boost::chrono::steady_clock::now();
    for (size_t i = 0; i < g_thread_num; i++) {
        threads.emplace_back(boost::bind(
            read_insert_thread<T>::func, boost::ref(env->table),
            env->keys.begin()+(i*keys_per_thread)+env->init_size,
            env->keys.begin()+((i+1)*keys_per_thread)+env->init_size,
            boost::ref(counter),
            (double)g_insert_percent / 100.0, start_seed +i));
    }
    for (size_t i = 0; i < threads.size(); i++) {
        threads[i].join();
    }
    t2 = boost::chrono::steady_clock::now();
    double elapsed_time = boost::chrono::duration<double>(t2 - t1).count();
    // Reports the results
    std::cout << "----------Results----------" << std::endl;
    std::cout << "Final load factor:\t" << g_end_load << "%" << std::endl;
    std::cout << "Number of operations:\t" << counter.load() << std::endl;
    std::cout << "Time elapsed:\t" << elapsed_time/1000 << " seconds"
              << std::endl;
    std::cout << "Throughput: " << std::fixed
              << (double)counter.load() / (elapsed_time/1000)
              << " ops/sec" << std::endl;
}

int main(int argc, char** argv) {
#ifdef _WIN32
    win32_disable_error_dialogs();
#endif
    const char* args[] = {"--power", "--table-capacity", "--thread-num",
                          "--begin-load", "--end-load", "--seed",
                          "--insert-percent"};
    size_t* arg_vars[] = {&g_power, &g_table_capacity, &g_thread_num, &g_begin_load,
                          &g_end_load, &g_seed, &g_insert_percent};
    const char* arg_help[] = {
        "The number of keys to size the table with, expressed as a power of 2",
        "The initial capacity of the table, expressed as a power of 2. "
        "If 0, the table is initialized to the number of keys",
        "The number of threads to spawn for each type of operation",
        "The load factor to fill the table up to before testing throughput",
        "The maximum load factor to fill the table up to when testing "
        "throughput",
        "The seed used by the random number generator",
        "The percentage of operations that should be inserts"
    };
    const char* flags[] = {"--use-strings"};
    bool* flag_vars[] = {&use_strings};
    const char* flag_help[] = {
        "If set, the key type of the map will be std::string"
    };
    parse_flags(argc, argv, "A benchmark for inserts", args, arg_vars, arg_help,
                sizeof(args)/sizeof(const char*), flags, flag_vars, flag_help,
                sizeof(flags)/sizeof(const char*));

    if (g_begin_load >= 100) {
        std::cerr << "--begin-load must be between 0 and 99" << std::endl;
        exit(1);
    } else if (g_begin_load >= g_end_load) {
        std::cerr << "--end-load must be greater than --begin-load"
                  << std::endl;
        exit(1);
    } else if (g_insert_percent < 1 || g_insert_percent > 99) {
        std::cerr << "--insert-percent must be between 1 and 99, inclusive"
                  << std::endl;
        exit(1);
    }

    if (use_strings) {
        ReadInsertEnvironment<cuckoohash_map<KeyType2, ValType> >* env =
            new ReadInsertEnvironment<cuckoohash_map<KeyType2, ValType> >;
        ReadInsertThroughputTest(env);
        delete env;
    } else {
        ReadInsertEnvironment<cuckoohash_map<KeyType, ValType> >* env =
            new ReadInsertEnvironment<cuckoohash_map<KeyType, ValType> >;
        ReadInsertThroughputTest(env);
        delete env;
    }
    return main_return_value;
}
