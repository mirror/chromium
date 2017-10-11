// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CounterValue_h
#define CounterValue_h

#include <limits>

namespace blink {

// Represents a counter value that does not overflow or underflow when
// incremented.
class CounterValue {
 public:
  CounterValue(int value) : value_(value) {}

  operator int() const { return value_; }

  // According to the spec, if an increment would overflow or underflow the
  // counter, we are allowed to ignore the increment.
  // https://drafts.csswg.org/css-lists-3/#valdef-counter-reset-custom-ident-integer
  int operator+(int increment) const {
    if (value_ > 0 && increment > std::numeric_limits<int>::max() - value_)
      return value_;
    if (value_ < 0 && increment < std::numeric_limits<int>::min() - value_)
      return value_;
    return value_ + increment;  // Cannot overflow.
  }

 private:
  int value_;
};

}  // namespace blink

#endif
