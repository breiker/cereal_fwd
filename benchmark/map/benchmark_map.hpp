//
// Created by breiker on 6/22/17.
//

#pragma once


#include <cstdint>
#include <string>
#include <map>
#include <iostream>
#include <functional>

#include "utils.hpp"


//////////////////////////
// Protobuf adapters
/////////////////////////
class ProtoMapInt;

template<>
struct ProtoClass<std::map<std::int32_t, std::int32_t>>
{
  using type = ProtoMapInt;
};


template<class T>
class MapOProtobuf
{
  public:
    MapOProtobuf(std::ostream & os) : os_(os)
    {}

    MapOProtobuf & operator<<(const T & c)
    {
      typename ProtoClass<T>::type pc;
      auto map = pc.mutable_f1();
      for (const auto e : c) {
        map->insert({e.first, e.second});
      }
      pc.SerializeToOstream(&os_);
      return *this;
    }

  private:
    std::ostream & os_;
};

template<class T>
class MapIProtobuf
{
  public:
    MapIProtobuf(std::istream & is) : is_(is)
    {}

    MapIProtobuf & operator>>(T & c)
    {
      typename ProtoClass<T>::type pc;
      pc.ParseFromIstream(&is_);
      const auto & pcv = pc.f1();
      for (const auto& e : pcv) {
        c.emplace(e.first, e.second);
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
inline void RandomInitT(std::size_t size, std::function<void(T&)>& rand, std::map<T, T> & out)
{
  for (std::size_t i = 0; i < size; ++i) {
    T first, second;
    rand(first);
    rand(second);
    out.emplace(first, second);
  }
}

template<class T>
inline void RandomInit(std::size_t size, std::function<void(std::int32_t&)> rand, std::map<T, T> & out)
{
  RandomInitT<>(size, rand, out);
}

template<class T>
inline void RandomInit(std::size_t size, std::function<void(float&)> rand, std::map<T, T> & out)
{
  RandomInitT<>(size, rand, out);
}