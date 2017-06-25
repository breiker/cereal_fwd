//
// Created by breiker on 6/16/17.
//

#pragma once

#include <memory>
#include <boost/serialization/base_object.hpp>
#include "cereal/types/base_class.hpp"
#include "utils.hpp"

namespace boost {
  namespace archive {
    class binary_oarchive;
    class binary_iarchive;
  }
}


template<class Archive, class Base, class Derived>
void SaveBaseClass(Archive& ar, Derived* derived);

template<class, class Base, class Derived>
void SaveBaseClass(boost::archive::binary_oarchive& ar, Derived* derived) {
  ar & boost::serialization::base_object<Base>(*derived);
}

template<class, class Base, class Derived>
void SaveBaseClass(boost::archive::binary_iarchive& ar, Derived* derived) {
  ar & boost::serialization::base_object<Base>(*derived);
}

template<class Archive, class Base, class Derived>
void SaveBaseClass(Archive& ar, Derived* derived) {
  ar & cereal::base_class<Base>(derived);
}

struct Node;

std::unique_ptr<Node> ClassForLevel(int level);

struct Node
{
  virtual ~Node() {}

  void add(std::size_t levels)
  {
    if (0 == levels) {
      return;
    }
    --levels;
    left_ = ClassForLevel(levels);
    right_ = ClassForLevel(levels);
    left_->add(levels);
    right_->add(levels);
  }

  std::unique_ptr<Node> left_;
  std::unique_ptr<Node> right_;

  template<class Archive>
  void serialize(Archive & ar, unsigned int/*version*/)
  {
    ar & left_;
    ar & right_;
  }
};

template<int N>
struct NodeP : Node
{
  bool p1 = true;

  template<class Archive>
  void serialize(Archive & ar, unsigned int /*version*/)
  {
    SaveBaseClass<Archive, Node>(ar, this);
    ar & p1;
  }
};

std::unique_ptr<Node> ClassForLevel(int level)
{
  switch (level) {
    case 0:
      return std::unique_ptr<Node>(new NodeP<0>);
    case 1:
      return std::unique_ptr<Node>(new NodeP<1>);
    case 2:
      return std::unique_ptr<Node>(new NodeP<2>);
    case 3:
      return std::unique_ptr<Node>(new NodeP<3>);
    case 4:
      return std::unique_ptr<Node>(new NodeP<4>);
    case 5:
      return std::unique_ptr<Node>(new NodeP<5>);
    case 6:
      return std::unique_ptr<Node>(new NodeP<6>);
    case 7:
      return std::unique_ptr<Node>(new NodeP<7>);
    case 8:
      return std::unique_ptr<Node>(new NodeP<8>);
    case 9:
      return std::unique_ptr<Node>(new NodeP<9>);
    default:
      return std::unique_ptr<Node>(new Node);
  }
}



#define BENCHMARK_REGISTER_TYPES \
  BENCHMARK_REGISTER_TYPE(Node, NodeP<0>) \
  BENCHMARK_REGISTER_TYPE(Node, NodeP<1>) \
  BENCHMARK_REGISTER_TYPE(Node, NodeP<2>) \
  BENCHMARK_REGISTER_TYPE(Node, NodeP<3>) \
  BENCHMARK_REGISTER_TYPE(Node, NodeP<4>) \
  BENCHMARK_REGISTER_TYPE(Node, NodeP<5>) \
  BENCHMARK_REGISTER_TYPE(Node, NodeP<6>) \
  BENCHMARK_REGISTER_TYPE(Node, NodeP<7>) \
  BENCHMARK_REGISTER_TYPE(Node, NodeP<8>) \
  BENCHMARK_REGISTER_TYPE(Node, NodeP<9>)

class Tree
{
  public:
    void add(std::size_t levels)
    {
      if (0 == levels) {
        return;
      }
      levels_ = levels;
      --levels;
      std::unique_ptr<Node> new_node(new Node);
      root_ = std::move(new_node);
      root_->add(levels);
    }

    std::size_t levels() const
    {
      return levels_;
    }

    std::unique_ptr<Node> root_;
    std::size_t levels_ = 0;
};


template<class Archive>
void serialize(Archive & ar, Tree & l, unsigned int/*version*/)
{
  ar & l.root_;
}

////////////////
// random
///////////////

inline void RandomInit(std::size_t size, Tree & out)
{
  out.add(size);
}


