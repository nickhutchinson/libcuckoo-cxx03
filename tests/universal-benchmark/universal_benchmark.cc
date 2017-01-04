/* Benchmarks a mix of operations for a compile-time specified key-value pair */

#include <stdint.h>

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstdio>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include <boost/array.hpp>
#include <boost/chrono.hpp>
#include <boost/container/vector.hpp>
#include <boost/foreach.hpp>
#include <boost/random.hpp>
#include <boost/random/random_device.hpp>
#include <boost/thread.hpp>

#include <test_util.hh>

#include "universal_gen.hh"
#include "universal_table_wrapper.hh"

#if defined(_MSC_VER) && _MSC_VER < 1900
#define log2(x) (log(static_cast<double>(x)) / log(2.0))
#endif  // defined(_MSC_VER) && _MSC_VER < 1900

/* Run-time parameters -- operation mix and table configuration */

// The following specify what percentage of operations should be of each type.
// They must add up to 100, but by default are all 0.
size_t g_read_percentage = 0;
size_t g_insert_percentage = 0;
size_t g_erase_percentage = 0;
size_t g_update_percentage = 0;
size_t g_upsert_percentage = 0;

// The initial capacity of the table, specified as a power of 2.
size_t g_initial_capacity = 25;
// The percentage of the initial table capacity should we fill the table to
// before running the benchmark.
size_t g_prefill_percentage = 0;
// Total number of operations we are running, specified as a percentage of the
// initial capacity. This can exceed 100.
size_t g_total_ops_percentage = 90;

// Number of threads to run with
size_t g_threads = boost::thread::hardware_concurrency();

// Seed for random number generator. If left at the default (0), we'll generate
// a random seed.
size_t g_seed = 0;

const char* args[] = {
    "--reads",
    "--inserts",
    "--erases",
    "--updates",
    "--upserts",
    "--initial-capacity",
    "--prefill",
    "--total-ops",
    "--num-threads",
    "--seed",
};

size_t* arg_vars[] = {
    &g_read_percentage,
    &g_insert_percentage,
    &g_erase_percentage,
    &g_update_percentage,
    &g_upsert_percentage,
    &g_initial_capacity,
    &g_prefill_percentage,
    &g_total_ops_percentage,
    &g_threads,
    &g_seed,
};

const char* arg_descriptions[] = {
    "Percentage of mix that is reads",
    "Percentage of mix that is inserts",
    "Percentage of mix that is erases",
    "Percentage of mix that is updates",
    "Percentage of mix that is upserts",
    "Initial capacity of table, as a power of 2",
    "Percentage of final size to pre-fill table",
    "Number of operations, as a percentage of the initial capacity. This can exceed 100",
    "Number of threads",
    "Seed for random number generator",
};

#define XSTR(s) STR(s)
#define STR(s) #s

const char* description = "A benchmark that can run an arbitrary mixture of "
    "table operations.\nThe sum of read, insert, erase, update, and upsert "
    "percentages must be 100.\nMap type is " TABLE_TYPE "<" XSTR(KEY)
    ", " XSTR(VALUE) ">."
    ;

void check_percentage(size_t value, const char* name) {
    if (value > 100) {
        std::string msg("Percentage for `");
        msg += name;
        msg += "` cannot exceed 100\n";
        throw std::runtime_error(msg.c_str());
    }
}

enum Ops {
    READ,
    INSERT,
    ERASE,
    UPDATE,
    UPSERT,
};

void genkeys(std::vector<uint64_t>& keys,
             const size_t /*gen_elems*/,
             boost::random::mt19937_64& rng) {
    BOOST_FOREACH(uint64_t& num, keys) {
        num = rng();
    }
}

void prefill_thread(Table& tbl,
                    const std::vector<uint64_t>& keys,
                    const size_t prefill_elems) {
    for (size_t i = 0; i < prefill_elems; ++i) {
        ASSERT_TRUE(tbl.insert(Gen<KEY>::key(keys[i]),
                               Gen<VALUE>::value()));
    }
}

struct NoOp {
    void operator()(...) const {}
};

