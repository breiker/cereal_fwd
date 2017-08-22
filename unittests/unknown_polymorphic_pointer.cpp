/*! \file unknown_polymorhic_pointer.cpp
    \brief Load polymorphic pointers of unknown type
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
#include <cereal/types/memory.hpp>

#include <cereal/archives/extendable_binary.hpp>
#include <boost/test/unit_test.hpp>
#include <fstream>


struct PolymorphicBase
{
  int x;
  int y;

  PolymorphicBase() {}
  PolymorphicBase(int x_, int y_) :
      x(x_), y(y_)
  {}
  virtual ~PolymorphicBase() = default;

  template<class Archive>
  void serialize(Archive& ar) {
    ar(x, y);
  }

};
struct DerivedPolymorphic : public PolymorphicBase
{
  int first;
  DerivedPolymorphic() {}
  DerivedPolymorphic(int first_, int internalX_, int internalY_) :
      PolymorphicBase(internalX_, internalY_), first(first_)
  {}

  template <class Archive>
  void serialize(Archive& ar) {
    ar(cereal::base_class<PolymorphicBase>(this));
    ar(first);
  }
};

CEREAL_REGISTER_TYPE(DerivedPolymorphic)

template <class IArchive, class OArchive>
void test_unknown_polymorphic_pointers()
{
  auto o_first = 1;
  std::shared_ptr<PolymorphicBase> o_shared = std::make_shared<DerivedPolymorphic>(20, 21, 22);
  std::unique_ptr<PolymorphicBase> o_unique = std::unique_ptr<DerivedPolymorphic>(new DerivedPolymorphic(30, 31, 32));
  auto o_last = 4;

  std::ostringstream os(std::ios::binary);
  {
    OArchive oar(os);
    oar(o_first);
    oar(o_shared);
    oar(o_unique);
    oar(o_last);
  }

  std::string changed = os.str();
  std::string typeName = "DerivedPolymorphic";
  std::string typeNameChanged = "dERIVEDpOLYMORPHIC";
  const auto found = changed.find(typeName);
  BOOST_CHECK_NE(found, std::string::npos);
  changed.replace(found, typeName.size(), typeNameChanged);


  auto i_first = 0;
  std::shared_ptr<PolymorphicBase> i_shared;
  std::unique_ptr<PolymorphicBase> i_unique;
  auto i_last = 0;

  std::istringstream is(changed, std::ios::binary);
  {
    IArchive iar(is);
    iar(i_first);
    iar(i_shared);
    iar(i_unique);
    iar(i_last);
  }
  BOOST_CHECK_EQUAL(i_first, 1);
  BOOST_CHECK(nullptr == i_shared);
  BOOST_CHECK(nullptr == i_unique);
  BOOST_CHECK_EQUAL(i_last, 4);
}

BOOST_AUTO_TEST_CASE( extendable_binary_unknown_polymorpic_pointer )
{
  test_unknown_polymorphic_pointers<cereal::ExtendableBinaryInputArchive, cereal::ExtendableBinaryOutputArchive>();
}
