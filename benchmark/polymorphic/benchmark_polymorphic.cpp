//
// Created by breiker on 6/16/17.
//

#include "benchmark_polymorphic.hpp"

#include <memory>
#include "benchmark_config.hpp"
#include "utils.hpp"

#include <boost/serialization/unique_ptr.hpp>
#include <boost/serialization/export.hpp>
#include "cereal/types/memory.hpp"


static void CustomArguments(benchmark::internal::Benchmark *b)
{
  for (int i = 0; i < 10; ++i) {
    b->Arg(i);
  }
}

#undef PARAMS_BENCH_NO_REPEAT_VECT
#define PARAMS_BENCH_NO_REPEAT_VECT ->ReportAggregatesOnly(true)THREADED_GBENCHMARK->Apply(CustomArguments)
template<class Archive, class Base, class Derived>
void SaveBaseClass(Archive& ar, Derived* derived);

template<class, class Base, class Derived>
void SaveBaseClass(ba::binary_oarchive& ar, Derived* derived) {
  ar & boost::serialization::base_object<Base>(*derived);
}

template<class, class Base, class Derived>
void SaveBaseClass(ba::binary_iarchive& ar, Derived* derived) {
  ar & boost::serialization::base_object<Base>(*derived);
}

template<class Archive, class Base, class Derived>
void SaveBaseClass(Archive& ar, Derived* derived) {
  ar & cereal::base_class<Base>(derived);
}

struct Node;

std::unique_ptr<Node> ClassForLevel(int level);

struct Node
{
  virtual ~Node() {}

  void add(std::size_t levels)
  {
    if (0 == levels) {
      return;
    }
    --levels;
    left_ = ClassForLevel(levels);
    right_ = ClassForLevel(levels);
    left_->add(levels);
    right_->add(levels);
  }

  std::unique_ptr<Node> left_;
  std::unique_ptr<Node> right_;

  template<class Archive>
  void serialize(Archive & ar, unsigned int/*version*/)
  {
    ar & left_;
    ar & right_;
  }
};

template<int N>
struct NodeP : Node
{
  bool p1 = true;

  template<class Archive>
  void serialize(Archive & ar, unsigned int /*version*/)
  {
    SaveBaseClass<Archive, Node>(ar, this);
    ar & p1;
  }
};

std::unique_ptr<Node> ClassForLevel(int level)
{
  switch (level) {
    case 0:
      return std::unique_ptr<Node>(new NodeP<0>);
    case 1:
      return std::unique_ptr<Node>(new NodeP<1>);
    case 2:
      return std::unique_ptr<Node>(new NodeP<2>);
    case 3:
      return std::unique_ptr<Node>(new NodeP<3>);
    case 4:
      return std::unique_ptr<Node>(new NodeP<4>);
    case 5:
      return std::unique_ptr<Node>(new NodeP<5>);
    case 6:
      return std::unique_ptr<Node>(new NodeP<6>);
    case 7:
      return std::unique_ptr<Node>(new NodeP<7>);
    case 8:
      return std::unique_ptr<Node>(new NodeP<8>);
    case 9:
      return std::unique_ptr<Node>(new NodeP<9>);
    default:
      return std::unique_ptr<Node>(new Node);
  }
}

#define BENCHMARK_REGISTER_TYPE(B, D) \
  CEREAL_REGISTER_TYPE(D) \
  CEREAL_REGISTER_POLYMORPHIC_RELATION(B,D) \
  BOOST_CLASS_EXPORT(D)


BENCHMARK_REGISTER_TYPE(Node, NodeP<0>)
BENCHMARK_REGISTER_TYPE(Node, NodeP<1>)
BENCHMARK_REGISTER_TYPE(Node, NodeP<2>)
BENCHMARK_REGISTER_TYPE(Node, NodeP<3>)
BENCHMARK_REGISTER_TYPE(Node, NodeP<4>)
BENCHMARK_REGISTER_TYPE(Node, NodeP<5>)
BENCHMARK_REGISTER_TYPE(Node, NodeP<6>)
BENCHMARK_REGISTER_TYPE(Node, NodeP<7>)
BENCHMARK_REGISTER_TYPE(Node, NodeP<8>)
BENCHMARK_REGISTER_TYPE(Node, NodeP<9>)

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