void mix_thread(Table& tbl,
                const size_t num_ops,
                const boost::array<Ops, 100>& op_mix,
                const std::vector<uint64_t>& keys,
                const size_t prefill_elems) {
    // Invariant: erase_seq <= insert_seq
    // Invariant: insert_seq < numkeys
    const size_t numkeys = keys.size();
    size_t erase_seq = 0;
    size_t insert_seq = prefill_elems;
    // These variables are initialized out here so we don't create new variables
    // in the switch statement.
    size_t n;
    VALUE v;

// A convenience macro for getting the nth key
#define GET_KEY(n) Gen<KEY>::key(keys.at(n))

    // The upsert function is just the identity
    NoOp upsert_fn = NoOp();
    // Use an LCG over the keys array to iterate over the keys in a pseudorandom
    // order, for find operations
    size_t find_seq = 0;
    ASSERT_TRUE(numkeys % 4 == 0 && numkeys > 4);
    const size_t a = numkeys / 2 + 1;
    const size_t c = numkeys / 4 - 1;
    const size_t find_seq_mask = numkeys - 1;
#define FIND_SEQ_UPDATE()                              \
    do {                                               \
        find_seq = (a * find_seq + c) & find_seq_mask; \
    } while (0)

    // Run the operation mix for num_ops operations
    for (size_t i = 0; i < num_ops;) {
        for (size_t j = 0; j < 100 && i < num_ops; ++i, ++j) {
            switch (op_mix[j]) {
            case READ:
                // If `find_seq` is between `erase_seq` and `insert_seq`, then it
                // should be in the table.
                ASSERT_EQ(
                    find_seq >= erase_seq && find_seq < insert_seq,
                    tbl.read(GET_KEY(find_seq), v));
                FIND_SEQ_UPDATE();
                break;
            case INSERT:
                // Insert sequence number `insert_seq`. This should always
                // succeed and be inserting a new value.
                ASSERT_TRUE(
                    tbl.insert(GET_KEY(insert_seq++), Gen<VALUE>::value()));
                break;
            case ERASE:
                // If `erase_seq` == `insert_seq`, the table should be empty, so
                // we pick a random index to unsuccessfully erase. Otherwise we
                // erase `erase_seq`.
                if (erase_seq == insert_seq) {
                    ASSERT_TRUE(!tbl.erase(GET_KEY(find_seq)));
                    FIND_SEQ_UPDATE();
                } else {
                    ASSERT_TRUE(tbl.erase(GET_KEY(erase_seq++)));
                }
                break;
            case UPDATE:
                // Same as find, except we update to the same default value
                ASSERT_EQ(
                    find_seq >= erase_seq && find_seq < insert_seq,
                    tbl.update(GET_KEY(find_seq), Gen<VALUE>::value()));
                FIND_SEQ_UPDATE();
                break;
            case UPSERT:
                // Pick a number from the full distribution, but cap it to the
                // insert_seq, so we don't insert a number greater than
                // insert_seq.
                n = std::max(find_seq, insert_seq);
                FIND_SEQ_UPDATE();
                tbl.upsert(GET_KEY(n), upsert_fn, Gen<VALUE>::value());
                if (n == insert_seq) {
                    ++insert_seq;
                }
                break;
            }
        }
    }
}

template <typename Generator>
struct Shuffler {
    Shuffler(Generator& state) : state_(state) {}

    size_t operator()(size_t i) {
        boost::random::uniform_int_distribution<size_t> rng(0, i - 1);
        return rng(state_);
    }

private:
    Generator& state_;
};

