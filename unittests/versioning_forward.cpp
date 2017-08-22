/*! \file versioning_forward.cpp
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

struct SkippedStruct : public StructBase {
  SkippedStruct(int xx, int yy) : StructBase(xx, yy) {}

  template<class Archive>
  void serialize(Archive& ar)
  {
    ar(x, y);
  }
};

template<class T> inline
typename std::enable_if<std::is_same<T, SkippedStruct>::value, T>::type
random_value(std::mt19937 & gen)
{ return { random_value<int>(gen), random_value<int>(gen) }; }


struct SkippedEmptyStruct {
  template<class Archive>
  void serialize(Archive&)
  { }
};

template<class T> inline
typename std::enable_if<std::is_same<T, SkippedEmptyStruct>::value, T>::type
random_value(std::mt19937 &)
{ return T(); }

template<class T>
struct StructBaseT
{
  StructBaseT() {}
  StructBaseT( int xx, T yy ) : x( xx ), y( yy ) {}
  int x;
  T y;
  bool operator==(StructBase const & other) const
  { return x == other.x && y == other.y; }
  bool operator!=(StructBase const & other) const
  { return x != other.x || y != other.y; }
  bool operator<(StructBase const & other) const
  {
    if (x < other.x) return true;
    else if(other.x < x) return false;
    else return (y < other.y);
  }
};

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
//      ar( inner, x, y );
      ar( inner/*, x, y */);
      ar( /*inner,*/ x/*, y */);
      ar( /*inner, x,*/ y );
    }
};

/** new application */
template<class NewType>
struct InnerNew : public StructBaseT<NewType>
{
    InnerNew() : StructBaseT<NewType>(0, 0) {}
    InnerNew(int x_, const NewType& y_) : StructBaseT<NewType>(x_, y_) {}

    template<class Archive>
    void serialize( Archive & ar, const std::uint32_t version )
    {
      ar( this->x );
      if( version >= 1 )
        ar( this->y );
    }
};

template <class NewType = int>
struct OuterNew
{
    OuterNew() : x(0), y(0) {}
    OuterNew(int x_, int y_, int xi_, const NewType& yi_) : x(x_), y(y_), inner(xi_, yi_) {}
    int x, y;
    InnerNew<NewType> inner;

    template<class Archive>
    void serialize(Archive & ar, const std::uint32_t)
    {
      ar( inner, x, y );
    }
};



