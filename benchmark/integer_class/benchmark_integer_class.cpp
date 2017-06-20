//
// Created by breiker on 6/15/17.
//

#include "benchmark_config.hpp"

#include <boost/serialization/vector.hpp>
#include "cereal/types/vector.hpp"

#include "IntegerClass.pb.h"
#include "integer_class.hpp"

namespace ba = boost::archive;
namespace c = cereal;

using boost_binary_oarchive = boost::archive::binary_oarchive;
using boost_binary_iarchive = boost::archive::binary_iarchive;

template<class Archive>
void serialize(Archive & ar, IntegerClass & c, unsigned int/*version*/)
{
  ar & c.f1;
  ar & c.f2;
  ar & c.f3;
  ar & c.f4;
  ar & c.f5;
  ar & c.f6;
  ar & c.f7;
  ar & c.f8;
  ar & c.f9;
  ar & c.f10;

}

template<class Archive>
void serialize(Archive & ar, IntegerClassVect & c, unsigned int/*version*/)
{
  ar & c.v;
}

//////////////////////////
// Protobuf adapters
/////////////////////////
class IntegerOProtobuf
{
  public:
    IntegerOProtobuf(std::ostream & os) : os_(os)
    {}

    IntegerOProtobuf & operator<<(IntegerClass & c)
    {
      ProtoIntegerClass pc;
      pc.set_f1(c.f1);
      pc.set_f2(c.f2);
      pc.set_f3(c.f3);
      pc.set_f4(c.f4);
      pc.set_f5(c.f5);
      pc.set_f6(c.f6);
      pc.set_f7(c.f7);
      pc.set_f8(c.f8);
      pc.set_f9(c.f9);
      pc.SerializeToOstream(&os_);
      return *this;
    }

  private:
    std::ostream & os_;

};

class IntegerIProtobuf
{
  public:
    IntegerIProtobuf(std::istream & is) : is_(is)
    {}

    IntegerIProtobuf & operator>>(IntegerClass & c)
    {
      ProtoIntegerClass pc;
      pc.ParseFromIstream(&is_);
      c.f1 = pc.f1();
      c.f2 = pc.f2();
      c.f3 = pc.f3();
      c.f4 = pc.f4();
      c.f5 = pc.f5();
      c.f6 = pc.f6();
      c.f7 = pc.f7();
      c.f8 = pc.f8();
      c.f9 = pc.f9();
      return *this;
    }

  private:
    std::istream & is_;
};

class IntegerVectOProtobuf
{
  public:
    IntegerVectOProtobuf(std::ostream & os) : os_(os)
    {}

    IntegerVectOProtobuf & operator<<(IntegerClassVect & c)
    {
      ProtoIntegerClassVect pc;
      for (const auto e : c.v) {
        auto pe = pc.add_f1();

        pe->set_f1(e.f1);
        pe->set_f2(e.f2);
        pe->set_f3(e.f3);
        pe->set_f4(e.f4);
        pe->set_f5(e.f5);
        pe->set_f6(e.f6);
        pe->set_f7(e.f7);
        pe->set_f8(e.f8);
        pe->set_f9(e.f9);
        pe->set_f10(e.f10);
      }
      pc.SerializeToOstream(&os_);
      return *this;
    }

  private:
    std::ostream & os_;

};

class IntegerVectIProtobuf
{
  public:
    IntegerVectIProtobuf(std::istream & is) : is_(is)
    {}

    IntegerVectIProtobuf & operator>>(IntegerClassVect & c)
    {
      ProtoIntegerClassVect pc;
      pc.ParseFromIstream(&is_);
      c.v.reserve(pc.f1_size());
      for (int i = 0; i < pc.f1_size(); ++i) {
        auto e = pc.f1(i);
        c.v.emplace_back(IntegerClass{
            e.f1(),
            e.f2(),
            e.f3(),
            e.f4(),
            e.f5(),
            e.f6(),
            e.f7(),
            e.f8(),
            e.f9(),
            e.f10(),
        });
      }

      return *this;
    }

  private:
    std::istream & is_;
};


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