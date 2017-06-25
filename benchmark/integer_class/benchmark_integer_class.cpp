//
// Created by breiker on 6/15/17.
//

#include "benchmark_integer_class.hpp"

#include <benchmark/benchmark.h>

#include "benchmark_config.hpp"

#include <boost/serialization/vector.hpp>
#include "cereal/types/vector.hpp"

#include "IntegerClass.pb.h"
#include "benchmark_integer_class_protobuf.hpp"



template<class BenchArchive, class param = void, param...flags>
void SaveIntegerClass(benchmark::State & st)
{
  IntegerClass c;
  InitWithRandomWholeRange(c);
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

BENCHMARK_TEMPLATE(SaveIntegerClass, boost_binary_oarchive, ba::archive_flags, ba::no_header)PARAMS_BENCH;
BENCHMARK_TEMPLATE(SaveIntegerClass, boost_binary_oarchive, ba::archive_flags)PARAMS_BENCH;
BENCHMARK_TEMPLATE(SaveIntegerClass, c::BinaryOutputArchive)PARAMS_BENCH;
BENCHMARK_TEMPLATE(SaveIntegerClass, c::ExtendableBinaryOutputArchive)PARAMS_BENCH;
BENCHMARK_TEMPLATE(SaveIntegerClass, IntegerOProtobuf)PARAMS_BENCH;


template<class BenchArchive, class param = void, param...flags>
void SaveIntegerClassSmall(benchmark::State & st)
{
  IntegerClass c;
  InitWithRandomNormalOneByte(c);
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

BENCHMARK_TEMPLATE(SaveIntegerClassSmall, boost_binary_oarchive, ba::archive_flags, ba::no_header)PARAMS_BENCH;
BENCHMARK_TEMPLATE(SaveIntegerClassSmall, c::BinaryOutputArchive)PARAMS_BENCH;
BENCHMARK_TEMPLATE(SaveIntegerClassSmall, c::ExtendableBinaryOutputArchive)PARAMS_BENCH;
BENCHMARK_TEMPLATE(SaveIntegerClassSmall, IntegerOProtobuf)PARAMS_BENCH;

template<class OArchive, class IArchive, class param = void, param...flags>
void LoadIntegerClass(benchmark::State & st)
{
  IntegerClass c;
  InitWithRandomWholeRange(c);
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

BENCHMARK_TEMPLATE(LoadIntegerClass, boost_binary_oarchive, boost_binary_iarchive, ba::archive_flags,
                   ba::no_header)PARAMS_BENCH;
BENCHMARK_TEMPLATE(LoadIntegerClass, c::BinaryOutputArchive, c::BinaryInputArchive)PARAMS_BENCH;
BENCHMARK_TEMPLATE(LoadIntegerClass, c::ExtendableBinaryOutputArchive,
                   c::ExtendableBinaryInputArchive)PARAMS_BENCH;
BENCHMARK_TEMPLATE(LoadIntegerClass, IntegerOProtobuf,
                   IntegerIProtobuf)PARAMS_BENCH;

template<class OArchive, class IArchive, class param = void, param...flags>
void LoadIntegerClassSmall(benchmark::State & st)
{
  IntegerClass c;
  InitWithRandomNormalOneByte(c);
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

BENCHMARK_TEMPLATE(LoadIntegerClassSmall, boost_binary_oarchive, boost_binary_iarchive, ba::archive_flags,
                   ba::no_header)PARAMS_BENCH;
BENCHMARK_TEMPLATE(LoadIntegerClassSmall, c::BinaryOutputArchive, c::BinaryInputArchive)PARAMS_BENCH;
BENCHMARK_TEMPLATE(LoadIntegerClassSmall, c::ExtendableBinaryOutputArchive,
                   c::ExtendableBinaryInputArchive)PARAMS_BENCH;
BENCHMARK_TEMPLATE(LoadIntegerClassSmall, IntegerOProtobuf,
                   IntegerIProtobuf)PARAMS_BENCH;


template<class BenchArchive, class param, param...flags>
void SaveIntegerClassVect(benchmark::State & st)
{
  IntegerClassVect c;
  RandomInit(st.range(0), InitWithRandomWholeRange, c);
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


BENCHMARK_TEMPLATE(SaveIntegerClassVect, boost_binary_oarchive, ba::archive_flags, ba::no_header)PARAMS_BENCH_VECT;
BENCHMARK_TEMPLATE(SaveIntegerClassVect, c::BinaryOutputArchive, void)PARAMS_BENCH_VECT;
BENCHMARK_TEMPLATE(SaveIntegerClassVect, c::ExtendableBinaryOutputArchive, void)PARAMS_BENCH_VECT;
BENCHMARK_TEMPLATE(SaveIntegerClassVect, IntegerVectOProtobuf, void)PARAMS_BENCH_VECT;

template<class OArchive, class IArchive, class param = void, param...flags>
void LoadIntegerClassVect(benchmark::State & st)
{

  IntegerClassVect c;
  RandomInit(st.range(0), InitWithRandomWholeRange, c);
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

BENCHMARK_TEMPLATE(LoadIntegerClassVect, boost_binary_oarchive, boost_binary_iarchive, ba::archive_flags,
                   ba::no_header)PARAMS_BENCH_VECT;
BENCHMARK_TEMPLATE(LoadIntegerClassVect, c::BinaryOutputArchive, c::BinaryInputArchive)PARAMS_BENCH_VECT;
BENCHMARK_TEMPLATE(LoadIntegerClassVect, c::ExtendableBinaryOutputArchive,
                   c::ExtendableBinaryInputArchive)PARAMS_BENCH_VECT;
BENCHMARK_TEMPLATE(LoadIntegerClassVect, IntegerVectOProtobuf,
                   IntegerVectIProtobuf)PARAMS_BENCH_VECT;


int main(int argc, char **argv)
{
  GOOGLE_PROTOBUF_VERIFY_VERSION;

  ::benchmark::Initialize(&argc, argv);
  if (::benchmark::ReportUnrecognizedArguments(argc, argv)) return 1;

  ::benchmark::RunSpecifiedBenchmarks();
}