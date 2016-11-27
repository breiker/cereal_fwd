/*! \file versioning_omit_shared.cpp
    \brief Tests for reading shared pointer first ommited then read
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


static const int ITERATIONS_PER_TEST = 100;

struct PointerStruct
{
  PointerStruct() : y(0)
  {}

  PointerStruct(int y_) : y(y_)
  {}

  bool operator==(const PointerStruct& other) const
  {
    return y == other.y;
  }

  friend std::ostream& operator<<(std::ostream& os, const PointerStruct& data) {
    return os << data.y;
  }

  template<class Archive>
  void serialize(Archive &ar, const std::uint32_t)
  {
    ar(y);
  }

  int y;
};
/*template<class T> inline
typename std::enable_if<std::is_same<T, std::shared_ptr<PointerStruct>>::value, T>::type
random_value(std::mt19937 & gen)
{ return std::make_shared<T>(PointerStruct(random_value<int>(gen))); }*/

template<class T> inline
typename std::enable_if<std::is_same<T, PointerStruct>::value, T>::type
random_value(std::mt19937 & gen)
{ return PointerStruct(random_value<int>(gen)); }

template<class T> inline
typename std::enable_if<std::is_same<T, std::shared_ptr<typename T::element_type>>::value, T>::type
random_value(std::mt19937 & gen)
{ return std::make_shared<typename T::element_type>(random_value<typename T::element_type>(gen)); }

template<class T, class T2>
struct InnerFirstBase
{
  InnerFirstBase() : x_base(0)
  {}

  InnerFirstBase(int x_, T ptr_, T2 ptr2_) : x_base(x_), ptr(ptr_), ptr2(ptr2_)
  {}

  virtual ~InnerFirstBase() = default;

  int x_base;
  T ptr;
  T2 ptr2;
};

/** old application */
template<class T, class T2>
struct InnerFirstOld : public InnerFirstBase<T, T2>
{
  InnerFirstOld() : InnerFirstBase<T, T2>(), x(0)
  {}

  InnerFirstOld(int x_, int y_, T ptr_, T2 ptr2_) : InnerFirstBase<T, T2>(y_, ptr_, ptr2_), x(x_)
  {}

  template<class Archive>
  void serialize(Archive &ar, const std::uint32_t)
  {
    ar(this->x_base);
    ar(x);
  }
  int x;
};

/** new application */
template<class T, class T2>
struct InnerFirstNew : public InnerFirstBase<T, T2>
{
  using first_type = T;
  using second_type = T2;

  InnerFirstNew() : x(0)
  {}

  InnerFirstNew(int x_, int y_, T ptr_, T2 ptr2_) : InnerFirstBase<T, T2>(y_, ptr_, ptr2_), x(x_)
  {}

  template<class Archive>
  void serialize(Archive &ar, const std::uint32_t version)
  {
    ar(this->x_base);
    ar(x);
    if (version >= 1) {
      std::cout << "saving shared_ptr" << std::endl;
      ar(this->ptr);
      ar(x);
      ar(this->ptr2);
    }

  }

  int x;
};


template<class T, class T2>
struct OuterOld
{
  OuterOld() : x(0), z(0)
  {}

  OuterOld(int x_, int y_, int z_, int xi_, T ptr_, T2 ptr2_) : x(x_), z(z_),
                                                                inner(xi_, y_, ptr_, ptr2_),
                                                                ptr2(ptr_), ptr3(ptr_),
                                                                ptr2_2(ptr2_), ptr2_3(ptr2_)
  {}

  int x;
  int z;
  InnerFirstOld<T, T2> inner;
  T ptr2;
  T ptr3;
  T2 ptr2_2;
  T2 ptr2_3;

  template<class Archive>
  void serialize(Archive &ar, const std::uint32_t)
  {
    ar(inner, x, ptr2, ptr3, z, ptr2_2, ptr2_3, z);
  }
};


template<class T, class T2>
struct OuterNew
{
  OuterNew() : x(0), z(0)
  {}

  OuterNew(int x_, int y_, int z_, int xi_, T ptr_, T2 ptr2_) : x(x_), z(z_),
                                                                inner(xi_, y_, ptr_, ptr2_),
                                                                ptr2(ptr_), ptr3(ptr_),
                                                                ptr2_2(ptr2_), ptr2_3(ptr2_)
  {}

