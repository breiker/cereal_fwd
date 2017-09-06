/*! \file versioning_forward_version_read.cpp
    \brief Test reading class version in forward compatibility situations
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


struct VersionedStruct {
  VersionedStruct() : va(0) {}
  VersionedStruct(float va_) : va(va_) {}

  template<class Archive>
  void serialize(Archive& ar, unsigned int version) {
    BOOST_CHECK_EQUAL(version, 1);
    ar(va);
  }
  bool operator==(VersionedStruct const & other) const
  { return va == other.va; }

  float va;
};

struct OldInnerStruct : public StructBase {
  OldInnerStruct() {}
  OldInnerStruct(int xx, int yy) : StructBase(xx, yy) {}

  template<class Archive>
  void serialize(Archive& ar, unsigned int /*version*/) {
    ar(x, y);
  }
};

struct NewInnerStruct : public StructBase {
  NewInnerStruct() {}
  NewInnerStruct(int xx, int yy, float va) : StructBase(xx, yy), v(va) {}

  template<class Archive>
  void serialize(Archive& ar, unsigned int version) {
    ar(x, y);
    if(version == 1) {
      ar(v);
    }
  }
  bool operator==(OldInnerStruct const & other) const {
    return StructBase::operator==(other);
    // v not compared
  }

  VersionedStruct v;
};

struct OldOuterStruct {
  OldOuterStruct() : a(0) {}
  OldOuterStruct(int a_ , int x_, int y_, float va_) : fst(x_, y_), a(a_), va2(va_)  {}

  template<class Archive>
  void serialize(Archive& ar, unsigned int /*version*/) {
    ar(fst, a, va2);
  }

  OldInnerStruct fst;
  int a;
  VersionedStruct va2;
};

struct NewOuterStruct {
  NewOuterStruct() : a(0) {}
  NewOuterStruct(int a_ , int x_, int y_, float va1_, float va_) : fst(x_, y_, va1_), a(a_), va2(va_) {}

  template<class Archive>
  void serialize(Archive& ar, unsigned int /*version*/) {
    ar(fst, a, va2);
  }
  bool operator==(OldOuterStruct const & other) const
  { return a == other.a && fst == other.fst && va2 == other.va2; }

  NewInnerStruct fst;
  int a;
  VersionedStruct va2;
};

template <class IArchive, class OArchive>
void test_forward_version_read()
{

  std::random_device rd;
  std::mt19937 gen(rd());

  for(int ii=0; ii<100; ++ii)
  {
    NewOuterStruct o_struct = { random_value<int>(gen), random_value<int>(gen) ,
                            random_value<int>(gen), random_value<float>(gen), random_value<float>(gen) };

    std::ostringstream os;
    {
      OArchive oar(os);
      oar( o_struct );
    }

    OldOuterStruct i_struct;

    std::istringstream is(os.str());
    {
      IArchive iar(is);
      iar( i_struct );
    }

//  more specific errors below, can simply check BOOST_CHECK(o_struct == i_struct);
    BOOST_CHECK_EQUAL(o_struct.a, i_struct.a);
    BOOST_CHECK_EQUAL(o_struct.fst.x, i_struct.fst.x);
    BOOST_CHECK_EQUAL(o_struct.fst.y, i_struct.fst.y);
    BOOST_CHECK_EQUAL(o_struct.va2.va, o_struct.va2.va);
  }
}

#if 0 // doesn't work yet
BOOST_AUTO_TEST_CASE( XML_FORWARD_VERSION_READ )
{
  test_forward_version_read<cereal::XMLInputArchive, cereal::XMLOutputArchive>();
}

BOOST_AUTO_TEST_CASE( json_forward_version_read )
{
  test_forward_version_read<cereal::JSONInputArchive, cereal::JSONOutputArchive>();
}
#endif

BOOST_AUTO_TEST_CASE( extendable_binary_forward_version_read )
{
  test_forward_version_read<cereal::ExtendableBinaryInputArchive, cereal::ExtendableBinaryOutputArchive>();
}

CEREAL_CLASS_VERSION( OldInnerStruct, 0 ) // by default
CEREAL_CLASS_VERSION( NewInnerStruct, 1 )

CEREAL_CLASS_VERSION( OldOuterStruct, 0 ) // by default
CEREAL_CLASS_VERSION( NewOuterStruct, 1 )

CEREAL_CLASS_VERSION( VersionedStruct, 1 )
