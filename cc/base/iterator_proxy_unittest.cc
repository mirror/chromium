// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/base/iterator_proxy.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace cc {
namespace {

class Iterator {
 public:
  explicit Iterator(int start_value) : value_(start_value) {}

  Iterator& operator++() {
    ++value_;
    return *this;
  }
  int operator*() const { return value_; }
  operator bool() const { return value_ < 10; }

 private:
  int value_ = 0;
};

class Container {
 public:
  Container() = default;
  IteratorProxy<Iterator> begin() const { return IteratorProxy<Iterator>(0); }
  IteratorProxy<Iterator> end() const { return IteratorProxy<Iterator>(); }
};

TEST(IteratorProxyTest, BasicIteration) {
  Container c;
  int expected = 0;
  for (int actual : c) {
    EXPECT_EQ(expected, actual);
    ++expected;
  }
  EXPECT_EQ(expected, 10);
}

}  // namespace
}  // namespace cc