template <class IArchive, class OArchive>
void test_backward_support()
{
  std::random_device rd;
  std::mt19937 gen(rd());

  for(int ii=0; ii<100; ++ii)
  {
    OuterOld o_struct = { random_value<int>(gen), random_value<int>(gen) ,
                          random_value<int>(gen) };

    std::ostringstream os;
    {
      OArchive oar(os);
      oar( o_struct );
    }

    OuterNew<int> i_struct;

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

BOOST_AUTO_TEST_CASE( extendable_binary_backward_support )
{
  test_backward_support<cereal::ExtendableBinaryInputArchive, cereal::ExtendableBinaryOutputArchive>();
}

template <class IArchive, class OArchive>
void test_forward_support()
{
  std::random_device rd;
  std::mt19937 gen(rd());

  for(int ii=0; ii<100; ++ii)
  {
    OuterNew<> o_struct = { random_value<int>(gen), random_value<int>(gen) ,
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

BOOST_AUTO_TEST_CASE( xml_forward_support )
{
  test_forward_support<cereal::XMLInputArchive, cereal::XMLOutputArchive>();
}

BOOST_AUTO_TEST_CASE( json_forward_support )
{
  test_forward_support<cereal::JSONInputArchive, cereal::JSONOutputArchive>();
}

BOOST_AUTO_TEST_CASE( extendable_binary_forward_support )
{
  test_forward_support<cereal::ExtendableBinaryInputArchive, cereal::ExtendableBinaryOutputArchive>();
}

template <class IArchive, class OArchive, class NewType>
void test_forward_support_extended()
{
  std::random_device rd;
  std::mt19937 gen(rd());

  for(int ii=0; ii<100; ++ii)
  {
    OuterNew<NewType> o_struct = { random_value<int>(gen), random_value<int>(gen),
                                   random_value<int>(gen), random_value<NewType>(gen)};

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

BOOST_AUTO_TEST_CASE( xml_forward_support_extended )
{
  test_forward_support_extended<cereal::XMLInputArchive, cereal::XMLOutputArchive, int>();
  test_forward_support_extended<cereal::XMLInputArchive, cereal::XMLOutputArchive, float>();
  test_forward_support_extended<cereal::XMLInputArchive, cereal::XMLOutputArchive, double>();
  test_forward_support_extended<cereal::XMLInputArchive, cereal::XMLOutputArchive, bool>();
  test_forward_support_extended<cereal::XMLInputArchive, cereal::XMLOutputArchive, std::uint8_t>();
  test_forward_support_extended<cereal::XMLInputArchive, cereal::XMLOutputArchive, std::uint16_t>();
  test_forward_support_extended<cereal::XMLInputArchive, cereal::XMLOutputArchive, std::uint64_t>();

  test_forward_support_extended<cereal::XMLInputArchive, cereal::XMLOutputArchive, std::string>();
  test_forward_support_extended<cereal::XMLInputArchive, cereal::XMLOutputArchive, std::array<std::uint16_t, 4>>();
}

BOOST_AUTO_TEST_CASE( json_forward_support_extended )
{
  test_forward_support_extended<cereal::JSONInputArchive, cereal::JSONOutputArchive, int>();
  test_forward_support_extended<cereal::JSONInputArchive, cereal::JSONOutputArchive, float>();
  test_forward_support_extended<cereal::JSONInputArchive, cereal::JSONOutputArchive, double>();
  test_forward_support_extended<cereal::JSONInputArchive, cereal::JSONOutputArchive, bool>();
  test_forward_support_extended<cereal::JSONInputArchive, cereal::JSONOutputArchive, std::uint8_t>();
  test_forward_support_extended<cereal::JSONInputArchive, cereal::JSONOutputArchive, std::uint16_t>();
  test_forward_support_extended<cereal::JSONInputArchive, cereal::JSONOutputArchive, std::uint64_t>();

  test_forward_support_extended<cereal::JSONInputArchive, cereal::JSONOutputArchive, std::string>();
  test_forward_support_extended<cereal::JSONInputArchive, cereal::JSONOutputArchive, std::array<std::uint16_t, 4>>();
}

BOOST_AUTO_TEST_CASE( extendable_binary_forward_support_extended )
{
  test_forward_support_extended<cereal::ExtendableBinaryInputArchive, cereal::ExtendableBinaryOutputArchive, int>();
  test_forward_support_extended<cereal::ExtendableBinaryInputArchive, cereal::ExtendableBinaryOutputArchive, float>();
  test_forward_support_extended<cereal::ExtendableBinaryInputArchive, cereal::ExtendableBinaryOutputArchive, double>();
  test_forward_support_extended<cereal::ExtendableBinaryInputArchive, cereal::ExtendableBinaryOutputArchive, bool>();
  test_forward_support_extended<cereal::ExtendableBinaryInputArchive, cereal::ExtendableBinaryOutputArchive, std::uint8_t>();
  test_forward_support_extended<cereal::ExtendableBinaryInputArchive, cereal::ExtendableBinaryOutputArchive, std::uint16_t>();
  test_forward_support_extended<cereal::ExtendableBinaryInputArchive, cereal::ExtendableBinaryOutputArchive, std::uint64_t>();

  test_forward_support_extended<cereal::ExtendableBinaryInputArchive, cereal::ExtendableBinaryOutputArchive, std::string>();
  test_forward_support_extended<cereal::ExtendableBinaryInputArchive, cereal::ExtendableBinaryOutputArchive, std::array<std::uint16_t, 4>>();
  test_forward_support_extended<cereal::ExtendableBinaryInputArchive, cereal::ExtendableBinaryOutputArchive, SkippedStruct>();
  test_forward_support_extended<cereal::ExtendableBinaryInputArchive, cereal::ExtendableBinaryOutputArchive, SkippedEmptyStruct>();
}

BOOST_AUTO_TEST_CASE( portable_binary_forward_support_extended )
{
//  test_forward_support_extended<cereal::PortableBinaryInputArchive, cereal::PortableBinaryOutputArchive, int>(); // fails
//  test_forward_support_extended<cereal::PortableBinaryInputArchive, cereal::PortableBinaryOutputArchive, float>(); // fails
//  test_forward_support_extended<cereal::PortableBinaryInputArchive, cereal::PortableBinaryOutputArchive, bool>(); // failes
//  test_forward_support_extended<cereal::PortableBinaryInputArchive, cereal::PortableBinaryOutputArchive, std::uint8_t>(); // failes
//  test_forward_support_extended<cereal::PortableBinaryInputArchive, cereal::PortableBinaryOutputArchive, std::uint16_t>(); // failes
//  test_forward_support_extended<cereal::PortableBinaryInputArchive, cereal::PortableBinaryOutputArchive, std::uint64_t>(); // failes

//  test_forward_support_extended<cereal::PortableBinaryInputArchive, cereal::PortableBinaryOutputArchive, std::string>(); // failes
//  test_forward_support_extended<cereal::PortableBinaryInputArchive, cereal::PortableBinaryOutputArchive, std::array<std::uint16_t, 4>>(); // failes
}

#define CEREAL_MACRO_COMMA ,
CEREAL_CLASS_VERSION( InnerNew<int>, 1 )
CEREAL_CLASS_VERSION( InnerNew<float>, 1 )
CEREAL_CLASS_VERSION( InnerNew<double>, 1 )
CEREAL_CLASS_VERSION( InnerNew<bool>, 1 )
CEREAL_CLASS_VERSION( InnerNew<std::uint8_t>, 1 )
CEREAL_CLASS_VERSION( InnerNew<std::uint16_t>, 1 )
CEREAL_CLASS_VERSION( InnerNew<std::uint64_t>, 1 )
CEREAL_CLASS_VERSION( InnerNew<std::string>, 1 )
CEREAL_CLASS_VERSION( InnerNew<std::array<std::uint16_t CEREAL_MACRO_COMMA 4>>, 1 )
CEREAL_CLASS_VERSION( InnerNew<SkippedStruct>, 1 )
CEREAL_CLASS_VERSION( InnerNew<SkippedEmptyStruct>, 1 )
#undef CEREAL_MACRO_COMMA
