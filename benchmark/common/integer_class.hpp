//
// Created by breiker on 6/11/17.
//

#pragma once

#if CELERO_BENCHMARK
#include <celero/Celero.h>
#else
#include <benchmark/benchmark.h>
#endif

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


#if CELERO_BENCHMARK
class save_integer_class : public celero::TestFixture
{
  public:
    virtual void setUp(int64_t /*experimentalValue*/)
    {
      c.f1 = Random::getInstance().uniformIntDistribution(Random::getInstance().randomDevice);
      c.f2 = Random::getInstance().uniformIntDistribution(Random::getInstance().randomDevice);
      c.f3 = Random::getInstance().uniformIntDistribution(Random::getInstance().randomDevice);
      c.f4 = Random::getInstance().uniformIntDistribution(Random::getInstance().randomDevice);
      c.f5 = Random::getInstance().uniformIntDistribution(Random::getInstance().randomDevice);
      c.f6 = Random::getInstance().uniformIntDistribution(Random::getInstance().randomDevice);
      c.f7 = Random::getInstance().uniformIntDistribution(Random::getInstance().randomDevice);
      c.f8 = Random::getInstance().uniformIntDistribution(Random::getInstance().randomDevice);
      c.f9 = Random::getInstance().uniformIntDistribution(Random::getInstance().randomDevice);
      c.f10 = Random::getInstance().uniformIntDistribution(Random::getInstance().randomDevice);

    }

    IntegerClass c;
};
#else


class save_integer_class_b : public benchmark::Fixture
{
  public:
    virtual void SetUp(benchmark::State&/*state*/) override
    {
      c.f1 = Random::getInstance().uniformIntDistribution(Random::getInstance().randomDevice);
      c.f2 = Random::getInstance().uniformIntDistribution(Random::getInstance().randomDevice);
      c.f3 = Random::getInstance().uniformIntDistribution(Random::getInstance().randomDevice);
      c.f4 = Random::getInstance().uniformIntDistribution(Random::getInstance().randomDevice);
      c.f5 = Random::getInstance().uniformIntDistribution(Random::getInstance().randomDevice);
      c.f6 = Random::getInstance().uniformIntDistribution(Random::getInstance().randomDevice);
      c.f7 = Random::getInstance().uniformIntDistribution(Random::getInstance().randomDevice);
      c.f8 = Random::getInstance().uniformIntDistribution(Random::getInstance().randomDevice);
      c.f9 = Random::getInstance().uniformIntDistribution(Random::getInstance().randomDevice);
      c.f10 = Random::getInstance().uniformIntDistribution(Random::getInstance().randomDevice);
    }

    IntegerClass c;
};
#endif


#if CELERO_BENCHMARK
class save_integer_class_vect : public celero::TestFixture
{
  public:

//    virtual std::vector<std::pair<int64_t, uint64_t>> getExperimentValues() const override
//    {
//      std::vector<std::pair<int64_t, uint64_t>> problemSpace;
//
//      // We will run some total number of sets of tests all together.
//      // Each one growing by a power of 2.
//      const int totalNumberOfTests = 6;
//
//      for (int i = 0; i < totalNumberOfTests; i++) {
//        // ExperimentValues is part of the base class and allows us to specify
//        // some values to control various test runs to end up building a nice graph.
//        problemSpace.push_back(std::make_pair(int64_t(pow(2, i + 2)), uint64_t(0)));
//      }
//
//      return problemSpace;
//    }

    virtual void setUp(int64_t experimentalValue) override
    {
      experimentalValue = 128;
      c.v.clear();
      c.v.reserve(experimentalValue);
      for (int64_t i = 0; i < experimentalValue; ++i) {
        c.v.emplace_back(IntegerClass{
            Random::getInstance().uniformIntDistribution(Random::generator()),
            Random::getInstance().uniformIntDistribution(Random::generator()),
            Random::getInstance().uniformIntDistribution(Random::generator()),
            Random::getInstance().uniformIntDistribution(Random::generator()),
            Random::getInstance().uniformIntDistribution(Random::generator()),
            Random::getInstance().uniformIntDistribution(Random::generator()),
            Random::getInstance().uniformIntDistribution(Random::generator()),
            Random::getInstance().uniformIntDistribution(Random::generator()),
            Random::getInstance().uniformIntDistribution(Random::generator()),
            Random::getInstance().uniformIntDistribution(Random::generator())
        });
      }
    }
    virtual void tearDown() override {
      c.v.clear();
    }

    IntegerClassVect c;
};
#endif

