/*! \file versioning_omit_pointers.hpp
    \brief Check if omit is working with derived class registration
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

class A;
class B;
class AA;
class BB;
class AAA;
class BBB;
static const bool WRITE_TO_CONSOLE=false;

static uint8_t A_VERSION_WRITE = 0;
static uint8_t A_VERSION_READ = 0;

class AAA {
  public:
  int aaa;
  virtual ~AAA() {}
  virtual void init() {
    aaa = 123456789;
  }
  bool check() {
    BOOST_CHECK(aaa == 123456789);
    return aaa == 123456789;
  }
  template<class Archive>
  void serialize(Archive & ar, const unsigned int /*version*/) {
    ar & CEREAL_NVP(aaa);
  }
};

class BBB : public AAA {
  public:
  int bbb;
  virtual ~BBB() {}
  virtual void init() {
    AAA::init();
    bbb = 213456789;
  }
  virtual bool check() {
    BOOST_CHECK(bbb == 213456789);
    return AAA::check() && bbb == 213456789;
  }
  template<class Archive>
  void serialize(Archive & ar, const unsigned int /*version*/) {
    ar & cereal::virtual_base_class<AAA>(this);
    ar & CEREAL_NVP(bbb);
  }
};

class AAAA {
  public:
  int aaaa;
  virtual ~AAAA() {}
  virtual void init() {
    aaaa = 123456789;
  }
  bool check() {
    BOOST_CHECK(aaaa == 123456789);
    return aaaa == 123456789;
  }
  template<class Archive>
  void serialize(Archive & ar, const unsigned int /*version*/) {
    ar & CEREAL_NVP(aaaa);
  }
};

class BBBB : public AAAA {
  public:
  int bbbb;
  virtual ~BBBB() {}
  virtual void init() {
    AAAA::init();
    bbbb = 213456788;
  }
  virtual bool check() {
    BOOST_CHECK(bbbb == 213456788);
    return AAAA::check() && bbbb == 213456788;
  }
  template<class Archive>
  void serialize(Archive & ar, const unsigned int /*version*/) {
    ar & cereal::virtual_base_class<AAAA>(this);
    ar & CEREAL_NVP(bbbb);
  }
};


class AA {
  public:
  int aa;
  virtual ~AA() {}

  virtual bool check() {
    BOOST_CHECK(aa == 7892);
    return aa == 7892;
  }

  virtual void init() {
    aa = 7892;
  }
  template<class Archive>
  void serialize(Archive & ar, const unsigned int /*version*/) {
    ar & CEREAL_NVP(aa);
  }
};

class BB : public AA {
  public:
  int bb;
  virtual bool check() {
    BOOST_CHECK(bb == 7891);
    return AA::check() && bb == 7891;
  }

  virtual void init() {
    AA::init();
    bb = 7891;
  }
  template<class Archive>
  void serialize(Archive & ar, const unsigned int /*version*/) {
    ar & cereal::virtual_base_class<AA>(this);
    ar & CEREAL_NVP(bb);
  }
};


class A {
  public:
  enum OmitFields {
    OMIT_NONE = 0,
    OMIT1_FROM_A= 1,
    OMIT2_FROM_A = 2,
    OMIT3_FROM_A = 4,
    SKIP1_FROM_A = 8,
    SKIP2_FROM_A = 16,
    SKIP3_FROM_A = 32,
  };

  std::unique_ptr<AA> AAtoBB1;
  std::unique_ptr<AA> AAtoBB2;
  std::unique_ptr<AAA> AAAtoBBB;

  A() :AAtoBB1(nullptr), AAtoBB2(nullptr), AAAtoBBB(nullptr) {}

  virtual ~A() {}


  void init() {
    AAtoBB1 = std::unique_ptr<AA>(new BB());
    AAtoBB2 = std::unique_ptr<AA>(new BB());
    AAAtoBBB = std::unique_ptr<AAA>(new BBB());

    AAtoBB1->init();
    AAtoBB2->init();
    AAAtoBBB->init();
  }

