//
// Created by breiker on 6/11/17.
//

#include "utils.hpp"

thread_local Random Random::random;

Random& GetRandom() {
  static thread_local Random random;
  return random;
}

#if USING_NULLSTREAM
int NullBuffer::overflow(int c)
{
  ++bytes;
  return c;
}
#endif

Random::Random()
    : gen(randomDevice()),
      uniformIntDistribution(std::numeric_limits<int>::min(), std::numeric_limits<int>::max()),
      uniformFloatDistribution(std::numeric_limits<float>::min(), std::numeric_limits<float>::max()),
      normalDistributionFirstByte(0, std::numeric_limits<signed char>::max())
{

}

