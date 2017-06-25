//
// Created by breiker on 6/26/17.
//

#pragma once

#include <vector>
#include <functional>

#include "utils.hpp"


class IntegerClass
{
  public:
    int f1;
    int f2;
    int f3;
    int f4;
    int f5;
    int f6;
    int f7;
    int f8;
    int f9;
    int f10;
};


class IntegerClassVect
{
  public:
    void clear() {
      v.clear();
    }
    std::vector<IntegerClass> v;
};

inline void RandomInit(std::function<void(int&)> rand, IntegerClass& out)
{
  rand(out.f1);
  rand(out.f2);
  rand(out.f3);
  rand(out.f4);
  rand(out.f5);
  rand(out.f6);
  rand(out.f7);
  rand(out.f8);
  rand(out.f9);
  rand(out.f10);
}
inline void InitWithRandomWholeRange(IntegerClass & c)
{
  RandomInit(&RandomIntWholeRange, c);
}
inline void InitWithRandomNormalOneByte(IntegerClass & c)
{
  RandomInit(&RandomIntSmall, c);
}

inline void RandomInit(std::size_t size, std::function<void(IntegerClass&)> rand, IntegerClassVect& out) {
  out.v.reserve(size);
  for(std::size_t i = 0; i < size; ++i) {
    out.v.emplace_back();
    rand(out.v.back());
  }
}

template<class Archive>
void serialize(Archive & ar, IntegerClass & c, unsigned int/*version*/)
{
  ar & c.f1;
  ar & c.f2;
  ar & c.f3;
  ar & c.f4;
  ar & c.f5;
  ar & c.f6;
  ar & c.f7;
  ar & c.f8;
  ar & c.f9;
  ar & c.f10;

}

template<class Archive>
void serialize(Archive & ar, IntegerClassVect & c, unsigned int/*version*/)
{
  ar & c.v;
}