  template<class Archive>
  void serialize(Archive & ar, const unsigned int /*version*/) {
    uint8_t version = Archive::is_saving::value ? A_VERSION_WRITE : A_VERSION_READ;
    if(WRITE_TO_CONSOLE) std::cout << "\t\tserialize A archive saving?" << Archive::is_saving::value << " version:" << std::bitset<8>(version) << std::endl;
    if(version & OMIT1_FROM_A) {
      ar(CEREAL_NVP(cereal::OmittedFieldTag())); // AA*
    } else if(version & SKIP1_FROM_A) {
      return;
      // do nothing
    } else {
      ar & CEREAL_NVP(AAtoBB1);
    }
    if(Archive::is_loading::value) {
      BOOST_CHECK_EQUAL(ar.wasSerialized(), false == (A_VERSION_WRITE & OMIT1_FROM_A));
    }
    if(version & OMIT2_FROM_A) {
      ar(CEREAL_NVP(cereal::OmittedFieldTag())); // AA*
    } else if(version & SKIP2_FROM_A) {
      return;
      // do nothing
    } else {
      ar & CEREAL_NVP(AAtoBB2);
    }
    if(Archive::is_loading::value) {
      BOOST_CHECK_EQUAL(ar.wasSerialized(), false == (A_VERSION_WRITE & OMIT2_FROM_A));
    }
    if(version & OMIT3_FROM_A) {
      ar(CEREAL_NVP(cereal::OmittedFieldTag())); // AAA*
    } else if(version & SKIP3_FROM_A) {
      return;
    } else {
      ar & CEREAL_NVP(AAAtoBBB);
    }
    if(Archive::is_loading::value) {
      BOOST_CHECK_EQUAL(ar.wasSerialized(), false == (A_VERSION_WRITE & OMIT3_FROM_A));
    }
  }
  bool check() {
    if(WRITE_TO_CONSOLE) std::cout << "\t\tcheck A archive version write:" << std::bitset<8>(A_VERSION_WRITE) << " version read:" << std::bitset<8>(A_VERSION_WRITE) << std::endl;

    if((A_VERSION_WRITE & SKIP1_FROM_A) || (A_VERSION_READ & SKIP1_FROM_A)) {
      BOOST_CHECK(AAtoBB1 == nullptr);
      BOOST_CHECK(AAtoBB2 ==  nullptr);
      BOOST_CHECK(AAAtoBBB == nullptr);
      return true; // skip next checks
    }
    if((A_VERSION_WRITE & OMIT1_FROM_A) || (A_VERSION_READ & OMIT1_FROM_A)) {
      BOOST_CHECK(AAtoBB1 == nullptr);
    } else {
      BOOST_ASSERT(dynamic_cast<BB*>(AAtoBB1.get()) != nullptr);
      BOOST_CHECK(dynamic_cast<BB*>(AAtoBB1.get())->check());
    }

    if((A_VERSION_WRITE & SKIP2_FROM_A) || (A_VERSION_READ & SKIP2_FROM_A)) {
      BOOST_CHECK(AAtoBB2 == nullptr);
      BOOST_CHECK(AAAtoBBB == nullptr);
      return true;
    }
    if((A_VERSION_WRITE & OMIT2_FROM_A) || (A_VERSION_READ & OMIT2_FROM_A)) {
      BOOST_CHECK(AAtoBB2 == nullptr);
    } else {
      BOOST_ASSERT(dynamic_cast<BB*>(AAtoBB2.get()) != nullptr);
      BOOST_CHECK(dynamic_cast<BB*>(AAtoBB2.get())->check());
    }

    if((A_VERSION_WRITE & SKIP3_FROM_A) || (A_VERSION_READ & SKIP3_FROM_A)) {
      BOOST_CHECK(AAAtoBBB == nullptr);
      return true;
    }
    if((A_VERSION_WRITE & OMIT3_FROM_A) || (A_VERSION_READ & OMIT3_FROM_A)) {
      BOOST_CHECK(AAAtoBBB == nullptr);
    } else {
      BOOST_ASSERT(AAAtoBBB != nullptr);
      BOOST_CHECK(AAAtoBBB->check());
    }
    return true;
  }


};

class B {
  public:
  std::unique_ptr<A> a;

  std::unique_ptr<AAAA> AAAAtoBBBB;
  std::unique_ptr<AA> AAtoBB;
  int o;

  B() :a(nullptr), AAAAtoBBBB(nullptr), o(0) {}

  virtual ~B() {}

  void init() {
    a = std::unique_ptr<A>(new A());
    AAAAtoBBBB = std::unique_ptr<AAAA>(new BBBB());
    o = 12468546;

    AAAAtoBBBB->init();
    a->init();

    AAtoBB = std::unique_ptr<AA>(new BB());
    AAtoBB->init();
  }