  int x;
  int z;
  InnerFirstNew<T, T2> inner;
  T ptr2;
  T ptr3;
  T2 ptr2_2;
  T2 ptr2_3;

  template<class Archive>
  void serialize(Archive &ar, const std::uint32_t)
  {
    ar(inner, x, ptr2, ptr3, z, ptr2_2, ptr2_3, z);
  }
};




template <class IArchive, class OArchive, class T>
void test_backward_support()
{
  using T1 = typename T::first_type;
  using T2 = typename T::second_type;

  std::random_device rd;
  std::mt19937 gen(rd());

  for(int ii=0; ii < ITERATIONS_PER_TEST; ++ii)
  {
    OuterNew<T1, T2> o_struct = {random_value<int>(gen), random_value<int>(gen),
                            random_value<int>(gen), random_value<int>(gen),
                            random_value<T1>(gen), random_value<T2>(gen)};

    std::ostringstream os;
    {
      OArchive oar(os);
      oar( o_struct );
    }

    OuterOld<T1, T2> i_struct;

    std::istringstream is(os.str());
    {
      IArchive iar(is);
      iar( i_struct );
    }

    BOOST_CHECK_EQUAL(o_struct.inner.x, i_struct.inner.x);
    BOOST_CHECK_EQUAL(o_struct.x, i_struct.x);
    BOOST_CHECK_EQUAL(o_struct.z, i_struct.z);
    BOOST_CHECK(o_struct.inner.ptr != nullptr);

    BOOST_CHECK_EQUAL(o_struct.inner.ptr , o_struct.ptr2);
    BOOST_CHECK_EQUAL(*(o_struct.ptr2) , *(i_struct.ptr2));
    BOOST_CHECK_EQUAL(*(o_struct.ptr3) , *(i_struct.ptr3));
    BOOST_CHECK_EQUAL(*(o_struct.inner.ptr) , *(i_struct.ptr2));
    BOOST_CHECK_EQUAL(i_struct.ptr2, i_struct.ptr3);

    BOOST_CHECK_EQUAL(o_struct.inner.ptr2 , o_struct.ptr2_2);
    BOOST_CHECK_EQUAL(*(o_struct.ptr2_2) , *(i_struct.ptr2_2));
    BOOST_CHECK_EQUAL(*(o_struct.ptr2_3) , *(i_struct.ptr2_3));
    BOOST_CHECK_EQUAL(*(o_struct.inner.ptr2) , *(i_struct.ptr2_2));
    BOOST_CHECK_EQUAL(i_struct.ptr2_2, i_struct.ptr2_3);
  }
}

using test1_type = InnerFirstNew<std::shared_ptr<int>, std::shared_ptr<int>>;
using test2_type = InnerFirstNew<std::shared_ptr<PointerStruct>, std::shared_ptr<int>>;

CEREAL_CLASS_VERSION( test1_type, 1 )
CEREAL_CLASS_VERSION( test2_type, 1 )

BOOST_AUTO_TEST_CASE( extendable_binary_backward_support )
{
  test_backward_support<cereal::ExtendableBinaryInputArchive, cereal::ExtendableBinaryOutputArchive, test1_type>();
  test_backward_support<cereal::ExtendableBinaryInputArchive, cereal::ExtendableBinaryOutputArchive, test2_type>();
}
struct D {
  int d;

  template<class Archive>
  void serialize(Archive &ar, const std::uint32_t)
  {
    ar(d);
  }
  bool operator==(const D& other) const { return d == other.d; }
};
struct C {
  std::shared_ptr<D> toD;

  template<class Archive>
  void serialize(Archive &ar, const std::uint32_t)
  {
    ar(toD);
  }
  bool operator==(const C& other) const { return (*toD) == (*other.toD); }
};
struct E {
  int e;
  std::shared_ptr<D> toD;
  std::shared_ptr<C> toC;

  template<class Archive>
  void serialize(Archive &ar, const std::uint32_t)
  {
    ar(e, toD, toC);
  }
  bool operator==(const E& other) const
  { return e == other.e && (*toD) == (*other.toD) && (*toC) == (*other.toC); }
};

