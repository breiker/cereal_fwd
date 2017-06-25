//
// Created by breiker on 6/16/17.
//

#pragma once

#include <memory>

#include "utils.hpp"



class Node
{
  public:
    void add(std::size_t levels, const std::shared_ptr<Node>& this_node)
    {
      if (0 == levels) {
        return;
      }
      --levels;
      left_ = std::make_shared<Node>();
      left_->parent_ = this_node;
      left_->add(levels, left_);
      right_ = std::make_shared<Node>();
      right_->parent_ = this_node;
      right_->add(levels, right_);
    }

  public:
    std::shared_ptr<Node> left_;
    std::shared_ptr<Node> right_;
    std::weak_ptr<Node> parent_;
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
      auto new_node = std::make_shared<Node>();
      root_ = std::move(new_node);
      root_->add(levels, root_);
    }

    std::size_t levels() const
    {
      return levels_;
    }

    std::shared_ptr<Node> root_;
    std::size_t levels_ = 0;
};

template<class Archive>
void serialize(Archive & ar, Node & n, unsigned int/*version*/)
{
  ar & n.left_;
  ar & n.right_;
  ar & n.parent_;
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
