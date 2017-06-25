//
// Created by breiker on 6/25/17.
//

#include "benchmark_polymorphic.hpp"

#include <fstream>

#include "utils.hpp"

#if TEST_SIZE_BOOST
#include <boost/serialization/serialization.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>

#include <boost/serialization/unique_ptr.hpp>
#include <boost/serialization/export.hpp>
using oarchive = boost::archive::binary_oarchive;
using iarchive = boost::archive::binary_iarchive;
#define BENCHMARK_REGISTER_TYPE(B, D) \
  CEREAL_REGISTER_TYPE(D) \
  CEREAL_REGISTER_POLYMORPHIC_RELATION(B,D)

#elif TEST_SIZE_CEREAL_BINARY
#include "cereal/cereal.hpp"
#include "cereal/archives/binary.hpp"

#include "cereal/types/memory.hpp"
using oarchive = cereal::BinaryOutputArchive;
using iarchive = cereal::BinaryInputArchive;
#define BENCHMARK_REGISTER_TYPE(B, D) \
  CEREAL_REGISTER_TYPE(D) \
  CEREAL_REGISTER_POLYMORPHIC_RELATION(B,D)

#elif TEST_SIZE_CEREAL_EXTENDABLE
#include "cereal/cereal.hpp"
#include "cereal/archives/extendable_binary.hpp"

#include "cereal/types/memory.hpp"
using oarchive = cereal::ExtendableBinaryOutputArchive;
using iarchive = cereal::ExtendableBinaryInputArchive;
#define BENCHMARK_REGISTER_TYPE(B, D) \
  CEREAL_REGISTER_TYPE(D) \
  CEREAL_REGISTER_POLYMORPHIC_RELATION(B,D)

#else
#error "unknown test"
#endif


template<class OArchive>
void save()
{
  std::ofstream os("test.bin", std::ios::binary);
  Tree c;
  RandomInit(100, c);
  oarchive ar(os);
  ar << c;
}

template<class IArchive>
void load()
{
  std::ifstream is("test.bin", std::ios::binary);
  Tree c;
  iarchive ar(is);
  ar >> c;
}

int main()
{
  save<oarchive>();
  load<iarchive>();
}
