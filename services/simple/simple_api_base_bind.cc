// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/bind.h"
#include "base/callback.h"
#include "simple_api.h"

#if !defined(USE_BIND_ONCE) && !defined(USE_BIND_REPEATING)
#error \
    "One of these macros must be defined: USE_BIND_ONCE or USE_BIND_REPEATING"
#endif

namespace {

// Implementation of the simple:: classes using a simple base::OnceCallback
// during each call. This is mostly used to measure the cost of creating
// movable callbacks.

class MyMath : public simple::Math {
 public:
  MyMath() = default;

  int32_t Add(int32_t x, int32_t y) override {
#if defined(USE_BIND_ONCE)
    auto callback = base::BindOnce(&MyMath::DoAdd, x, y);
    return std::move(callback).Run();
#elif defined(USE_BIND_REPEATING)
    auto callback = base::BindRepeating(&MyMath::DoAdd, x, y);
    return callback.Run();
#endif
  }

 private:
  static int32_t DoAdd(int32_t x, int32_t y) { return x + y; }
};

class MyEcho : public simple::Echo {
 public:
  MyEcho() = default;

  std::string Ping(const std::string& str) override {
#if defined(USE_BIND_ONCE)
    auto callback = base::BindOnce(&MyEcho::DoPing, base::ConstRef(str));
    return std::move(callback).Run();
#elif defined(USE_BIND_REPEATING)
    auto callback = base::BindRepeating(&MyEcho::DoPing, base::ConstRef(str));
    return callback.Run();
#endif
  }

  static std::string DoPing(const std::string& str) { return str; }
};

}  // namespace

simple::Math* createMath() {
  return new MyMath();
}
simple::Echo* createEcho() {
  return new MyEcho();
}
const char* createAbstract() {
#if defined(USE_BIND_ONCE)
  return "base::BindOnce() + Run() on each method call.";
#elif defined(USE_BIND_REPEATING)
  return "base::BindRepeating() + Run() on each method call.";
#endif
}