int main(int argc, char** argv) {
    try {
        // Parse parameters and check them.
        parse_flags(argc, argv, description, args, arg_vars, arg_descriptions,
                    sizeof(args)/sizeof(const char*), NULL, NULL, NULL, 0);
        check_percentage(g_read_percentage, "reads");
        check_percentage(g_insert_percentage, "inserts");
        check_percentage(g_erase_percentage, "erases");
        check_percentage(g_update_percentage, "updates");
        check_percentage(g_upsert_percentage, "upserts");
        check_percentage(g_prefill_percentage, "prefill");
        if (g_read_percentage + g_insert_percentage + g_erase_percentage +
            g_update_percentage + g_upsert_percentage != 100) {
            throw std::runtime_error("Operation mix percentages must sum to 100\n");
        }
        if (g_seed == 0) {
            g_seed = boost::random_device()();
        }

        boost::random::mt19937_64 base_rng(g_seed);

        const size_t initial_capacity = 1ULL << g_initial_capacity;
        const size_t total_ops = initial_capacity * g_total_ops_percentage / 100;

        // Pre-generate an operation mix based on our percentages.
        boost::array<Ops, 100> op_mix;
        Ops* op_mix_p = &op_mix[0];
        for (size_t i = 0; i < g_read_percentage; ++i) {
            *op_mix_p++ = READ;
        }
        for (size_t i = 0; i < g_insert_percentage; ++i) {
            *op_mix_p++ = INSERT;
        }
        for (size_t i = 0; i < g_erase_percentage; ++i) {
            *op_mix_p++ = ERASE;
        }
        for (size_t i = 0; i < g_update_percentage; ++i) {
            *op_mix_p++ = UPDATE;
        }
        for (size_t i = 0; i < g_upsert_percentage; ++i) {
            *op_mix_p++ = UPSERT;
        }

        Shuffler<boost::mt19937_64> shuffler(base_rng);
        std::random_shuffle(op_mix.begin(), op_mix.end(), shuffler);

        // Pre-generate all the keys we'd want to insert. In case the insert +
        // upsert percentage is too low, lower bound by the table capacity.
        std::cerr << "Generating keys\n";
        const size_t prefill_elems =
            initial_capacity * g_prefill_percentage / 100;
        const size_t max_insert_ops =
            total_ops *
            (g_insert_percentage + g_upsert_percentage) / 100 +
            g_threads;
        const size_t insert_keys =
            std::max(initial_capacity, max_insert_ops) + prefill_elems;
        // Round this quantity up to a power of 2, so that we can use an LCG to
        // cycle over the array "randomly".
        size_t insert_keys_per_thread = insert_keys / g_threads;
        insert_keys_per_thread = 1ULL << static_cast<size_t>(
            ceil(log2(static_cast<double>(insert_keys_per_thread))));
        std::vector<std::vector<uint64_t> > keys(g_threads);
        for (size_t i = 0; i < g_threads; ++i) {
            keys[i].resize(insert_keys_per_thread);
            genkeys(keys[i], insert_keys_per_thread, base_rng);
        }

        int64_t start_rss = max_rss();

        // Create and size the table
        Table tbl(initial_capacity);

        std::cerr << "Pre-filling table\n";
        boost::container::vector<boost::thread> prefill_threads(g_threads);
        for (size_t i = 0; i < g_threads; ++i) {
            prefill_threads[i] = boost::thread(
                prefill_thread, boost::ref(tbl), boost::ref(keys[i]),
                prefill_elems / g_threads);
        }
        BOOST_FOREACH (boost::thread& t, prefill_threads) {
            t.join();
        }

        // Run the operation mix, timed
        std::cerr << "Running operations\n";
        boost::container::vector<boost::thread> mix_threads(g_threads);
        boost::chrono::high_resolution_clock::time_point start_time =
            boost::chrono::high_resolution_clock::now();
        for (size_t i = 0; i < g_threads; ++i) {
            mix_threads[i] = boost::thread(
                boost::bind(mix_thread, boost::ref(tbl), total_ops / g_threads,
                            boost::ref(op_mix), boost::ref(keys[i]),
                            prefill_elems / g_threads));
        }
        BOOST_FOREACH (boost::thread& t, mix_threads) {
            t.join();
        }
        boost::chrono::high_resolution_clock::time_point end_time =
            boost::chrono::high_resolution_clock::now();
        int64_t end_rss = max_rss();
        double seconds_elapsed = boost::chrono::duration_cast<
            boost::chrono::duration<double> >(end_time - start_time).count();

        // Print out args, preprocessor constants, and results in JSON format
        std::stringstream argstr;
        argstr << args[0] << " " << *arg_vars[0];
        for (size_t i = 1; i < sizeof(args)/sizeof(args[0]); ++i) {
            argstr << " " << args[i] << " " << *arg_vars[i];
        }
        const char* json_format =
            "{\n"
            "    \"args\": \"%s\",\n"
            "    \"key\": \"%s\",\n"
            "    \"value\": \"%s\",\n"
            "    \"table\": \"%s\",\n"
            "    \"output\": {\n"
            "        \"total_ops\": {\n"
            "            \"name\": \"Total Operations\",\n"
            "            \"units\": \"count\",\n"
            "            \"value\": %zu\n"
            "        },\n"
            "        \"time_elapsed\": {\n"
            "            \"name\": \"Time Elapsed\",\n"
            "            \"units\": \"seconds\",\n"
            "            \"value\": %.4f\n"
            "        },\n"
            "        \"throughput\": {\n"
            "            \"name\": \"Throughput\",\n"
            "            \"units\": \"count/seconds\",\n"
            "            \"value\": %.4f\n"
            "        },\n"
            "        \"max_rss_change\": {\n"
            "            \"name\": \"Change in Maximum RSS\",\n"
            "            \"units\": \"bytes\",\n"
            "            \"value\": %ld\n"
            "        }\n"
            "    }\n"
            "}\n";
        printf(json_format, argstr.str().c_str(), XSTR(KEY), XSTR(VALUE),
               TABLE, total_ops, seconds_elapsed,
               total_ops / seconds_elapsed, end_rss - start_rss);
    } catch (const std::exception& e) {
        std::cerr << e.what();
        std::exit(1);
    }
}
