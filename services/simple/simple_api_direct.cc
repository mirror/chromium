// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "simple_api.h"

// Direct implementation of the simple:: classes.

namespace {

class MyMath : public simple::Math {
 public:
  int32_t Add(int32_t x, int32_t y) override { return x + y; }
};

class MyEcho : public simple::Echo {
 public:
  std::string Ping(const std::string& str) override { return str; }
};

}  // namespace

simple::Math* createMath() {
  return new MyMath();
}
simple::Echo* createEcho() {
  return new MyEcho();
}
