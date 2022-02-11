// Copyright (c) 2015-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef POCKETCOIN_BENCH_BENCH_H
#define POCKETCOIN_BENCH_BENCH_H

#include <chrono>
#include <functional>
#include <map>
#include <string>
#include <vector>

#include <bench/nanobench.h>
#include <boost/preprocessor/cat.hpp>
#include <boost/preprocessor/stringize.hpp>

/*
 * Usage:

static void NameOfYourBenchmarkFunction(benchmark::Bench& bench)
{
    ...do any setup needed...
    bench.run([&] {
         ...do stuff you want to time; refer to src/bench/nanobench.h
            for more information and the options that can be passed here...
    });
    ...do any cleanup needed...
}

BENCHMARK(NameOfYourBenchmarkFunction);

 */

namespace benchmark {

using ankerl::nanobench::Bench;

typedef std::function<void(Bench&)> BenchFunction;

struct Args {
    bool is_list_only;
    std::chrono::milliseconds min_time;
    std::vector<double> asymptote;
    std::string output_csv;
    std::string output_json;
    std::string regex_filter;
};

class BenchRunner
{
    typedef std::map<std::string, BenchFunction> BenchmarkMap;
    static BenchmarkMap& benchmarks();

public:
    BenchRunner(std::string name, BenchFunction func);

    static void RunAll(const Args& args);
};
}
// BENCHMARK(foo) expands to:  benchmark::BenchRunner bench_11foo("foo");
#define BENCHMARK(n) \
    benchmark::BenchRunner BOOST_PP_CAT(bench_, BOOST_PP_CAT(__LINE__, n))(BOOST_PP_STRINGIZE(n), n);

#endif // BITCOIN_BENCH_BENCH_H
