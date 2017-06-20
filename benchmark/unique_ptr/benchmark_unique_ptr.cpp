//
// Created by breiker on 6/16/17.
//

#include "benchmark_unique_ptr.hpp"

#include <memory>
#include "benchmark_config.hpp"
#include "utils.hpp"

#include <boost/serialization/unique_ptr.hpp>
#include "cereal/types/memory.hpp"

static void CustomArguments(benchmark::internal::Benchmark *b)
{
  for (int i = 0; i < 10; ++i) {
    b->Arg(i);
  }
}

#undef PARAMS_BENCH_NO_REPEAT_VECT
#define PARAMS_BENCH_NO_REPEAT_VECT ->ReportAggregatesOnly(true)THREADED_GBENCHMARK->Apply(CustomArguments)

class Node
{
  public:
    void add(std::size_t levels)
    {
      if (0 == levels) {
        return;
      }
      --levels;
      left_ = std::unique_ptr<Node>(new Node);
      left_->add(levels);
      right_ = std::unique_ptr<Node>(new Node);
      right_->add(levels);
    }

  public:
    std::unique_ptr<Node> left_;
    std::unique_ptr<Node> right_;
};

class Tree
{
  public:
    void add(std::size_t levels)
    {
      if (0 == levels) {
        return;
      }
      levels_ = levels;
      --levels;
      std::unique_ptr<Node> new_node(new Node);
      root_ = std::move(new_node);
      root_->add(levels);
    }

    std::size_t levels() const
    {
      return levels_;
    }

    std::unique_ptr<Node> root_;
    std::size_t levels_ = 0;
};

template<class Archive>
void serialize(Archive & ar, Node & n, unsigned int/*version*/)
{
  ar & n.left_;
  ar & n.right_;
}

template<class Archive>
void serialize(Archive & ar, Tree & l, unsigned int/*version*/)
{
  ar & l.root_;
}

////////////////
// random
///////////////

inline void RandomInit(std::size_t size, Tree & out)
{
  out.add(size);
}

////////////////
// benchmark
////////////////

template<class BenchArchive, class param = void, param...flags>
void SaveUniquePtr(benchmark::State & st)
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

BENCHMARK_TEMPLATE(SaveUniquePtr, boost_binary_oarchive, ba::archive_flags, ba::no_header)PARAMS_BENCH_NO_REPEAT_VECT;
BENCHMARK_TEMPLATE(SaveUniquePtr, c::BinaryOutputArchive)PARAMS_BENCH_NO_REPEAT_VECT;
BENCHMARK_TEMPLATE(SaveUniquePtr, c::ExtendableBinaryOutputArchive)PARAMS_BENCH_NO_REPEAT_VECT;


template<class OArchive, class IArchive, class param = void, param...flags>
void LoadUniquePtr(benchmark::State & st)
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

BENCHMARK_TEMPLATE(LoadUniquePtr, boost_binary_oarchive, boost_binary_iarchive, ba::archive_flags,
                   ba::no_header)PARAMS_BENCH_NO_REPEAT_VECT;
BENCHMARK_TEMPLATE(LoadUniquePtr, c::BinaryOutputArchive, c::BinaryInputArchive)PARAMS_BENCH_NO_REPEAT_VECT;
BENCHMARK_TEMPLATE(LoadUniquePtr, c::ExtendableBinaryOutputArchive,
                   c::ExtendableBinaryInputArchive)PARAMS_BENCH_NO_REPEAT_VECT;

BENCHMARK_MAIN()
