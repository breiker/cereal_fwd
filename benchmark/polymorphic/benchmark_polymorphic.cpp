//
// Created by breiker on 6/16/17.
//

#include "benchmark_polymorphic.hpp"

#include <benchmark/benchmark.h>
#include "benchmark_config.hpp"

#include <boost/serialization/unique_ptr.hpp>
#include <boost/serialization/export.hpp>
#include "cereal/types/memory.hpp"


#define BENCHMARK_REGISTER_TYPE(B, D) \
  CEREAL_REGISTER_TYPE(D) \
  CEREAL_REGISTER_POLYMORPHIC_RELATION(B,D) \
  BOOST_CLASS_EXPORT(D)

BENCHMARK_REGISTER_TYPES

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
void SavePolymorphic(benchmark::State & st)
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

BENCHMARK_TEMPLATE(SavePolymorphic, boost_binary_oarchive, ba::archive_flags, ba::no_header)PARAMS_BENCH_NO_REPEAT_VECT;
BENCHMARK_TEMPLATE(SavePolymorphic, c::BinaryOutputArchive)PARAMS_BENCH_NO_REPEAT_VECT;
BENCHMARK_TEMPLATE(SavePolymorphic, c::ExtendableBinaryOutputArchive)PARAMS_BENCH_NO_REPEAT_VECT;


template<class OArchive, class IArchive, class param = void, param...flags>
void LoadPolymorphic(benchmark::State & st)
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

BENCHMARK_TEMPLATE(LoadPolymorphic, boost_binary_oarchive, boost_binary_iarchive, ba::archive_flags,
                   ba::no_header)PARAMS_BENCH_NO_REPEAT_VECT;
BENCHMARK_TEMPLATE(LoadPolymorphic, c::BinaryOutputArchive, c::BinaryInputArchive)PARAMS_BENCH_NO_REPEAT_VECT;
BENCHMARK_TEMPLATE(LoadPolymorphic, c::ExtendableBinaryOutputArchive,
                   c::ExtendableBinaryInputArchive)PARAMS_BENCH_NO_REPEAT_VECT;

BENCHMARK_MAIN()