struct BOld {
  int b;
  std::shared_ptr<C> toC;

  template<class Archive>
  void serialize(Archive &ar, const std::uint32_t)
  {
    ar(b);
  }
};

struct BNew {
  int b;
  std::shared_ptr<C> toC;

  template<class Archive>
  void serialize(Archive &ar, const std::uint32_t version)
  {
    ar(b);
    if(version >= 1)
      ar(toC);
  }
};

struct AOld {
  BOld b;
  E e;
  template<class Archive>
  void serialize(Archive &ar, const std::uint32_t)
  {
    ar(b, e);
  }
};

struct ANew {
  BNew b;
  E e;
  template<class Archive>
  void serialize(Archive &ar, const std::uint32_t)
  {
    ar(b, e);
  }
};
template <class IArchive, class OArchive>
void test_omited_shared_out_of_order()
{

  std::random_device rd;
  std::mt19937 gen(rd());

  for(int ii=0; ii < ITERATIONS_PER_TEST; ++ii)
  {
    auto toD = std::shared_ptr<D>(new D{random_value<int>(gen)});
    auto toC = std::shared_ptr<C>(new C{toD});
    ANew o_struct = {BNew{random_value<int>(gen), toC}, E{random_value<int>(gen), toD, toC}};

    std::ostringstream os;
    {
      OArchive oar(os);
      oar( o_struct );
    }

    AOld i_struct;

    std::istringstream is(os.str());
    {
      IArchive iar(is);
      iar( i_struct );
    }

    BOOST_CHECK_EQUAL(o_struct.b.b, i_struct.b.b);
    BOOST_CHECK_EQUAL(o_struct.e.e, i_struct.e.e);
    BOOST_CHECK(*(o_struct.e.toD) == *(i_struct.e.toD));
    BOOST_CHECK(*(o_struct.e.toC) == *(i_struct.e.toC));
    BOOST_CHECK(*(o_struct.b.toC) == *(i_struct.e.toC));
    BOOST_CHECK(*(o_struct.b.toC->toD) == *(i_struct.e.toD));
    BOOST_CHECK_EQUAL(i_struct.e.toC->toD, i_struct.e.toD); // address


  }
}

CEREAL_CLASS_VERSION( BNew, 1 )
CEREAL_CLASS_VERSION( ANew, 1 )

BOOST_AUTO_TEST_CASE( extendable_binary_omited_shared_out_of_order )
{
  test_omited_shared_out_of_order<cereal::ExtendableBinaryInputArchive, cereal::ExtendableBinaryOutputArchive>();
}

struct Version0
{
  static constexpr std::uint32_t version = 0;
};
struct Version1
{
  static constexpr std::uint32_t version = 1;
};

struct Z
{
  int z;

  bool operator==(const Z & other) const
  { return z == other.z; }

  template<class Archive>
  void serialize(Archive & ar, std::uint32_t)
  {
    ar(z);
  }
};

struct Q
{
  std::shared_ptr<Z> toZ;
  int q;

  bool operator==(const Q & other) const
  {
    return q == other.q &&
           (
               ((toZ != nullptr && other.toZ != nullptr) && (*toZ == *other.toZ))
               ||
               (toZ == nullptr && other.toZ == nullptr)
           );
  }

  template<class Archive>
  void serialize(Archive & ar, std::uint32_t)
  {
    ar(toZ, q);
  }
};

struct V
{
  std::shared_ptr<Z> toZ;
  int v;

  bool operator==(const V & other) const
  {
    return v == other.v &&
           (
               ((toZ != nullptr && other.toZ != nullptr) && (*toZ == *other.toZ))
               ||
               (toZ == nullptr && other.toZ == nullptr)
           );
  }
  template<class Archive>
  void serialize(Archive & ar, std::uint32_t)
  {
    ar(toZ, v);
  }
};

/*
* #Q {     - 1
* #    Z { - 2
* #    }   - 3
* #}       - 4
* V {      - 5
*      Z @2- 6
* }        - 7
* Q @1     - 8
*
* 2-3       ->2-3
* 5-7  5-7    1-4  1-4
* V    V{Z}    Q   Q{Z}
*/

template<class Version>
struct Inner1
{
  int x;
  std::shared_ptr<Q> toQ;
  int y;

