/*! \file portability_golden.cpp
    \brief Golden Master Testing for different architetures and archives
    \ingroup tests */
/*
  Copyright (c) 2016, Randolph Voorhies, Shane Grant, Michal Breiter
  All rights reserved.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions are met:
      * Redistributions of source code must retain the above copyright
        notice, this list of conditions and the following disclaimer.
      * Redistributions in binary form must reproduce the above copyright
        notice, this list of conditions and the following disclaimer in the
        documentation and/or other materials provided with the distribution.
      * Neither the name of cereal nor the
        names of its contributors may be used to endorse or promote products
        derived from this software without specific prior written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
  ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  DISCLAIMED. IN NO EVENT SHALL RANDOLPH VOORHIES AND SHANE GRANT AND MICHAL BREITER BE LIABLE FOR ANY
  DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
  (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
  ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
#include "common.hpp"
#include <boost/test/unit_test.hpp>
#include <fstream>


std::vector<std::uint8_t> get_uint8()
{
  std::vector<std::uint8_t> v;
  v.insert(v.end(), {0x0u, 0x1u});
  return v;
}

std::vector<std::uint16_t> get_uint16()
{
  std::vector<std::uint16_t> v;
  auto smaller = get_uint8();
  v.insert(v.end(), smaller.begin(), smaller.end());
  v.insert(v.end(), {0x201u});
  return v;
}

std::vector<std::uint32_t> get_uint32()
{

  std::vector<std::uint32_t> v;
  auto smaller = get_uint16();
  v.insert(v.end(), smaller.begin(), smaller.end());
  v.insert(v.end(), {0x40201u, 0x8040201u});
  return v;
}

std::vector<std::uint64_t> get_uint64()
{
  std::vector<std::uint64_t> v;
  auto smaller = get_uint32();
  v.insert(v.end(), smaller.begin(), smaller.end());
  v.insert(v.end(), {
      0x7808040201ull,
      0x707808040201ull,
      0x60707808040201ull,
      0x4060707808040201ull
  });
  return v;
}

/* signed */
template<class Signed, class Unsigned>
std::vector<Signed> add_signed(const std::vector<Unsigned> & v)
{
  std::vector<Signed> res;
  res.reserve(v.size() * 2);
  for (const auto & c : v) {
    res.push_back(c);
    res.push_back(-c);
  }
  return res;
}

std::vector<std::int8_t> get_int8()
{
  return add_signed<std::int8_t>(get_uint8());
}

std::vector<std::int16_t> get_int16()
{
  return add_signed<std::int16_t>(get_uint16());
}

std::vector<std::int32_t> get_int32()
{
  return add_signed<std::int32_t>(get_uint32());
}

std::vector<std::int64_t> get_int64()
{
  return add_signed<std::int64_t>(get_uint64());
}

std::vector<std::string> get_string()
{
  return {"", "a", "asdf"};
}

std::vector<std::vector<std::string>> get_vector_string()
{
  return { {}, get_string() };

}

std::vector<std::vector<std::int32_t>> get_vector_int()
{
  return { {}, get_int32()};
}

/* save vector of values */
template<class Archive, class T>
void serialize_vector_separate_impl(Archive & ar, const std::vector<T> & v, std::true_type)
{
  for (const auto & c : v) {
    ar(c);
  }
}
template<class Archive, class T>
void serialize_vector_separate_impl(Archive & ar, const std::vector<std::vector<T>> & orig, std::false_type)
{
  for (const auto & orig_v : orig) {
    std::vector<T> loaded;
    ar(loaded);
    BOOST_CHECK_EQUAL_COLLECTIONS(orig_v.begin(), orig_v.end(), loaded.begin(), loaded.end());
  }
}

template<class Archive, class T>
void serialize_vector_separate_impl(Archive & ar, const std::vector<T> & orig, std::false_type)
{
  for (const auto & orig_v : orig) {
    T loaded;
    ar(loaded);
    BOOST_CHECK_EQUAL(orig_v, loaded);
  }
}


template<class Archive, class T>
void serialize_vector_separate_varint_impl(Archive & ar, const std::vector<T> & v, std::true_type)
{
  for (const auto & c : v) {
    ar.saveVarint(c);
  }
}

template<class Archive, class T>
void serialize_vector_separate_varint_impl(Archive & ar, const std::vector<T> & orig, std::false_type)
{
  for (const auto & orig_v : orig) {
    T loaded;
    ar.loadVarint(loaded);
    BOOST_CHECK_EQUAL(orig_v, loaded);
  }
}

template<class Archive, class T>
void serialize_vector_separate(Archive & ar, const std::vector<T> & v)
{
  serialize_vector_separate_impl(ar, v, typename Archive::is_saving());
}

template<class Archive, class T>
void serialize_vector_separate_varint(Archive & ar, const std::vector<T> & v)
{
  serialize_vector_separate_varint_impl(ar, v, typename Archive::is_saving());
}


