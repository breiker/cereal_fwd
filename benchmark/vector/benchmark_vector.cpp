//
// Created by breiker on 6/14/17.
//


#include "benchmark_vector.hpp"
#include <benchmark/benchmark.h>
#include "benchmark_config.hpp"

#include "Vector.pb.h"

#include <boost/serialization/vector.hpp>
#include "cereal/types/vector.hpp"
////////////////
// benchmark
////////////////

template<class BenchArchive, class param = void, param...flags>
void SaveVectorInt(benchmark::State & st)
{
  std::vector<std::int32_t> c;
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

BENCHMARK_TEMPLATE(SaveVectorInt, boost_binary_oarchive, ba::archive_flags, ba::no_header)PARAMS_BENCH_NO_REPEAT_VECT;
BENCHMARK_TEMPLATE(SaveVectorInt, boost_binary_oarchive, ba::archive_flags)PARAMS_BENCH_NO_REPEAT_VECT;
BENCHMARK_TEMPLATE(SaveVectorInt, c::BinaryOutputArchive)PARAMS_BENCH_NO_REPEAT_VECT;
BENCHMARK_TEMPLATE(SaveVectorInt, c::ExtendableBinaryOutputArchive)PARAMS_BENCH_NO_REPEAT_VECT;
BENCHMARK_TEMPLATE(SaveVectorInt, VectorOProtobuf<std::vector<std::int32_t>>)PARAMS_BENCH_NO_REPEAT_VECT;


template<class BenchArchive, class param = void, param...flags>
void SaveVectorFloat(benchmark::State & st)
{
  std::vector<float> c;
  RandomInit<>(st.range(0), &RandomFloatWholeRange, c);
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

BENCHMARK_TEMPLATE(SaveVectorFloat, boost_binary_oarchive, ba::archive_flags, ba::no_header)PARAMS_BENCH_NO_REPEAT_VECT;
BENCHMARK_TEMPLATE(SaveVectorFloat, c::BinaryOutputArchive)PARAMS_BENCH_NO_REPEAT_VECT;
BENCHMARK_TEMPLATE(SaveVectorFloat, c::ExtendableBinaryOutputArchive)PARAMS_BENCH_NO_REPEAT_VECT;
BENCHMARK_TEMPLATE(SaveVectorFloat, VectorOProtobuf<std::vector<float>>)PARAMS_BENCH_NO_REPEAT_VECT;

template<class OArchive, class IArchive, class param = void, param...flags>
void LoadVectorInt(benchmark::State & st)
{

  std::vector<std::int32_t> c;
  RandomInit<>(st.range(0), &RandomIntWholeRange, c);
  std::stringstream ss(std::ios_base::in | std::ios_base::out | std::ios_base::binary);
  {
    OArchive ar(ss, flags...);
    ar << c;
  }
  while (st.KeepRunning()) {
    c.clear();
    ss.clear();
    ss.seekg(0, ss.beg);
    IArchive ar(ss, flags...);
    ar >> c;
    benchmark::DoNotOptimize(c);
  }
}

BENCHMARK_TEMPLATE(LoadVectorInt, boost_binary_oarchive, boost_binary_iarchive, ba::archive_flags,
                   ba::no_header)PARAMS_BENCH_NO_REPEAT_VECT;
BENCHMARK_TEMPLATE(LoadVectorInt, c::BinaryOutputArchive, c::BinaryInputArchive)PARAMS_BENCH_NO_REPEAT_VECT;
BENCHMARK_TEMPLATE(LoadVectorInt, c::ExtendableBinaryOutputArchive,
                   c::ExtendableBinaryInputArchive)PARAMS_BENCH_NO_REPEAT_VECT;
BENCHMARK_TEMPLATE(LoadVectorInt, VectorOProtobuf<std::vector<std::int32_t>>,
                   VectorIProtobuf<std::vector<std::int32_t>>)PARAMS_BENCH_NO_REPEAT_VECT;

template<class OArchive, class IArchive, class param = void, param...flags>
void LoadVectorFloat(benchmark::State & st)
{
  std::vector<float> c;
  RandomInit<>(st.range(0), &RandomFloatWholeRange, c);
  std::stringstream ss(std::ios_base::in | std::ios_base::out | std::ios_base::binary);
  {
    OArchive ar(ss, flags...);
    ar << c;
  }
  while (st.KeepRunning()) {
    c.clear();
    ss.clear();
    ss.seekg(0, ss.beg);
    IArchive ar(ss, flags...);
    ar >> c;
    benchmark::DoNotOptimize(c);
  }
}

BENCHMARK_TEMPLATE(LoadVectorFloat, boost_binary_oarchive, boost_binary_iarchive, ba::archive_flags,
                   ba::no_header)PARAMS_BENCH_NO_REPEAT_VECT;
BENCHMARK_TEMPLATE(LoadVectorFloat, c::BinaryOutputArchive, c::BinaryInputArchive)PARAMS_BENCH_NO_REPEAT_VECT;
BENCHMARK_TEMPLATE(LoadVectorFloat, c::ExtendableBinaryOutputArchive,
                   c::ExtendableBinaryInputArchive)PARAMS_BENCH_NO_REPEAT_VECT;
BENCHMARK_TEMPLATE(LoadVectorFloat, VectorOProtobuf<std::vector<float>>,
                   VectorIProtobuf<std::vector<float>>)PARAMS_BENCH_NO_REPEAT_VECT;


int main(int argc, char **argv)
{
  GOOGLE_PROTOBUF_VERIFY_VERSION;

  ::benchmark::Initialize(&argc, argv);
  if (::benchmark::ReportUnrecognizedArguments(argc, argv)) return 1;

  ::benchmark::RunSpecifiedBenchmarks();
}
