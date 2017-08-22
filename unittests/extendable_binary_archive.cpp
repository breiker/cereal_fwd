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


#define CEREAL_TEST_CHECK_EQUAL_INTEGER                  \
    BOOST_CHECK_EQUAL(i_uint8  , o_uint8);               \
    BOOST_CHECK_EQUAL(i_uint16 , o_uint16);              \
    BOOST_CHECK_EQUAL(i_uint32 , o_uint32);              \
    BOOST_CHECK_EQUAL(i_uint64 , o_uint64);


template<class IArchive, class OArchive>
void test_varint_f(typename IArchive::Options const & iOptions, typename OArchive::Options const & oOptions,
                   const std::string & suffix)
{
  std::random_device rd;
  std::mt19937 gen(rd());

  uint8_t o_uint8 = 0x1;
  uint16_t o_uint16 = 0x201;
  uint32_t o_uint32 = 0x8040201;
  uint64_t o_uint64 = 0x80C0E0F008040201;


  std::string host_endian = cereal::extendable_binary_detail::is_little_endian() ? "hl" : "hb";
  std::string filename = std::string("variant_f_") + suffix + "_" + host_endian + ".output";
  std::ofstream os(filename, std::ios::binary);
  {
    OArchive oar(os, oOptions);

    oar.saveVarint(o_uint8);
    oar.saveVarint(o_uint16);
    oar.saveVarint(o_uint32);
    oar.saveVarint(o_uint64);
  }
  os.close();

  uint8_t i_uint8 = 0;
  uint16_t i_uint16 = 0;
  uint32_t i_uint32 = 0;
  uint64_t i_uint64 = 0;

  std::ifstream is(filename, std::ios::binary);
  {
    IArchive iar(is, iOptions);
    iar.loadVarint(i_uint8);
    iar.loadVarint(i_uint16);
    iar.loadVarint(i_uint32);
    iar.loadVarint(i_uint64);
  }

  CEREAL_TEST_CHECK_EQUAL_INTEGER

}

BOOST_AUTO_TEST_CASE(extendable_binary_varint_f_le)
{
  auto inputOptions = cereal::ExtendableBinaryInputArchive::Options();
  auto outputOptions = cereal::ExtendableBinaryOutputArchive::Options().littleEndian();

  test_varint_f<cereal::ExtendableBinaryInputArchive, cereal::ExtendableBinaryOutputArchive>(
      inputOptions, outputOptions, "le");

}

BOOST_AUTO_TEST_CASE(extendable_binary_varint_f_be)
{
  auto inputOptions = cereal::ExtendableBinaryInputArchive::Options();
  auto outputOptions = cereal::ExtendableBinaryOutputArchive::Options().bigEndian();

  test_varint_f<cereal::ExtendableBinaryInputArchive, cereal::ExtendableBinaryOutputArchive>(
      inputOptions, outputOptions, "be");
}

template<class IArchive, class OArchive>
void test_varint(typename IArchive::Options const & iOptions, typename OArchive::Options const & oOptions)
{
  std::random_device rd;
  std::mt19937 gen(rd());

  for (size_t i = 0; i < 1000; ++i) {
    uint8_t o_uint8 = random_value<uint8_t>(gen);
    uint16_t o_uint16 = random_value<uint16_t>(gen);
    uint32_t o_uint32 = random_value<uint32_t>(gen);
    uint64_t o_uint64 = random_value<uint64_t>(gen);


    std::ostringstream os;
    {
      OArchive oar(os, oOptions);

      oar.saveVarint(o_uint8);
      oar.saveVarint(o_uint16);
      oar.saveVarint(o_uint32);
      oar.saveVarint(o_uint64);
    }

    uint8_t i_uint8 = 0;
    uint16_t i_uint16 = 0;
    uint32_t i_uint32 = 0;
    uint64_t i_uint64 = 0;

    std::istringstream is(os.str());
    {
      IArchive iar(is, iOptions);
      iar.loadVarint(i_uint8);
      iar.loadVarint(i_uint16);
      iar.loadVarint(i_uint32);
      iar.loadVarint(i_uint64);
    }

    CEREAL_TEST_CHECK_EQUAL_INTEGER
  }

}

BOOST_AUTO_TEST_CASE(extendable_binary_varint_le)
{
  auto inputOptions = cereal::ExtendableBinaryInputArchive::Options();
  auto outputOptions = cereal::ExtendableBinaryOutputArchive::Options().littleEndian();

  test_varint<cereal::ExtendableBinaryInputArchive, cereal::ExtendableBinaryOutputArchive>(
      inputOptions, outputOptions);
}

