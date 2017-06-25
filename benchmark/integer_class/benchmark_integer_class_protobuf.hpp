//
// Created by breiker on 6/26/17.
//

#pragma once

#include "integer_class.hpp"

//////////////////////////
// Protobuf adapters
/////////////////////////
class IntegerOProtobuf
{
  public:
    IntegerOProtobuf(std::ostream & os) : os_(os)
    {}

    IntegerOProtobuf & operator<<(IntegerClass & c)
    {
      ProtoIntegerClass pc;
      pc.set_f1(c.f1);
      pc.set_f2(c.f2);
      pc.set_f3(c.f3);
      pc.set_f4(c.f4);
      pc.set_f5(c.f5);
      pc.set_f6(c.f6);
      pc.set_f7(c.f7);
      pc.set_f8(c.f8);
      pc.set_f9(c.f9);
      pc.SerializeToOstream(&os_);
      return *this;
    }

  private:
    std::ostream & os_;

};

class IntegerIProtobuf
{
  public:
    IntegerIProtobuf(std::istream & is) : is_(is)
    {}

    IntegerIProtobuf & operator>>(IntegerClass & c)
    {
      ProtoIntegerClass pc;
      pc.ParseFromIstream(&is_);
      c.f1 = pc.f1();
      c.f2 = pc.f2();
      c.f3 = pc.f3();
      c.f4 = pc.f4();
      c.f5 = pc.f5();
      c.f6 = pc.f6();
      c.f7 = pc.f7();
      c.f8 = pc.f8();
      c.f9 = pc.f9();
      return *this;
    }

  private:
    std::istream & is_;
};

class IntegerVectOProtobuf
{
  public:
    IntegerVectOProtobuf(std::ostream & os) : os_(os)
    {}

    IntegerVectOProtobuf & operator<<(IntegerClassVect & c)
    {
      ProtoIntegerClassVect pc;
      for (const auto e : c.v) {
        auto pe = pc.add_f1();

        pe->set_f1(e.f1);
        pe->set_f2(e.f2);
        pe->set_f3(e.f3);
        pe->set_f4(e.f4);
        pe->set_f5(e.f5);
        pe->set_f6(e.f6);
        pe->set_f7(e.f7);
        pe->set_f8(e.f8);
        pe->set_f9(e.f9);
        pe->set_f10(e.f10);
      }
      pc.SerializeToOstream(&os_);
      return *this;
    }

  private:
    std::ostream & os_;

};

class IntegerVectIProtobuf
{
  public:
    IntegerVectIProtobuf(std::istream & is) : is_(is)
    {}

    IntegerVectIProtobuf & operator>>(IntegerClassVect & c)
    {
      ProtoIntegerClassVect pc;
      pc.ParseFromIstream(&is_);
      c.v.reserve(pc.f1_size());
      for (int i = 0; i < pc.f1_size(); ++i) {
        auto e = pc.f1(i);
        c.v.emplace_back(IntegerClass{
            e.f1(),
            e.f2(),
            e.f3(),
            e.f4(),
            e.f5(),
            e.f6(),
            e.f7(),
            e.f8(),
            e.f9(),
            e.f10(),
        });
      }

      return *this;
    }

  private:
    std::istream & is_;
};
