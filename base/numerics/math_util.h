// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BASE_NUMERICS_MATH_UTIL_H_
#define BASE_NUMERICS_MATH_UTIL_H_

#include <algorithm>

namespace base {

constexpr double kPiDouble = 3.14159265358979323846;
constexpr float kPiFloat = 3.14159265358979323846f;

constexpr double DegToRad(double deg) {
  return deg * kPiDouble / 180.0;
}
constexpr float DegToRad(float deg) {
  return deg * kPiFloat / 180.0f;
}

constexpr double RadToDeg(double rad) {
  return rad * 180.0 / kPiDouble;
}
constexpr float RadToDeg(float rad) {
  return rad * 180.0f / kPiFloat;
}

template <typename T>
T ClampToRange(T value, T min, T max) {
  return std::min(std::max(value, min), max);
}

}  // namespace base

#endif  // BASE_NUMERICS_MATH_UTIL_H_