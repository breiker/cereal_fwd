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


#define CEREAL_TEST_CHECK_EQUAL_INTEGER                  \
    BOOST_CHECK_EQUAL(i_uint8  , o_uint8);               \
    BOOST_CHECK_EQUAL(i_uint16 , o_uint16);              \
    BOOST_CHECK_EQUAL(i_uint32 , o_uint32);              \
    BOOST_CHECK_EQUAL(i_uint64 , o_uint64);



BOOST_AUTO_TEST_CASE( extendable_archive_varint )
{
  std::random_device rd;
  std::mt19937 gen(rd());

  for(size_t i=0; i<1000; ++i)
  {
    uint8_t  o_uint8  = random_value<uint8_t>(gen);
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

    uint8_t  i_uint8  = 0;
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


#undef CEREAL_TEST_CHECK_EQUAL
