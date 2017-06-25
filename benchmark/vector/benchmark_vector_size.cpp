//
// Created by breiker on 6/25/17.
//

#include "benchmark_vector.hpp"

#include <fstream>

#include "utils.hpp"

#if TEST_SIZE_BOOST
#include <boost/serialization/serialization.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>

#include <boost/serialization/vector.hpp>
using oarchive = boost::archive::binary_oarchive;
using iarchive = boost::archive::binary_iarchive;
#elif TEST_SIZE_CEREAL_BINARY
#include "cereal/cereal.hpp"
#include "cereal/archives/binary.hpp"

#include "cereal/types/vector.hpp"
using oarchive = cereal::BinaryOutputArchive;
using iarchive = cereal::BinaryInputArchive;
#elif TEST_SIZE_CEREAL_EXTENDABLE
#include "cereal/cereal.hpp"
#include "cereal/archives/extendable_binary.hpp"

#include "cereal/types/vector.hpp"
using oarchive = cereal::ExtendableBinaryOutputArchive;
using iarchive = cereal::ExtendableBinaryInputArchive;
#elif TEST_SIZE_PROTOBUF
#include "Vector.pb.h"
using oarchive_int = VectorOProtobuf<std::vector<std::int32_t>>;
using iarchive_int = VectorIProtobuf<std::vector<std::int32_t>>;

using oarchive_float = VectorOProtobuf<std::vector<float>>;
using iarchive_float = VectorIProtobuf<std::vector<float>>;
#else
#error "unknown test"
#endif

#ifndef TEST_SIZE_PROTOBUF
using oarchive_int = oarchive;
using iarchive_int = iarchive;
using oarchive_float = oarchive;
using iarchive_float = iarchive;
#endif

template<class OArchive>
void save_int()
{
  std::ofstream os("test_int.bin", std::ios::binary);
  std::vector<std::int32_t> c;
  RandomInit<>(100, RandomIntWholeRange, c);
  oarchive_int ar(os);
  ar << c;
}

template<class IArchive>
void load_int()
{
  std::ifstream is("test_int.bin", std::ios::binary);
  std::vector<std::int32_t> c;
  iarchive_int ar(is);
  ar >> c;
}

template<class OArchive>
void save_float()
{
  std::ofstream os("test_float.bin", std::ios::binary);
  std::vector<float> c;
  RandomInit<>(100, RandomFloatWholeRange, c);
  oarchive_float ar(os);
  ar << c;
}

template<class IArchive>
void load_float()
{
  std::ifstream is("test_float.bin", std::ios::binary);
  std::vector<float> c;
  iarchive_float ar(is);
  ar >> c;
}

int main()
{
  save_int<oarchive_int>();
  load_int<iarchive_int>();
  save_float<oarchive_float>();
  load_float<iarchive_float>();
}