  template<class Archive>
  void serialize(Archive & ar, std::uint32_t version)
  {
    ar(x);
    if (Version::version >= 1 && version >= 1) {
      ar(toQ);
      ar(y);
    }

  }
};

template<class Version>
struct Out1
{
  Inner1<Version> inner;
  std::shared_ptr<V> toV;
  std::shared_ptr<Q> toQ;
  std::shared_ptr<V> toV2;
  std::shared_ptr<Q> toQ2;

  template<class Archive>
  void serialize(Archive & ar, std::uint32_t)
  {
    ar(inner, toV, toQ, toV2, toQ2);
  }
};

CEREAL_CLASS_VERSION(Inner1<Version1>, 1)

template<class IArchive, class OArchive>
void test_omited_shared_out_of_order_1()
{

  std::random_device rd;
  std::mt19937 gen(rd());

  for (int ii = 0; ii < ITERATIONS_PER_TEST; ++ii) {
    auto toZ = std::shared_ptr<Z>(new Z{random_value<int>(gen)});
    auto toQ = std::shared_ptr<Q>(new Q{toZ, random_value<int>(gen)});
    auto toV = std::shared_ptr<V>(new V{toZ, random_value<int>(gen)});
    Out1<Version1> o_struct = {Inner1<Version1>{random_value<int>(gen), toQ, random_value<int>(gen)}, toV, toQ, toV, toQ};

    std::ostringstream os;
    {
      OArchive oar(os);
      oar(o_struct);
    }

    Out1<Version0> i_struct;

    std::istringstream is(os.str());
    {
      IArchive iar(is);
      iar(i_struct);
    }

    BOOST_CHECK(*o_struct.toV == *i_struct.toV);
    BOOST_CHECK(*o_struct.toQ == *i_struct.toQ);
    BOOST_CHECK_EQUAL(o_struct.inner.x, i_struct.inner.x);
    // addresses
    BOOST_REQUIRE(i_struct.toQ != nullptr);
    BOOST_REQUIRE(i_struct.toV != nullptr);
    BOOST_REQUIRE(i_struct.toV->toZ != nullptr);
    BOOST_REQUIRE(i_struct.toQ->toZ != nullptr);
    BOOST_CHECK_EQUAL(i_struct.toV->toZ, i_struct.toQ->toZ);
    BOOST_CHECK_EQUAL(i_struct.toQ, i_struct.toQ2);
    BOOST_CHECK_EQUAL(i_struct.toV, i_struct.toV2);

    BOOST_CHECK(*o_struct.toQ == *i_struct.toQ);
    BOOST_CHECK(*o_struct.toV == *i_struct.toV);
  };
}

BOOST_AUTO_TEST_CASE( extendable_binary_omited_shared_out_of_order_1 )
{
  test_omited_shared_out_of_order_1<cereal::ExtendableBinaryInputArchive, cereal::ExtendableBinaryOutputArchive>();
}

/*
* #Q {     - 1
* #    Z { - 2
* #    }   - 3
* #}       - 4
*  Q @1    - 5
*
*      m2-3      ->2-3
* 1-4  1-4  1-4   1-4
*  Q   Q{Z}  Q    Q{Z}
*/

template<class Version>
struct Inner2
{
  int x;
  std::shared_ptr<Q> toQ;
  int y;

  template<class Archive>
  void serialize(Archive & ar, std::uint32_t version)
  {
    ar(x);
    if (Version::version >= 1 && version >= 1) {
      ar(toQ);
      ar(y);
    }

  }
};

template<class Version>
struct Out2
{
  Inner2<Version> inner;
  std::shared_ptr<Q> toQ;
  std::shared_ptr<Q> toQ2;

  template<class Archive>
  void serialize(Archive & ar, std::uint32_t)
  {
    ar(inner, toQ, toQ2);
  }
};

CEREAL_CLASS_VERSION(Inner2<Version1>, 1)

