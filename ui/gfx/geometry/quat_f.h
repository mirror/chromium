// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_GFX_QUAT_F_
#define UI_GFX_QUAT_F_

#include <string>

#include "ui/gfx/gfx_export.h"

namespace gfx {

class Vector3dF;

class GFX_EXPORT QuatF {
 public:
  constexpr QuatF() = default;
  constexpr QuatF(float x, float y, float z, float w)
      : x_(x), y_(y), z_(z), w_(w) {}
  QuatF(const Vector3dF& axis, float angle);

  constexpr float x() const { return x_; }
  void set_x(float x) { x_ = x; }

  constexpr float y() const { return y_; }
  void set_y(float y) { y_ = y; }

  constexpr float z() const { return z_; }
  void set_z(float z) { z_ = z; }

  constexpr float w() const { return w_; }
  void set_w(float w) { w_ = w; }

  QuatF operator+(const QuatF& q) const {
    return {q.x_ + x_, q.y_ + y_, q.z_ + z_, q.w_ + w_};
  }

  QuatF operator*(const QuatF& q) const {
    return {q.w_ * x_ + q.x_ * w_ + q.y_ * z_ - q.z_ * y_,
            q.w_ * y_ - q.x_ * z_ + q.y_ * w_ + q.z_ * x_,
            q.w_ * z_ + q.x_ * y_ - q.y_ * x_ + q.z_ * w_,
            q.w_ * w_ - q.x_ * x_ - q.y_ * y_ - q.z_ * z_};
  }

  QuatF inverse() const { return {-x_, -y_, -z_, w_}; }

  // Blends with the given quaternion, |q|, via spherical linear interpolation.
  QuatF Slerp(const QuatF& q, float t) const;

  // Blends with the given quaternion, |q|, via linear interpolation. This is
  // rarely what you want. Use only if you know what you're doing.
  QuatF Lerp(const QuatF& q, float t) const;

  std::string ToString() const;

 private:
  float x_ = 0.0f;
  float y_ = 0.0f;
  float z_ = 0.0f;
  float w_ = 1.0f;
};

inline QuatF operator*(const QuatF& q, float s) {
  return QuatF(q.x() * s, q.y() * s, q.z() * s, q.w() * s);
}

inline QuatF operator*(float s, const QuatF& q) {
  return QuatF(q.x() * s, q.y() * s, q.z() * s, q.w() * s);
}

}  // namespace gfx

#endif  // UI_GFX_QUAT_F_
