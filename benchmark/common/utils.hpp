//
// Created by breiker on 6/11/17.
//

#pragma once

#include <iostream>
#include <random>
#include <sstream>
#include <limits>
#define USING_NULLSTREAM 1



#if USING_NULLSTREAM
class NullBuffer : public std::streambuf
{
  public:
    int overflow(int c);
    std::uint64_t bytes = 0;
};

class NullStream : public std::ostream
{
  public:
    NullStream() : std::ostream(&m_sb)
    {}
    std::uint64_t bytesSaved() {
      return m_sb.bytes;
    }

  private:
    NullBuffer m_sb;
};
#else
using NullStream = std::stringstream;
#endif

template <class Cpp>
struct ProtoClass;

class Random
{
  public:
    Random();
    static Random & getInstance()
    {
      return random;
    }
    std::mt19937& generator() {
      return random.gen;
    }

    std::random_device randomDevice;
    std::mt19937 gen;
    std::uniform_int_distribution<std::int32_t> uniformIntDistribution;
    std::uniform_real_distribution<float> uniformFloatDistribution;
    std::normal_distribution<float> normalDistributionFirstByte;
    static thread_local Random random;
};

Random& GetRandom();

inline void RandomIntWholeRange(std::int32_t& v) {
  v = Random::getInstance().uniformIntDistribution(Random::getInstance().randomDevice);
}

inline void RandomFloatWholeRange(float& v) {
  v = Random::getInstance().uniformFloatDistribution(Random::getInstance().randomDevice);
}

inline void RandomIntSmall(std::int32_t& v) {
  v = std::round(Random::getInstance().normalDistributionFirstByte(Random::getInstance().randomDevice));
}