/* class */

struct EmptyClass
{
  void set() {}
  bool operator==(const EmptyClass&) const { return true; }
  template<class Archive>
  void serialize(Archive &)
  {}

};

struct EmptyClassV1
{
  void set() {}
  bool operator==(const EmptyClassV1&) const { return true; }
  template<class Archive>
  void serialize(Archive &, unsigned int)
  {}
};

CEREAL_CLASS_VERSION(EmptyClassV1, 1)

struct ElemClass
{
  int a = 0;

  void set() { a = 2; }
  bool operator==(const ElemClass& o) const { return a == o.a; }

  template<class Archive>
  void serialize(Archive & ar)
  {
    ar(a);
  }
};

struct ElemClassV1
{
  int a = 0;

  void set() { a = 3; }
  bool operator==(const ElemClassV1& o) const { return a == o.a; }

  template<class Archive>
  void serialize(Archive & ar, unsigned int)
  {
    ar(a);
  }
};

CEREAL_CLASS_VERSION(ElemClassV1, 1)

template<class T, class Archive>
void serialize_class(Archive& ar) {
  T a;
  if(Archive::is_saving::value) {
    a.set();
  }
  ar & a;
  if(Archive::is_loading::value){
    T a2;
    a2.set();
    BOOST_CHECK(a == a2);
  }
}

/* all */
template<class Archive>
void serialize_all(Archive& ar) {

  serialize_vector_separate_varint(ar, get_uint8());
  serialize_vector_separate_varint(ar, get_uint16());
  serialize_vector_separate_varint(ar, get_uint32());
  serialize_vector_separate_varint(ar, get_uint64());


  serialize_vector_separate(ar, get_uint8());
  serialize_vector_separate(ar, get_uint16());
  serialize_vector_separate(ar, get_uint32());
  serialize_vector_separate(ar, get_uint64());

  serialize_vector_separate(ar, get_int8());
  serialize_vector_separate(ar, get_int16());
  serialize_vector_separate(ar, get_int32());
  serialize_vector_separate(ar, get_int64());

  serialize_class<EmptyClass>(ar);
  serialize_class<EmptyClassV1>(ar);
  serialize_class<ElemClass>(ar);
  serialize_class<ElemClassV1>(ar);

  serialize_vector_separate(ar, get_string());
  serialize_vector_separate(ar, get_vector_int());
  serialize_vector_separate(ar, get_vector_string());
}

template <class IArchive, class OArchive>
void test_golden(typename IArchive::Options const & iOptions, typename OArchive::Options const & oOptions,
                   const std::string & filename, bool save)
{


  if(save) {
    std::ofstream os(filename, std::ios::binary);
    {
      OArchive ar(os, oOptions);
      serialize_all(ar);
    }
    os.close();
  }


  std::ifstream is(filename, std::ios::binary);
  {
    IArchive iar(is, iOptions);
    serialize_all(iar);
  }
}

bool file_exists(const std::string& fileName)
{
    std::ifstream infile(fileName);
    return infile.good();
}

BOOST_AUTO_TEST_CASE( extendable_binary_golden_gen )
{
  std::string host_endian =  cereal::extendable_binary_detail::is_little_endian() ? "hl" : "hb";
  std::string fullname = std::string("al_" + host_endian + ".extendable");
  auto inputOptions = cereal::ExtendableBinaryInputArchive::Options();
  auto outputOptions = cereal::ExtendableBinaryOutputArchive::Options().littleEndian();

  test_golden<cereal::ExtendableBinaryInputArchive, cereal::ExtendableBinaryOutputArchive>(
      inputOptions, outputOptions, fullname, true);
  std::string fullname_be = std::string("ab_" + host_endian + ".extendable");

  outputOptions.bigEndian();

  test_golden<cereal::ExtendableBinaryInputArchive, cereal::ExtendableBinaryOutputArchive>(
      inputOptions, outputOptions, fullname_be, true);

}

BOOST_AUTO_TEST_CASE(extendable_binary_golden_load)
{
  std::vector<std::string> filenames = { "mips32_al_hb", "mips32_ab_hb", "x86_64_al_hl", "x86_64_ab_hl" };
  std::string host_endian = cereal::extendable_binary_detail::is_little_endian() ? "hl" : "hb";
  auto inputOptions = cereal::ExtendableBinaryInputArchive::Options();
  auto not_used = cereal::ExtendableBinaryOutputArchive::Options();
  for (const auto & filename : filenames) {
    std::string full_filename = std::string(GOLDEN_TEST_DATA_PATH) + "/" + filename + ".extendable";
    const auto exists = file_exists(full_filename);
    BOOST_CHECK_MESSAGE(exists, full_filename);
    if (false == exists)
      continue;

    test_golden<cereal::ExtendableBinaryInputArchive, cereal::ExtendableBinaryOutputArchive>(
        inputOptions, not_used, full_filename, false);
  }
}
