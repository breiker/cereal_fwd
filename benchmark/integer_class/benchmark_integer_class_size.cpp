//
// Created by breiker on 6/26/17.
//


#include "benchmark_integer_class.hpp"

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
#include "IntegerClass.pb.h"
#include "benchmark_integer_class_protobuf.hpp"
using oarchive = IntegerOProtobuf;
using iarchive = IntegerIProtobuf;

using oarchive_vect = IntegerVectOProtobuf;
using iarchive_vect = IntegerVectIProtobuf;
#else
#error "unknown test"
#endif

#ifndef TEST_SIZE_PROTOBUF
using oarchive_vect = oarchive;
using iarchive_vect = iarchive;
#endif

template<class OArchive>
void save()
{
  std::ofstream os("test_int.bin", std::ios::binary);
  IntegerClass c;
  InitWithRandomWholeRange(c);
  oarchive ar(os);
  ar << c;
}

template<class IArchive>
void load()
{
  std::ifstream is("test_int.bin", std::ios::binary);
  IntegerClass c;
  iarchive ar(is);
  ar >> c;
}

template<class OArchive>
void save_vect()
{
  std::ofstream os("test_vect.bin", std::ios::binary);
  IntegerClassVect c;
  RandomInit(100, InitWithRandomWholeRange, c);
  oarchive_vect ar(os);
  ar << c;
}

template<class IArchive>
void load_vect()
{
  std::ifstream is("test_vect.bin", std::ios::binary);
  IntegerClassVect c;
  iarchive_vect ar(is);
  ar >> c;
}

int main()
{
  save<oarchive>();
  load<iarchive>();
  save_vect<oarchive_vect>();
  load_vect<iarchive_vect>();
}
