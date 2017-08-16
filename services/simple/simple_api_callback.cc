// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/bind.h"
#include "base/callback.h"
#include "simple_api.h"

namespace {

// Implementation of the simple:: classes using a simple callback call.
class MyMath : public simple::Math {
 public:
  MyMath() : callback_(base::Bind(&DoAdd)) {}

  int32_t Add(int32_t x, int32_t y) override { return callback_.Run(x, y); }

  static int32_t DoAdd(int32_t x, int32_t y) { return x + y; }

 private:
  base::Callback<int32_t(int32_t, int32_t)> callback_;
};

class MyEcho : public simple::Echo {
 public:
  MyEcho() : callback_(base::Bind(&DoPing)) {}

  std::string Ping(const std::string& str) override {
    return callback_.Run(str);
  }

  static std::string DoPing(const std::string& str) { return str; }

 private:
  base::Callback<std::string(const std::string&)> callback_;
};

}  // namespace

simple::Math* createMath() {
  return new MyMath();
}
simple::Echo* createEcho() {
  return new MyEcho();
}