template<class IArchive, class OArchive>
void test_omited_shared_out_of_order_2()
{

  std::random_device rd;
  std::mt19937 gen(rd());

  for (int ii = 0; ii < ITERATIONS_PER_TEST; ++ii) {
    auto toZ = std::shared_ptr<Z>(new Z{random_value<int>(gen)});
    auto toQ = std::shared_ptr<Q>(new Q{toZ, random_value<int>(gen)});
    Out2<Version1> o_struct = {Inner2<Version1>{random_value<int>(gen), toQ, random_value<int>(gen)}, toQ, toQ};

    std::ostringstream os;
    {
      OArchive oar(os);
      oar(o_struct);
    }

    Out2<Version0> i_struct;

    std::istringstream is(os.str());
    {
      IArchive iar(is);
      iar(i_struct);
    }

    BOOST_CHECK(*o_struct.toQ == *i_struct.toQ);
    BOOST_CHECK_EQUAL(o_struct.inner.x, i_struct.inner.x);
    // addresses
    BOOST_REQUIRE(i_struct.toQ != nullptr);
    BOOST_REQUIRE(i_struct.toQ->toZ != nullptr);

    BOOST_CHECK(*o_struct.toQ == *i_struct.toQ);
    BOOST_CHECK_EQUAL(i_struct.toQ, i_struct.toQ2);
  };
}

BOOST_AUTO_TEST_CASE( extendable_binary_omited_shared_out_of_order_2 )
{
  test_omited_shared_out_of_order_2<cereal::ExtendableBinaryInputArchive, cereal::ExtendableBinaryOutputArchive>();
}


/*
* #Q {     - 1
* #    Z { - 2
* #    }   - 3
* #}       - 4
*  Z @ 2
*  Q @ 1
*/

template<class Version>
struct Inner3
{
  int x;
  std::shared_ptr<Q> toQ;
  int y;

  template<class Archive>
  void serialize(Archive & ar, std::uint32_t version)
  {
    ar(x);
    if (Version::version >= 1 && version >= 1) {
      ar(toQ);
      ar(y);
    }

  }
};

template<class Version>
struct Out3
{
  Inner3<Version> inner;
  std::shared_ptr<Z> toZ;
  std::shared_ptr<Q> toQ;
  std::shared_ptr<Z> toZ2;
  std::shared_ptr<Q> toQ2;

  template<class Archive>
  void serialize(Archive & ar, std::uint32_t)
  {
    ar(inner, toZ, toQ, toZ2, toQ2);
  }
};

CEREAL_CLASS_VERSION(Inner3<Version1>, 1)

template<class IArchive, class OArchive>
void test_omited_shared_out_of_order_3()
{

  std::random_device rd;
  std::mt19937 gen(rd());

  for (int ii = 0; ii < ITERATIONS_PER_TEST; ++ii) {
    auto toZ = std::shared_ptr<Z>(new Z{random_value<int>(gen)});
    auto toQ = std::shared_ptr<Q>(new Q{toZ, random_value<int>(gen)});
    Out3<Version1> o_struct = {Inner3<Version1>{random_value<int>(gen), toQ, random_value<int>(gen)}, toZ, toQ, toZ, toQ};

    std::ostringstream os;
    {
      OArchive oar(os);
      oar(o_struct);
    }

    Out3<Version0> i_struct;

    std::istringstream is(os.str());
    {
      IArchive iar(is);
      iar(i_struct);
    }

    BOOST_CHECK(*o_struct.toQ == *i_struct.toQ);
    BOOST_CHECK_EQUAL(o_struct.inner.x, i_struct.inner.x);
    // addresses
    BOOST_REQUIRE(i_struct.toQ != nullptr);
    BOOST_REQUIRE(i_struct.toQ->toZ != nullptr);
    BOOST_REQUIRE(i_struct.toQ2 != nullptr);
    BOOST_REQUIRE(i_struct.toQ2->toZ != nullptr);

    BOOST_CHECK(*o_struct.toQ == *i_struct.toQ);
    BOOST_CHECK_EQUAL(i_struct.toQ, i_struct.toQ2);
    BOOST_CHECK(*o_struct.toZ == *i_struct.toZ);
    BOOST_CHECK_EQUAL(i_struct.toZ, i_struct.toZ2);
  };
}

BOOST_AUTO_TEST_CASE( extendable_binary_omited_shared_out_of_order_3 )
{
  test_omited_shared_out_of_order_3<cereal::ExtendableBinaryInputArchive, cereal::ExtendableBinaryOutputArchive>();
}
