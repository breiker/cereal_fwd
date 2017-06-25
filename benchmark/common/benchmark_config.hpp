//
// Created by breiker on 6/15/17.
//

#pragma once


#include <boost/serialization/serialization.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>

#include "cereal/cereal.hpp"
#include "cereal/archives/binary.hpp"
#include "cereal/archives/extendable_binary.hpp"

#if THREADED_BENCHMARK
#define THREADED_GBENCHMARK ->Threads(6)
#else
#define THREADED_GBENCHMARK
#endif

#ifdef BENCHMARK_SINGLE_ITERATION
#define GBENCHMARK_ITERATIONS ->Iterations(1)
#else
#define GBENCHMARK_ITERATIONS
#endif

#define PARAMS_BENCH_VECT GBENCHMARK_ITERATIONS->Repetitions(100)->ReportAggregatesOnly(true)THREADED_GBENCHMARK->Range(8, 8<<9)
#define PARAMS_BENCH_NO_REPEAT_VECT GBENCHMARK_ITERATIONS->Repetitions(10)->ReportAggregatesOnly(true)THREADED_GBENCHMARK->Range(8, 8<<10)
#define PARAMS_BENCH GBENCHMARK_ITERATIONS->Repetitions(100)->ReportAggregatesOnly(true)THREADED_GBENCHMARK

namespace ba = boost::archive;
namespace c = cereal;

using boost_binary_oarchive = boost::archive::binary_oarchive;
using boost_binary_iarchive = boost::archive::binary_iarchive;

