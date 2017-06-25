//
// Created by breiker on 6/25/17.
//

#pragma once

#include <functional>
#include <iostream>
#include <vector>

#include "utils.hpp"

//////////////////////////
// Protobuf adapters
/////////////////////////
class ProtoVectorInt;
class ProtoVectorFloat;
class ProtoVectorString;

template<>
struct ProtoClass<std::vector<std::int32_t>>
{
  using type = ProtoVectorInt;
};
template<>
struct ProtoClass<std::vector<float>>
{
  using type = ProtoVectorFloat;
};
template<>
struct ProtoClass<std::vector<std::string>>
{
  using type = ProtoVectorString;
};

template<class T>
class VectorOProtobuf
{
  public:
    VectorOProtobuf(std::ostream & os) : os_(os)
    {}

    VectorOProtobuf & operator<<(const T & c)
    {
      typename ProtoClass<T>::type pc;
      for (const auto e : c) {
        pc.add_f1(e);
      }
      pc.SerializeToOstream(&os_);
      return *this;
    }

  private:
    std::ostream & os_;
};

template<class T>
class VectorIProtobuf
{
  public:
    VectorIProtobuf(std::istream & is) : is_(is)
    {}

    VectorIProtobuf & operator>>(T & c)
    {
      typename ProtoClass<T>::type pc;
      pc.ParseFromIstream(&is_);
      const auto & pcv = pc.f1();
      c.reserve(pc.f1_size());
      for (int i = 0; i < pcv.size(); ++i) {
        c.emplace_back(pc.f1()[i]);
      }
      return *this;
    }

  private:
    std::istream & is_;
};

////////////////
// random
///////////////

template<class T>
inline void RandomInitT(std::size_t size, std::function<void(T&)>& rand, std::vector<T> & out)
{
  out.reserve(size);
  for (std::size_t i = 0; i < size; ++i) {
    out.emplace_back();
    rand(out.back());
  }
}

template<class T>
inline void RandomInit(std::size_t size, std::function<void(std::int32_t&)> rand, std::vector<T> & out)
{
  RandomInitT<>(size, rand, out);
}

template<class T>
inline void RandomInit(std::size_t size, std::function<void(float&)> rand, std::vector<T> & out)
{
  RandomInitT<>(size, rand, out);
}
