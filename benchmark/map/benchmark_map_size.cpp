//
// Created by breiker on 6/22/17.
//

#include "benchmark_map.hpp"

#include <fstream>

#include "utils.hpp"

#if TEST_SIZE_BOOST
#include <boost/serialization/serialization.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>

#include <boost/serialization/map.hpp>
using oarchive = boost::archive::binary_oarchive;
using iarchive = boost::archive::binary_iarchive;
#elif TEST_SIZE_CEREAL_BINARY
#include "cereal/cereal.hpp"
#include "cereal/archives/binary.hpp"

#include "cereal/types/map.hpp"
using oarchive = cereal::BinaryOutputArchive;
using iarchive = cereal::BinaryInputArchive;
#elif TEST_SIZE_CEREAL_EXTENDABLE
#include "cereal/cereal.hpp"
#include "cereal/archives/extendable_binary.hpp"

#include "cereal/types/map.hpp"
using oarchive = cereal::ExtendableBinaryOutputArchive;
using iarchive = cereal::ExtendableBinaryInputArchive;
#elif TEST_SIZE_PROTOBUF
#include "Map.pb.h"
using oarchive = MapOProtobuf<std::map<std::int32_t, std::int32_t>>;
using iarchive = MapIProtobuf<std::map<std::int32_t, std::int32_t>>;
#else
#error "unknown test"
#endif


template<class OArchive>
void save()
{
  std::ofstream os("test.bin", std::ios::binary);
  std::map<std::int32_t, std::int32_t> c;
  RandomInit<>(100, RandomIntWholeRange, c);
  oarchive ar(os);
  ar << c;
}

template<class IArchive>
void load()
{
  std::ifstream is("test.bin", std::ios::binary);
  std::map<std::int32_t, std::int32_t> c;
  iarchive ar(is);
  ar >> c;
}

int main()
{
  save<oarchive>();
  load<iarchive>();
}