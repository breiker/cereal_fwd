/*! \file versioning_backward.cpp
    \brief Simple tests for reading archives with older application
    \ingroup Tests

    Simple tests for reading archives which has objects saved with newer version
    than one which is reading archive. Binary archive doesn't have any markers
    which would indicate that reading some fields should be skipped.
    As a result fields from OuterClass are read from InnerClass data.
*/
/*
  Copyright (c) 2016, Michal Breiter
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
  DISCLAIMED. IN NO EVENT SHALL RANDOLPH VOORHIES AND SHANE GRANT BE LIABLE FOR ANY
  DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
  (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
  ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
#include "common.hpp"
#include <boost/test/unit_test.hpp>


/** old application */
struct InnerOld : public StructBase
{
    InnerOld() : StructBase{0, 0} {}
    InnerOld(int x_, int y_) : StructBase(x_, y_) {}

    template<class Archive>
    void serialize(Archive & ar, const std::uint32_t )
    {
      ar( x );
    }
};

struct OuterOld
{
    OuterOld() : x(0), y(0) {}
    OuterOld(int x_, int y_, int xi_) : x(x_), y(y_), inner(xi_, 0) {}
    int x, y;
    InnerOld inner;

    template<class Archive>
    void serialize(Archive & ar, const std::uint32_t)
    {
      ar( inner, x, y );
    }
};

/** new application */
struct InnerNew : public StructBase
{
    InnerNew() : StructBase{0, 0} {}
    InnerNew(int x_, int y_) : StructBase(x_, y_) {}

    template<class Archive>
    void serialize( Archive & ar, const std::uint32_t version )
    {
      ar( x );
      if( version >= 1 )
        ar( y );
    }
};

struct OuterNew
{
    OuterNew() : x(0), y(0) {}
    OuterNew(int x_, int y_, int xi_, int yi_) : x(x_), y(y_), inner(xi_, yi_) {}
    int x, y;
    InnerNew inner;

    template<class Archive>
    void serialize(Archive & ar, const std::uint32_t)
    {
      ar( inner, x, y );
    }
};


CEREAL_CLASS_VERSION( InnerNew, 1 )


template <class IArchive, class OArchive>
void test_backward_support()
{
  std::random_device rd;
  std::mt19937 gen(rd());

  for(int ii=0; ii<2/*100*/; ++ii)
  {
    OuterNew o_struct = { random_value<int>(gen), random_value<int>(gen) ,
                          random_value<int>(gen), random_value<int>(gen) };

    std::ostringstream os;
    {
      OArchive oar(os);
      oar( o_struct );
    }

    OuterOld i_struct;

    std::istringstream is(os.str());
    {
      IArchive iar(is);
      iar( i_struct );
    }

    BOOST_CHECK(0 == i_struct.inner.y);
    BOOST_CHECK(o_struct.inner.x == i_struct.inner.x);
    BOOST_CHECK(o_struct.x == i_struct.x);
    BOOST_CHECK(o_struct.y == i_struct.y);
  }
}

BOOST_AUTO_TEST_CASE( xml_backward_support )
{
  test_backward_support<cereal::XMLInputArchive, cereal::XMLOutputArchive>();
}

BOOST_AUTO_TEST_CASE( json_backward_support )
{
  test_backward_support<cereal::JSONInputArchive, cereal::JSONOutputArchive>();
}

BOOST_AUTO_TEST_CASE( binary_backward_support )
{
  test_backward_support<cereal::BinaryInputArchive, cereal::BinaryOutputArchive>();
}

BOOST_AUTO_TEST_CASE( portable_binary_backward_support )
{
  test_backward_support<cereal::PortableBinaryInputArchive, cereal::PortableBinaryOutputArchive>();
}