  template<class Archive>
  void serialize(Archive & ar, const unsigned int /*version*/) {
    if(WRITE_TO_CONSOLE) std::cout << "\tserialize B archive saving?" << Archive::is_saving::value << std::endl;
    ar & CEREAL_NVP(a);
    ar & CEREAL_NVP(o);
    ar & CEREAL_NVP(AAAAtoBBBB);
    ar & CEREAL_NVP(AAtoBB);
  }
  bool check() {
    if(WRITE_TO_CONSOLE) std::cout << "\tcheck B" << std::endl;
    BOOST_CHECK(a != nullptr);
    BOOST_CHECK(o == 12468546);
    BOOST_CHECK(AAAAtoBBBB != nullptr);
    BOOST_ASSERT(dynamic_cast<BBBB*>(AAAAtoBBBB.get()) != nullptr);
    BOOST_ASSERT(AAtoBB != nullptr);
    BOOST_ASSERT(dynamic_cast<BB*>(AAtoBB.get()) != nullptr);
    return a != nullptr && AAAAtoBBBB != nullptr && a->check() && AAAAtoBBBB->check() && AAtoBB->check();
  }

};



CEREAL_REGISTER_TYPE(A)
CEREAL_REGISTER_TYPE(AA)
CEREAL_REGISTER_TYPE(BB)
CEREAL_REGISTER_TYPE(AAA)
CEREAL_REGISTER_TYPE(BBB)
CEREAL_REGISTER_TYPE(AAAA)
CEREAL_REGISTER_TYPE(BBBB)

template <class IArchive, class OArchive>
static bool check_both(uint8_t write, uint8_t read) {

  A_VERSION_WRITE = write;
  A_VERSION_READ = read;
  if(WRITE_TO_CONSOLE) std::cout << "check_both write:" << std::bitset<8>(write) << " read: " << std::bitset<8>(read) << std::endl;

  bool ok = false;
  std::ostringstream os(std::ios_base::binary | std::ios_base::out);
  BOOST_REQUIRE_NO_THROW({
                           B b;
                           b.init();
                           OArchive ar(os);
                           ar(b);
                         });
  try {
    B b;
    std::istringstream is(os.str(), std::ios_base::binary | std::ios_base::in);
    IArchive ar(is);
    ar(b);
    ok = b.check();
  } catch (const std::exception &e) {
    std::cerr << "exception: " << e.what() << "\n";
    BOOST_CHECK(false);
  }
  if(WRITE_TO_CONSOLE) std::cout << std::endl;
  BOOST_CHECK(ok);
  return ok;
}

template <class IArchive, class OArchive>
static bool check_first_none(uint8_t two) {
  return check_both<IArchive, OArchive>(A::OMIT_NONE, two);
}

// comma cannot be in BOOST_CHECK macro
static bool check_both_json(uint8_t write, uint8_t read) {
  return check_both<cereal::JSONInputArchive, cereal::JSONOutputArchive>(write, read);
}

static bool check_both_xml(uint8_t write, uint8_t read) {
  return check_both<cereal::XMLInputArchive, cereal::XMLOutputArchive>(write, read);
}

static bool check_both_extendable_binary(uint8_t write, uint8_t read) {
  return check_both<cereal::ExtendableBinaryInputArchive, cereal::ExtendableBinaryOutputArchive>(write, read);
}

static bool check_first_none_json(uint8_t two) {
  return check_both<cereal::JSONInputArchive, cereal::JSONOutputArchive>(A::OMIT_NONE, two);
}

static bool check_first_none_extendable_binary(uint8_t two) {
  return check_both<cereal::ExtendableBinaryInputArchive, cereal::ExtendableBinaryOutputArchive>(A::OMIT_NONE, two);
}
BOOST_AUTO_TEST_SUITE(OmitClassRegistration)