BOOST_AUTO_TEST_CASE(extendable_binary_varint_be)
{
  auto inputOptions = cereal::ExtendableBinaryInputArchive::Options();
  auto outputOptions = cereal::ExtendableBinaryOutputArchive::Options().bigEndian();

  test_varint<cereal::ExtendableBinaryInputArchive, cereal::ExtendableBinaryOutputArchive>(
      inputOptions, outputOptions);
}


BOOST_AUTO_TEST_CASE(extendable_archive_varint_min_max)
{
  {
    uint8_t o_uint8 = std::numeric_limits<uint8_t>::max();
    uint16_t o_uint16 = std::numeric_limits<uint8_t>::max();
    uint32_t o_uint32 = std::numeric_limits<uint8_t>::max();
    uint64_t o_uint64 = std::numeric_limits<uint8_t>::max();


    std::ostringstream os;
    {
      cereal::ExtendableBinaryOutputArchive oar(os);

      oar.saveVarint(o_uint8);
      oar.saveVarint(o_uint16);
      oar.saveVarint(o_uint32);
      oar.saveVarint(o_uint64);
    }

    uint8_t i_uint8 = 0;
    uint16_t i_uint16 = 0;
    uint32_t i_uint32 = 0;
    uint64_t i_uint64 = 0;

    std::istringstream is(os.str());
    {
      cereal::ExtendableBinaryInputArchive iar(is);
      iar.loadVarint(i_uint8);
      iar.loadVarint(i_uint16);
      iar.loadVarint(i_uint32);
      iar.loadVarint(i_uint64);
    }

    CEREAL_TEST_CHECK_EQUAL_INTEGER
  }
}

BOOST_AUTO_TEST_CASE(extendable_archive_arithmetic_min_max)
{
  {
    uint8_t o_uint8 = std::numeric_limits<uint8_t>::max();
    uint16_t o_uint16 = std::numeric_limits<uint8_t>::max();
    uint32_t o_uint32 = std::numeric_limits<uint8_t>::max();
    uint64_t o_uint64 = std::numeric_limits<uint8_t>::max();

    int8_t o_int8 = std::numeric_limits<int8_t>::max();
    int16_t o_int16 = std::numeric_limits<int8_t>::max();
    int32_t o_int32 = std::numeric_limits<int8_t>::max();
    int64_t o_int64 = std::numeric_limits<int8_t>::max();

    int8_t o_int8_min = std::numeric_limits<int8_t>::min();
    int16_t o_int16_min = std::numeric_limits<int8_t>::min();
    int32_t o_int32_min = std::numeric_limits<int8_t>::min();
    int64_t o_int64_min = std::numeric_limits<int8_t>::min();

    std::ostringstream os;
    {
      cereal::ExtendableBinaryOutputArchive oar(os);

      oar(o_uint8);
      oar(o_uint16);
      oar(o_uint32);
      oar(o_uint64);

      oar(o_int8);
      oar(o_int16);
      oar(o_int32);
      oar(o_int64);

      oar(o_int8_min);
      oar(o_int16_min);
      oar(o_int32_min);
      oar(o_int64_min);
    }

    uint8_t i_uint8 = 0;
    uint16_t i_uint16 = 0;
    uint32_t i_uint32 = 0;
    uint64_t i_uint64 = 0;

    int8_t i_int8 = 0;
    int16_t i_int16 = 0;
    int32_t i_int32 = 0;
    int64_t i_int64 = 0;

    int8_t i_int8_min = 0;
    int16_t i_int16_min = 0;
    int32_t i_int32_min = 0;
    int64_t i_int64_min = 0;

    std::istringstream is(os.str());
    {
      cereal::ExtendableBinaryInputArchive iar(is);
      iar(i_uint8);
      iar(i_uint16);
      iar(i_uint32);
      iar(i_uint64);

      iar(i_int8);
      iar(i_int16);
      iar(i_int32);
      iar(i_int64);

      iar(i_int8_min);
      iar(i_int16_min);
      iar(i_int32_min);
      iar(i_int64_min);
    }

    CEREAL_TEST_CHECK_EQUAL_INTEGER

    BOOST_CHECK_EQUAL(i_int8, o_int8);
    BOOST_CHECK_EQUAL(i_int16, o_int16);
    BOOST_CHECK_EQUAL(i_int32, o_int32);
    BOOST_CHECK_EQUAL(i_int64, o_int64);

    BOOST_CHECK_EQUAL(i_int8_min, o_int8_min);
    BOOST_CHECK_EQUAL(i_int16_min, o_int16_min);
    BOOST_CHECK_EQUAL(i_int32_min, o_int32_min);
    BOOST_CHECK_EQUAL(i_int64_min, o_int64_min);
  }
}
#undef CEREAL_TEST_CHECK_EQUAL


