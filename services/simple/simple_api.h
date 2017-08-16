// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_SIMPLE_SIMPLE_API_H
#define SERVICES_SIMPLE_SIMPLE_API_H

#include "simple_export.h"

#include <stdint.h>
#include <string>

// This header defines two abstract C++ interfaces that can be implemented
// differently by shared libraries (see simple::Math and simple::Echo
// below).
//
// Each shared library must export createXXX() symbols as declared below
// and must be compiled with the SIMPLE_API_IMPLEMENTATION macro defined.

namespace simple {

// Abstract base class for math operations.
class Math {
 public:
  virtual ~Math() = default;

  // Add two 32-bit integers and return their sum as the result.
  virtual int32_t Add(int32_t x, int32_t y) = 0;
};

class Echo {
 public:
  virtual ~Echo() = default;

  // Take an input string reference and return its unmodified copy.
  virtual std::string Ping(const std::string& str) = 0;
};

}  // namespace simple

// Each shared library should provide these entry points.
SIMPLE_EXPORT extern "C" simple::Math* createMath();
SIMPLE_EXPORT extern "C" simple::Echo* createEcho();

#endif  // SERVICES_SIMPLE_SIMPLE_API_H
