//
// Created by breiker on 6/16/17.
//

#include "benchmark_shared_ptr.hpp"

#include <benchmark/benchmark.h>
#include "benchmark_config.hpp"

#include <boost/serialization/shared_ptr.hpp>
#include <boost/serialization/weak_ptr.hpp>
#include "cereal/types/memory.hpp"

static void CustomArguments(benchmark::internal::Benchmark *b)
{
  for (int i = 0; i < 10; ++i) {
    b->Arg(i);
  }
}

#undef PARAMS_BENCH_NO_REPEAT_VECT
#define PARAMS_BENCH_NO_REPEAT_VECT GBENCHMARK_ITERATIONS->ReportAggregatesOnly(true)THREADED_GBENCHMARK->Apply(CustomArguments)

////////////////
// benchmark
////////////////

template<class BenchArchive, class param = void, param...flags>
void SaveSharedPtr(benchmark::State & st)
{
  Tree c;
  RandomInit(st.range(0), c);
  std::uint64_t last_size = 0;
  while (st.KeepRunning()) {
    NullStream nullstream;
    {
      BenchArchive ar(nullstream, flags...);
      ar << c;
      benchmark::DoNotOptimize(nullstream);
    }
    last_size = nullstream.bytesSaved();
  }
  st.counters["bytesSaved"] = benchmark::Counter(last_size, benchmark::Counter::kAvgThreads);
}

BENCHMARK_TEMPLATE(SaveSharedPtr, boost_binary_oarchive, ba::archive_flags, ba::no_header)PARAMS_BENCH_NO_REPEAT_VECT;
BENCHMARK_TEMPLATE(SaveSharedPtr, c::BinaryOutputArchive)PARAMS_BENCH_NO_REPEAT_VECT;
BENCHMARK_TEMPLATE(SaveSharedPtr, c::ExtendableBinaryOutputArchive)PARAMS_BENCH_NO_REPEAT_VECT;


template<class OArchive, class IArchive, class param = void, param...flags>
void LoadSharedPtr(benchmark::State & st)
{

  std::stringstream ss(std::ios_base::in | std::ios_base::out | std::ios_base::binary);
  {
    Tree c;
    RandomInit(st.range(0), c);
    OArchive ar(ss, flags...);
    ar << c;
  }
  while (st.KeepRunning()) {
    Tree c;
    ss.clear();
    ss.seekg(0, ss.beg);
    IArchive ar(ss, flags...);
    ar >> c;
    benchmark::DoNotOptimize(c);
  }
}


BENCHMARK_TEMPLATE(LoadSharedPtr, boost_binary_oarchive, boost_binary_iarchive, ba::archive_flags,
    ba::no_header)PARAMS_BENCH_NO_REPEAT_VECT;
BENCHMARK_TEMPLATE(LoadSharedPtr, c::BinaryOutputArchive, c::BinaryInputArchive)PARAMS_BENCH_NO_REPEAT_VECT;
BENCHMARK_TEMPLATE(LoadSharedPtr, c::ExtendableBinaryOutputArchive,
    c::ExtendableBinaryInputArchive)PARAMS_BENCH_NO_REPEAT_VECT;


BENCHMARK_MAIN()
