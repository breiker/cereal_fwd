//
// Created by breiker on 6/14/17.
//


#include "benchmark_map.hpp"

#include <benchmark/benchmark.h>
#include "benchmark_config.hpp"

#include <boost/serialization/map.hpp>
#include "cereal/types/map.hpp"

#include "Map.pb.h"


////////////////
// benchmark
////////////////

template<class BenchArchive, class param = void, param...flags>
void SaveMapInt(benchmark::State & st)
{
  std::map<std::int32_t, std::int32_t> c;
  RandomInit<>(st.range(0), RandomIntWholeRange, c);
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

BENCHMARK_TEMPLATE(SaveMapInt, boost_binary_oarchive, ba::archive_flags, ba::no_header)PARAMS_BENCH_NO_REPEAT_VECT;
BENCHMARK_TEMPLATE(SaveMapInt, c::BinaryOutputArchive)PARAMS_BENCH_NO_REPEAT_VECT;
BENCHMARK_TEMPLATE(SaveMapInt, c::ExtendableBinaryOutputArchive)PARAMS_BENCH_NO_REPEAT_VECT;
BENCHMARK_TEMPLATE(SaveMapInt, MapOProtobuf<std::map<std::int32_t, std::int32_t>>)PARAMS_BENCH_NO_REPEAT_VECT;



template<class OArchive, class IArchive, class param = void, param...flags>
void LoadMapInt(benchmark::State & st)
{

  std::map<std::int32_t, std::int32_t> c;
  RandomInit<>(st.range(0), &RandomIntWholeRange, c);
  std::stringstream ss(std::ios_base::in | std::ios_base::out | std::ios_base::binary);
  {
    OArchive ar(ss, flags...);
    ar << c;
  }
  while (st.KeepRunning()) {
    ss.clear();
    ss.seekg(0, ss.beg);
    IArchive ar(ss, flags...);
    ar >> c;
    benchmark::DoNotOptimize(c);
  }
}

BENCHMARK_TEMPLATE(LoadMapInt, boost_binary_oarchive, boost_binary_iarchive, ba::archive_flags,
                   ba::no_header)PARAMS_BENCH_NO_REPEAT_VECT;
BENCHMARK_TEMPLATE(LoadMapInt, c::BinaryOutputArchive, c::BinaryInputArchive)PARAMS_BENCH_NO_REPEAT_VECT;
BENCHMARK_TEMPLATE(LoadMapInt, c::ExtendableBinaryOutputArchive,
                   c::ExtendableBinaryInputArchive)PARAMS_BENCH_NO_REPEAT_VECT;
BENCHMARK_TEMPLATE(LoadMapInt, MapOProtobuf<std::map<std::int32_t, std::int32_t>>,
                   MapIProtobuf<std::map<std::int32_t, std::int32_t>>)PARAMS_BENCH_NO_REPEAT_VECT;


int main(int argc, char **argv)
{
  GOOGLE_PROTOBUF_VERIFY_VERSION;

  ::benchmark::Initialize(&argc, argv);
  if (::benchmark::ReportUnrecognizedArguments(argc, argv)) return 1;

  ::benchmark::RunSpecifiedBenchmarks();
}
