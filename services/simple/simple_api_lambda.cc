// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <functional>
#include "simple_api.h"

namespace {

// Implementation of the simple:: classes using a capturing lambda created
// on each call, after storing it in an std::function<> variable.
//
// Note: A non-local std::function<> member is needed, otherwise the compiler
//       will optimize out everything into a direct implementation. This is
//       generally good, but the purpose of this version is to measure the
//       cost of trivial lambda capture + storage.
//
//       Note that assigning a lambda into a std::function<> instance
//       actually performs a swap (and destroys the previous instance).

class MyMath : public simple::Math {
 public:
  MyMath() = default;

  int32_t Add(int32_t x, int32_t y) override {
    f_ = [x, y]() { return x + y; };
    return f_();
  }

 private:
  std::function<int32_t()> f_;
};

class MyEcho : public simple::Echo {
 public:
  MyEcho() = default;

  std::string Ping(const std::string& str) override {
    f_ = [&str]() { return str; };
    return f_();
  }

 private:
  std::function<std::string()> f_;
};

}  // namespace

simple::Math* createMath() {
  return new MyMath();
}
simple::Echo* createEcho() {
  return new MyEcho();
}

const char* createAbstract() {
  return "Capture + Store + Invoke a lambda on each method call";
}