/* both are reading and saving same fields */
  BOOST_AUTO_TEST_CASE(OmitNone)
  {
    BOOST_CHECK(check_both_json(A::OMIT_NONE, A::OMIT_NONE));
    BOOST_CHECK(check_both_xml(A::OMIT_NONE, A::OMIT_NONE));
    BOOST_CHECK(check_both_extendable_binary(A::OMIT_NONE, A::OMIT_NONE));
  }

  BOOST_AUTO_TEST_CASE(BothSkip1)
  {
    BOOST_CHECK(check_both_json(A::SKIP1_FROM_A, A::SKIP1_FROM_A));
    BOOST_CHECK(check_both_xml(A::SKIP1_FROM_A, A::SKIP1_FROM_A));
    BOOST_CHECK(check_both_extendable_binary(A::SKIP1_FROM_A, A::SKIP1_FROM_A));
  }

  BOOST_AUTO_TEST_CASE(BothSkip2)
  {
    BOOST_CHECK(check_both_json(A::SKIP2_FROM_A, A::SKIP2_FROM_A));
    BOOST_CHECK(check_both_xml(A::SKIP2_FROM_A, A::SKIP2_FROM_A));
    BOOST_CHECK(check_both_extendable_binary(A::SKIP2_FROM_A, A::SKIP2_FROM_A));
  }

  BOOST_AUTO_TEST_CASE(BothSkip3)
  {
    BOOST_CHECK(check_both_json(A::SKIP3_FROM_A, A::SKIP3_FROM_A));
    BOOST_CHECK(check_both_xml(A::SKIP3_FROM_A, A::SKIP3_FROM_A));
    BOOST_CHECK(check_both_extendable_binary(A::SKIP3_FROM_A, A::SKIP3_FROM_A));
  }

  BOOST_AUTO_TEST_CASE(OmitBoth1)
  {
    BOOST_CHECK(check_both_extendable_binary(A::OMIT1_FROM_A, A::OMIT1_FROM_A));
  }

  BOOST_AUTO_TEST_CASE(OmitBoth2)
  {
    BOOST_CHECK(check_both_extendable_binary(A::OMIT2_FROM_A, A::OMIT2_FROM_A));
  }

  BOOST_AUTO_TEST_CASE(OmitBoth3)
  {
    BOOST_CHECK(check_both_extendable_binary(A::OMIT3_FROM_A, A::OMIT3_FROM_A));
  }

  BOOST_AUTO_TEST_CASE(OmitBoth1a2)
  {
    BOOST_CHECK(check_both_extendable_binary(A::OMIT1_FROM_A | A::OMIT2_FROM_A, A::OMIT1_FROM_A | A::OMIT2_FROM_A));
  }

  BOOST_AUTO_TEST_CASE(OmitBoth1a3)
  {
    BOOST_CHECK(check_both_extendable_binary(A::OMIT1_FROM_A | A::OMIT3_FROM_A, A::OMIT1_FROM_A | A::OMIT3_FROM_A));
  }

  BOOST_AUTO_TEST_CASE(OmitBoth2a3)
  {
    BOOST_CHECK(check_both_extendable_binary(A::OMIT2_FROM_A | A::OMIT3_FROM_A, A::OMIT2_FROM_A | A::OMIT3_FROM_A));
  }

  BOOST_AUTO_TEST_CASE(OmitBoth1a2a3)
  {
    BOOST_CHECK(check_both_extendable_binary(A::OMIT1_FROM_A | A::OMIT2_FROM_A | A::OMIT3_FROM_A,
                                             A::OMIT1_FROM_A | A::OMIT2_FROM_A | A::OMIT3_FROM_A));
  }

/* reading side is omitting fields */

  BOOST_AUTO_TEST_CASE(OmitSec1)
  {
    BOOST_CHECK(check_first_none_extendable_binary(A::OMIT1_FROM_A));
  }

  BOOST_AUTO_TEST_CASE(OmitSec2)
  {
    BOOST_CHECK(check_first_none_extendable_binary(A::OMIT2_FROM_A));
  }

  BOOST_AUTO_TEST_CASE(OmitSec3)
  {
    BOOST_CHECK(check_first_none_extendable_binary(A::OMIT3_FROM_A));
  }

  BOOST_AUTO_TEST_CASE(OmitSec1a2)
  {
    BOOST_CHECK(check_first_none_extendable_binary(A::OMIT1_FROM_A | A::OMIT2_FROM_A));
  }

  BOOST_AUTO_TEST_CASE(OmitSec2a3)
  {
    BOOST_CHECK(check_first_none_extendable_binary(A::OMIT2_FROM_A | A::OMIT3_FROM_A));
  }

  BOOST_AUTO_TEST_CASE(OmitSec1a3)
  {
    BOOST_CHECK(check_first_none_extendable_binary(A::OMIT1_FROM_A | A::OMIT3_FROM_A));
  }

  BOOST_AUTO_TEST_CASE(OmitSec1a2a3)
  {
    BOOST_CHECK(check_first_none_extendable_binary(A::OMIT1_FROM_A | A::OMIT2_FROM_A | A::OMIT3_FROM_A));
  }

