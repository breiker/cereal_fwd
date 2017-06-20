//
// Created by breiker on 6/14/17.
//


#include <benchmark/benchmark.h>
#include "benchmark_config.hpp"

#include <boost/serialization/vector.hpp>
#include "cereal/types/vector.hpp"

#include "Vector.pb.h"

#include "integer_class.hpp" // TODO delete

namespace ba = boost::archive;
namespace c = cereal;

using boost_binary_oarchive = boost::archive::binary_oarchive;
using boost_binary_iarchive = boost::archive::binary_iarchive;

//////////////////////////
// Protobuf adapters
/////////////////////////

template<>
struct ProtoClass<std::vector<std::int32_t>>
{
  using type = ProtoVectorInt;
};
template<>
struct ProtoClass<std::vector<float>>
{
  using type = ProtoVectorFloat;
};
template<>
struct ProtoClass<std::vector<std::string>>
{
  using type = ProtoVectorInt;
};

template<class T>
class VectorOProtobuf
{
  public:
    VectorOProtobuf(std::ostream & os) : os_(os)
    {}

    VectorOProtobuf & operator<<(const T & c)
    {
      typename ProtoClass<T>::type pc;
      for (const auto e : c) {
        pc.add_f1(e);
      }
      pc.SerializeToOstream(&os_);
      return *this;
    }

  private:
    std::ostream & os_;
};

template<class T>
class VectorIProtobuf
{
  public:
    VectorIProtobuf(std::istream & is) : is_(is)
    {}

    VectorIProtobuf & operator>>(T & c)
    {
      typename ProtoClass<T>::type pc;
      pc.ParseFromIstream(&is_);
      const auto & pcv = pc.f1();
      c.reserve(pc.f1_size());
      for (int i = 0; i < pcv.size(); ++i) {
        c.emplace_back(pc.f1()[i]);
      }
      return *this;
    }

  private:
    std::istream & is_;
};

////////////////
// random
///////////////

template<class T>
inline void RandomInitT(std::size_t size, std::function<void(T&)>& rand, std::vector<T> & out)
{
  out.reserve(size);
  for (std::size_t i = 0; i < size; ++i) {
    out.emplace_back();
    rand(out.back());
  }
}

template<class T>
inline void RandomInit(std::size_t size, std::function<void(std::int32_t&)> rand, std::vector<T> & out)
{
  RandomInitT<>(size, rand, out);
}

template<class T>
inline void RandomInit(std::size_t size, std::function<void(float&)> rand, std::vector<T> & out)
{
  RandomInitT<>(size, rand, out);
}

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