BOOST_AUTO_TEST_CASE(extendable_binary)
{
  std::random_device rd;
  std::mt19937 gen(rd());

  for (size_t i = 0; i < 1000; ++i) {
    uint8_t o_uint8 = random_value<uint8_t>(gen);
    uint16_t o_uint16 = random_value<uint16_t>(gen);
    uint32_t o_uint32 = random_value<uint32_t>(gen);
    uint64_t o_uint64 = random_value<uint64_t>(gen);


    std::ostringstream os;
    {
      cereal::ExtendableBinaryOutputArchive oar(os);

      oar.saveVarint(o_uint8);
      oar.saveVarint(o_uint16);
      oar.saveVarint(o_uint32);
      oar.saveVarint(o_uint64);
    }

    uint8_t i_uint8 = 0;
    uint16_t i_uint16 = 0;
    uint32_t i_uint32 = 0;
    uint64_t i_uint64 = 0;

    std::istringstream is(os.str());
    {
      cereal::ExtendableBinaryInputArchive iar(is);
      iar.loadVarint(i_uint8);
      iar.loadVarint(i_uint16);
      iar.loadVarint(i_uint32);
      iar.loadVarint(i_uint64);
    }

    CEREAL_TEST_CHECK_EQUAL_INTEGER
  }

}

BOOST_AUTO_TEST_CASE(extendable_archive_float_special_values)
{
  {
    using longdouble = typename cereal::longdouble<cereal::ExtendableBinaryInputArchive>::type;
    static_assert(std::is_same<longdouble, double>::value, "should be same");

    union ufloat
    {
      float f;
      std::uint32_t i;
    };
    union udouble
    {
      double d;
      std::uint64_t i;
    };

    float o_float = std::numeric_limits<float>::max();
    double o_double = std::numeric_limits<double>::max();

    float o_float_min = std::numeric_limits<float>::min();
    double o_double_min = std::numeric_limits<double>::min();

    float o_float_qnan = std::numeric_limits<float>::quiet_NaN();
    double o_double_qnan = std::numeric_limits<double>::quiet_NaN();

    std::ostringstream os;
    {
      cereal::ExtendableBinaryOutputArchive oar(os);

      oar(o_float);
      oar(o_double);

      oar(o_float_min);
      oar(o_double_min);

      oar(o_float_qnan);
      oar(o_double_qnan);
    }

    float i_float = 0;
    double i_double = 0;

    float i_float_min = 0;
    double i_double_min = 0;

    float i_float_qnan = 0;
    double i_double_qnan = 0;


    std::istringstream is(os.str());
    {
      cereal::ExtendableBinaryInputArchive iar(is);
      iar(i_float);
      iar(i_double);

      iar(i_float_min);
      iar(i_double_min);

      iar(i_float_qnan);
      iar(i_double_qnan);
    }

    BOOST_CHECK_EQUAL(i_float, o_float);
    BOOST_CHECK_EQUAL(i_double, o_double);

    BOOST_CHECK_EQUAL(i_float_min, o_float_min);
    BOOST_CHECK_EQUAL(i_double_min, o_double_min);

    BOOST_CHECK_NE(i_float_qnan, o_float_qnan);
    BOOST_CHECK_NE(i_double_qnan, o_double_qnan);

    ufloat i_u_float_qnan, o_u_float_qnan;
    i_u_float_qnan.f = i_float_qnan;
    o_u_float_qnan.f = o_float_qnan;

    udouble i_u_double_qnan, o_u_double_qnan;
    i_u_double_qnan.d = i_double_qnan;
    o_u_double_qnan.d = o_double_qnan;
    BOOST_CHECK_BITWISE_EQUAL(i_u_float_qnan.i, o_u_float_qnan.i);
    BOOST_CHECK_BITWISE_EQUAL(i_u_double_qnan.i, o_u_double_qnan.i);
  }
}


template<class T>
inline void swapBytes(T & t)
{
  cereal::extendable_binary_detail::swap_bytes<sizeof(T)>(reinterpret_cast<std::uint8_t *>(&t));
}

// swaps all output data
#define CEREAL_TEST_SWAP_OUTPUT \
    swapBytes(o_bool);          \
    swapBytes(o_uint8);         \
    swapBytes(o_uint16);        \
    swapBytes(o_uint32);        \
    swapBytes(o_uint64);        \
    swapBytes(o_float);         \
    swapBytes(o_double);

