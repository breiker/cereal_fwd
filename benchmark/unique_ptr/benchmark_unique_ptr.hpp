//
// Created by breiker on 6/16/17.
//

#pragma once

#include <functional>
#include <iostream>
#include <memory>

#include "utils.hpp"

class Node
{
  public:
    void add(std::size_t levels)
    {
      if (0 == levels) {
        return;
      }
      --levels;
      left_ = std::unique_ptr<Node>(new Node);
      left_->add(levels);
      right_ = std::unique_ptr<Node>(new Node);
      right_->add(levels);
    }

  public:
    std::unique_ptr<Node> left_;
    std::unique_ptr<Node> right_;
};

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
void serialize(Archive & ar, Node & n, unsigned int/*version*/)
{
  ar & n.left_;
  ar & n.right_;
}

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