/* reading side is skipping fields */

  BOOST_AUTO_TEST_CASE(SkipSec1)
  {
    BOOST_CHECK(check_first_none_extendable_binary(A::SKIP1_FROM_A));
    // BOOST_CHECK(check_first_none_json(A::SKIP1_FROM_A)); not working
  }

  BOOST_AUTO_TEST_CASE(SkipSec2)
  {
    BOOST_CHECK(check_first_none_extendable_binary(A::SKIP2_FROM_A));
    BOOST_CHECK(check_first_none_json(A::SKIP2_FROM_A));
  }

  BOOST_AUTO_TEST_CASE(SkipSec3)
  {
    BOOST_CHECK(check_first_none_extendable_binary(A::SKIP3_FROM_A));
    BOOST_CHECK(check_first_none_json(A::SKIP3_FROM_A));
  }

/* writing side is skipping fields */
  BOOST_AUTO_TEST_CASE(OmitFirst1)
  {
    BOOST_CHECK(check_both_extendable_binary(A::OMIT1_FROM_A, A::OMIT_NONE));
  }

  BOOST_AUTO_TEST_CASE(OmitFirst2)
  {
    BOOST_CHECK(check_both_extendable_binary(A::OMIT2_FROM_A, A::OMIT_NONE));
  }

  BOOST_AUTO_TEST_CASE(OmitFirst3)
  {
    BOOST_CHECK(check_both_extendable_binary(A::OMIT3_FROM_A, A::OMIT_NONE));
  }

  BOOST_AUTO_TEST_CASE(OmitFirst1a2)
  {
    BOOST_CHECK(check_both_extendable_binary(A::OMIT1_FROM_A | A::OMIT2_FROM_A, A::OMIT_NONE));
  }

  BOOST_AUTO_TEST_CASE(OmitFirst2a3)
  {
    BOOST_CHECK(check_both_extendable_binary(A::OMIT1_FROM_A | A::OMIT3_FROM_A, A::OMIT_NONE));
  }

  BOOST_AUTO_TEST_CASE(OmitNone1a2a3)
  {
    BOOST_CHECK(check_both_extendable_binary(A::OMIT1_FROM_A | A::OMIT2_FROM_A | A::OMIT3_FROM_A, A::OMIT_NONE));
  }

#if 0
  /* Optional loading of all fields, not only omitted field would be required to pass this tests */
/* skip first */
  BOOST_AUTO_TEST_CASE( SkipFirst1 ) {
    BOOST_CHECK(check_both_extendable_binary(A::SKIP1_FROM_A, A::OMIT_NONE));
  }
  BOOST_AUTO_TEST_CASE( SkipFirst2 ) {
    BOOST_CHECK(check_both_extendable_binary(A::SKIP2_FROM_A, A::OMIT_NONE));
  }
  BOOST_AUTO_TEST_CASE( SkipFirst3 ) {
    BOOST_CHECK(check_both_extendable_binary(A::SKIP3_FROM_A, A::OMIT_NONE));
  }

/* mixed skip */
  BOOST_AUTO_TEST_CASE( SkipFirst1Sec2 ) {
    BOOST_CHECK(check_both(A::SKIP1_FROM_A, A::SKIP2_FROM_A));
  }
  BOOST_AUTO_TEST_CASE( SkipFirst1Sec3 ) {
    BOOST_CHECK(check_both(A::SKIP1_FROM_A, A::SKIP3_FROM_A));
  }

  BOOST_AUTO_TEST_CASE( SkipFirst2Sec1 ) {
    BOOST_CHECK(check_both(A::SKIP2_FROM_A, A::SKIP1_FROM_A));
  }
  BOOST_AUTO_TEST_CASE( SkipFirst2Sec3 ) {
    BOOST_CHECK(check_both(A::SKIP2_FROM_A, A::SKIP3_FROM_A));
  }

  BOOST_AUTO_TEST_CASE( SkipFirst3Sec1 ) {
    BOOST_CHECK(check_both(A::SKIP3_FROM_A, A::SKIP1_FROM_A));
  }
  BOOST_AUTO_TEST_CASE( SkipFirst3Sec2 ) {
    BOOST_CHECK(check_both(A::SKIP3_FROM_A, A::SKIP2_FROM_A));
  }
#endif

BOOST_AUTO_TEST_SUITE_END()