#define CEREAL_TEST_CHECK_EQUAL                          \
    BOOST_CHECK_EQUAL(i_bool   , o_bool);                \
    BOOST_CHECK_EQUAL(i_uint8  , o_uint8);               \
    BOOST_CHECK_EQUAL(i_uint16 , o_uint16);              \
    BOOST_CHECK_EQUAL(i_uint32 , o_uint32);              \
    BOOST_CHECK_EQUAL(i_uint64 , o_uint64);              \
    if( !std::isnan(i_float) && !std::isnan(o_float) ) BOOST_CHECK_CLOSE(i_float  , o_float,  (float)1e-5); \
    if( !std::isnan(i_float) && !std::isnan(o_float) ) BOOST_CHECK_CLOSE(i_double , o_double, 1e-5);

// Last parameter exists to keep everything hidden in options
template<class IArchive, class OArchive>
void test_endian_serialization(typename IArchive::Options const & iOptions, typename OArchive::Options const & oOptions,
                               const std::uint8_t inputLittleEndian)
{
  std::random_device rd;
  std::mt19937 gen(rd());

  BOOST_TEST_CHECKPOINT("input" << inputLittleEndian);
  for (size_t i = 0; i < 1000; ++i) {
    bool o_bool = random_value<uint8_t>(gen) % 2 ? true : false;
    uint8_t o_uint8 = random_value<uint8_t>(gen);
    uint16_t o_uint16 = random_value<uint16_t>(gen);
    uint32_t o_uint32 = random_value<uint32_t>(gen);
    uint64_t o_uint64 = random_value<uint64_t>(gen);
    float o_float = random_value<float>(gen);
    double o_double = random_value<double>(gen);

    std::vector<int32_t> o_vector(100);
    for (auto & elem : o_vector)
      elem = random_value<uint32_t>(gen);

    std::ostringstream os;
    {
      OArchive oar(os, oOptions);
      oar(o_bool);
      oar(o_uint8);
      oar(o_uint16);
      oar(o_uint32);
      oar(o_uint64);
      oar(o_float);
      oar(o_double);
      // We can't test vector directly here since we are artificially interfering with the endianness,
      // which can result in the size being incorrect
      oar(cereal::binary_data(o_vector.data(), static_cast<std::size_t>( o_vector.size() * sizeof(int32_t))));
    }

    bool i_bool = false;
    uint8_t i_uint8 = 0;
    uint16_t i_uint16 = 0;
    uint32_t i_uint32 = 0;
    uint64_t i_uint64 = 0;
    float i_float = 0;
    double i_double = 0;
    std::vector<int32_t> i_vector(100);

    std::istringstream is(os.str());
    {
      IArchive iar(is, iOptions);
      iar(i_bool);
      iar(i_uint8);
      iar(i_uint16);
      iar(i_uint32);
      iar(i_uint64);
      iar(i_float);
      iar(i_double);
      iar(cereal::binary_data(i_vector.data(), static_cast<std::size_t>( i_vector.size() * sizeof(int32_t))));
    }

    // Convert to big endian if we expect to read big and didn't start big
    if (cereal::portable_binary_detail::is_little_endian() ^ inputLittleEndian)
    {
      CEREAL_TEST_SWAP_OUTPUT
      for (auto & val : o_vector)
        swapBytes(val);
    }

    CEREAL_TEST_CHECK_EQUAL

    BOOST_CHECK_EQUAL_COLLECTIONS(i_vector.begin(), i_vector.end(), o_vector.begin(), o_vector.end());
  }
}


BOOST_AUTO_TEST_CASE(extendable_binary_archive_endian_conversions_lb) {
  if (cereal::portable_binary_detail::is_little_endian()) {
    test_endian_serialization<cereal::ExtendableBinaryInputArchive, cereal::ExtendableBinaryOutputArchive>(
        cereal::ExtendableBinaryInputArchive::Options().littleEndian(),
        cereal::ExtendableBinaryOutputArchive::Options().bigEndian(), true);

    test_endian_serialization<cereal::ExtendableBinaryInputArchive, cereal::ExtendableBinaryOutputArchive>(
        cereal::ExtendableBinaryInputArchive::Options().littleEndian(),
        cereal::ExtendableBinaryOutputArchive::Options().littleEndian(), true);
  } else {
    test_endian_serialization<cereal::ExtendableBinaryInputArchive, cereal::ExtendableBinaryOutputArchive>(
        cereal::ExtendableBinaryInputArchive::Options().bigEndian(),
        cereal::ExtendableBinaryOutputArchive::Options().bigEndian(), false);
    test_endian_serialization<cereal::ExtendableBinaryInputArchive, cereal::ExtendableBinaryOutputArchive>(
        cereal::ExtendableBinaryInputArchive::Options().bigEndian(),
        cereal::ExtendableBinaryOutputArchive::Options().littleEndian(), false);
  }
}
