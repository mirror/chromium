// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_GFX_MATH_UTIL_H_
#define UI_GFX_MATH_UTIL_H_

#include <algorithm>
#include <cmath>
#include <memory>
#include <vector>

#include "base/logging.h"
#include "build/build_config.h"
#include "ui/gfx/geometry/box_f.h"
#include "ui/gfx/geometry/point3_f.h"
#include "ui/gfx/geometry/point_f.h"
#include "ui/gfx/geometry/scroll_offset.h"
#include "ui/gfx/geometry/size.h"
#include "ui/gfx/gfx_export.h"
#include "ui/gfx/transform.h"

namespace base {
class Value;
namespace trace_event {
class TracedValue;
}
}  // namespace base

namespace gfx {
class QuadF;
class Rect;
class RectF;
class SizeF;
class Transform;
class Vector2dF;
class Vector2d;
class Vector3dF;

struct HomogeneousCoordinate {
  HomogeneousCoordinate(SkMScalar x, SkMScalar y, SkMScalar z, SkMScalar w) {
    vec[0] = x;
    vec[1] = y;
    vec[2] = z;
    vec[3] = w;
  }

  bool ShouldBeClipped() const { return w() <= 0.0; }

  PointF CartesianPoint2d() const {
    if (w() == SK_MScalar1)
      return PointF(x(), y());

    // For now, because this code is used privately only by MathUtil, it should
    // never be called when w == 0, and we do not yet need to handle that case.
    DCHECK(w());
    SkMScalar inv_w = SK_MScalar1 / w();
    return PointF(x() * inv_w, y() * inv_w);
  }

  Point3F CartesianPoint3d() const {
    if (w() == SK_MScalar1)
      return Point3F(x(), y(), z());

    // For now, because this code is used privately only by MathUtil, it should
    // never be called when w == 0, and we do not yet need to handle that case.
    DCHECK(w());
    SkMScalar inv_w = SK_MScalar1 / w();
    return Point3F(x() * inv_w, y() * inv_w, z() * inv_w);
  }

  SkMScalar x() const { return vec[0]; }
  SkMScalar y() const { return vec[1]; }
  SkMScalar z() const { return vec[2]; }
  SkMScalar w() const { return vec[3]; }

  SkMScalar vec[4];
};

class GFX_EXPORT MathUtil {
 public:
  static const double kPiDouble;
  static const float kPiFloat;

  static double Deg2Rad(double deg) { return deg * kPiDouble / 180.0; }
  static double Rad2Deg(double rad) { return rad * 180.0 / kPiDouble; }

  static float Deg2Rad(float deg) { return deg * kPiFloat / 180.0f; }
  static float Rad2Deg(float rad) { return rad * 180.0f / kPiFloat; }

  static float Round(float f) {
    return (f > 0.f) ? std::floor(f + 0.5f) : std::ceil(f - 0.5f);
  }
  static double Round(double d) {
    return (d > 0.0) ? std::floor(d + 0.5) : std::ceil(d - 0.5);
  }

  // Returns true if rounded up value does not overflow, false otherwise.
  template <typename T>
  static bool VerifyRoundup(T n, T mul) {
    return mul && (n <= (std::numeric_limits<T>::max() -
                         (std::numeric_limits<T>::max() % mul)));
  }

  // Rounds up a given |n| to be a multiple of |mul|, but may overflow.
  // Examples:
  //    - RoundUp(123, 50) returns 150.
  //    - RoundUp(-123, 50) returns -100.
  template <typename T>
  static T UncheckedRoundUp(T n, T mul) {
    static_assert(std::numeric_limits<T>::is_integer,
                  "T must be an integer type");
    DCHECK(VerifyRoundup(n, mul));
    return RoundUpInternal(n, mul);
  }

  // Similar to UncheckedRoundUp(), but dies with a CRASH() if rounding up a
  // given |n| overflows T.
  template <typename T>
  static T CheckedRoundUp(T n, T mul) {
    static_assert(std::numeric_limits<T>::is_integer,
                  "T must be an integer type");
    CHECK(VerifyRoundup(n, mul));
    return RoundUpInternal(n, mul);
  }

  // Returns true if rounded down value does not underflow, false otherwise.
  template <typename T>
  static bool VerifyRoundDown(T n, T mul) {
    return mul && (n >= (std::numeric_limits<T>::min() -
                         (std::numeric_limits<T>::min() % mul)));
  }

  // Rounds down a given |n| to be a multiple of |mul|, but may underflow.
  // Examples:
  //    - RoundDown(123, 50) returns 100.
  //    - RoundDown(-123, 50) returns -150.
  template <typename T>
  static T UncheckedRoundDown(T n, T mul) {
    static_assert(std::numeric_limits<T>::is_integer,
                  "T must be an integer type");
    DCHECK(VerifyRoundDown(n, mul));
    return RoundDownInternal(n, mul);
  }

  // Similar to UncheckedRoundDown(), but dies with a CRASH() if rounding down a
  // given |n| underflows T.
  template <typename T>
  static T CheckedRoundDown(T n, T mul) {
    static_assert(std::numeric_limits<T>::is_integer,
                  "T must be an integer type");
    CHECK(VerifyRoundDown(n, mul));
    return RoundDownInternal(n, mul);
  }

  template <typename T>
  static T ClampToRange(T value, T min, T max) {
    return std::min(std::max(value, min), max);
  }

  template <typename T>
  static bool ApproximatelyEqual(T lhs, T rhs, T tolerance) {
    DCHECK_LE(0, tolerance);
    return std::abs(rhs - lhs) <= tolerance;
  }

  template <typename T>
  static bool IsWithinEpsilon(T a, T b) {
    return std::abs(a - b) < std::numeric_limits<T>::epsilon();
  }

  // Background: Existing transform code does not do the right thing in
  // MapRect / MapQuad / ProjectQuad when there is a perspective projection that
  // causes one of the transformed vertices to go to w < 0. In those cases, it
  // is necessary to perform clipping in homogeneous coordinates, after applying
  // the transform, before dividing-by-w to convert to cartesian coordinates.
  //
  // These functions return the axis-aligned rect that encloses the correctly
  // clipped, transformed polygon.
  static Rect MapEnclosingClippedRect(const Transform& transform,
                                      const Rect& rect);
  static RectF MapClippedRect(const Transform& transform, const RectF& rect);
  static Rect ProjectEnclosingClippedRect(const Transform& transform,
                                          const Rect& rect);
  static RectF ProjectClippedRect(const Transform& transform,
                                  const RectF& rect);

  // This function is only valid when the transform preserves 2d axis
  // alignment and the resulting rect will not be clipped.
  static Rect MapEnclosedRectWith2dAxisAlignedTransform(
      const Transform& transform,
      const Rect& rect);

  // Returns an array of vertices that represent the clipped polygon. After
  // returning, indexes from 0 to num_vertices_in_clipped_quad are valid in the
  // clipped_quad array. Note that num_vertices_in_clipped_quad may be zero,
  // which means the entire quad was clipped, and none of the vertices in the
  // array are valid.
  static bool MapClippedQuad3d(const Transform& transform,
                               const QuadF& src_quad,
                               Point3F clipped_quad[6],
                               int* num_vertices_in_clipped_quad);

  static RectF ComputeEnclosingRectOfVertices(const PointF vertices[],
                                              int num_vertices);
  static RectF ComputeEnclosingClippedRect(const HomogeneousCoordinate& h1,
                                           const HomogeneousCoordinate& h2,
                                           const HomogeneousCoordinate& h3,
                                           const HomogeneousCoordinate& h4);

  // NOTE: These functions do not do correct clipping against w = 0 plane, but
  // they correctly detect the clipped condition via the boolean clipped.
  static QuadF MapQuad(const Transform& transform,
                       const QuadF& quad,
                       bool* clipped);
  static PointF MapPoint(const Transform& transform,
                         const PointF& point,
                         bool* clipped);
  static PointF ProjectPoint(const Transform& transform,
                             const PointF& point,
                             bool* clipped);
  // Identical to the above function, but coerces the homogeneous coordinate to
  // a 3d rather than a 2d point.
  static Point3F ProjectPoint3D(const Transform& transform,
                                const PointF& point,
                                bool* clipped);

  static Vector2dF ComputeTransform2dScaleComponents(const Transform&,
                                                     float fallbackValue);
  // Returns an approximate max scale value of the transform even if it has
  // perspective. Prefer to use ComputeTransform2dScaleComponents if there is no
  // perspective, since it can produce more accurate results.
  static float ComputeApproximateMaxScale(const Transform& transform);

  // Makes a rect that has the same relationship to input_outer_rect as
  // scale_inner_rect has to scale_outer_rect. scale_inner_rect should be
  // contained within scale_outer_rect, and likewise the rectangle that is
  // returned will be within input_outer_rect at a similar relative, scaled
  // position.
  static RectF ScaleRectProportional(const RectF& input_outer_rect,
                                     const RectF& scale_outer_rect,
                                     const RectF& scale_inner_rect);

  // Returns the smallest angle between the given two vectors in degrees.
  // Neither vector is assumed to be normalized.
  static float SmallestAngleBetweenVectors(const Vector2dF& v1,
                                           const Vector2dF& v2);

  // Projects the |source| vector onto |destination|. Neither vector is assumed
  // to be normalized.
  static Vector2dF ProjectVector(const Vector2dF& source,
                                 const Vector2dF& destination);

  // Conversion to value.
  static std::unique_ptr<base::Value> AsValue(const Size& s);
  static std::unique_ptr<base::Value> AsValue(const Rect& r);
  static bool FromValue(const base::Value*, Rect* out_rect);
  static std::unique_ptr<base::Value> AsValue(const PointF& q);

  static void AddToTracedValue(const char* name,
                               const Size& s,
                               base::trace_event::TracedValue* res);
  static void AddToTracedValue(const char* name,
                               const SizeF& s,
                               base::trace_event::TracedValue* res);
  static void AddToTracedValue(const char* name,
                               const Rect& r,
                               base::trace_event::TracedValue* res);
  static void AddToTracedValue(const char* name,
                               const Point& q,
                               base::trace_event::TracedValue* res);
  static void AddToTracedValue(const char* name,
                               const PointF& q,
                               base::trace_event::TracedValue* res);
  static void AddToTracedValue(const char* name,
                               const Point3F&,
                               base::trace_event::TracedValue* res);
  static void AddToTracedValue(const char* name,
                               const Vector2d& v,
                               base::trace_event::TracedValue* res);
  static void AddToTracedValue(const char* name,
                               const Vector2dF& v,
                               base::trace_event::TracedValue* res);
  static void AddToTracedValue(const char* name,
                               const ScrollOffset& v,
                               base::trace_event::TracedValue* res);
  static void AddToTracedValue(const char* name,
                               const QuadF& q,
                               base::trace_event::TracedValue* res);
  static void AddToTracedValue(const char* name,
                               const RectF& rect,
                               base::trace_event::TracedValue* res);
  static void AddToTracedValue(const char* name,
                               const Transform& transform,
                               base::trace_event::TracedValue* res);
  static void AddToTracedValue(const char* name,
                               const BoxF& box,
                               base::trace_event::TracedValue* res);

  // Returns a base::Value representation of the floating point value.
  // If the value is inf, returns max double/float representation.
  static double AsDoubleSafely(double value);
  static float AsFloatSafely(float value);

  // Returns vector that x axis (1,0,0) transforms to under given transform.
  static Vector3dF GetXAxis(const Transform& transform);

  // Returns vector that y axis (0,1,0) transforms to under given transform.
  static Vector3dF GetYAxis(const Transform& transform);

  static bool IsFloatNearlyTheSame(float left, float right);
  static bool IsNearlyTheSameForTesting(const PointF& l, const PointF& r);
  static bool IsNearlyTheSameForTesting(const Point3F& l, const Point3F& r);

 private:
  template <typename T>
  static T RoundUpInternal(T n, T mul) {
    return (n > 0) ? ((n + mul - 1) / mul) * mul : (n / mul) * mul;
  }

  template <typename T>
  static T RoundDownInternal(T n, T mul) {
    return (n > 0) ? (n / mul) * mul
                   : (n == 0) ? 0 : ((n - mul + 1) / mul) * mul;
  }
};

class GFX_EXPORT ScopedSubnormalFloatDisabler {
 public:
  ScopedSubnormalFloatDisabler();
  ~ScopedSubnormalFloatDisabler();

 private:
#if defined(ARCH_CPU_X86_FAMILY)
  unsigned int orig_state_;
#endif
  DISALLOW_COPY_AND_ASSIGN(ScopedSubnormalFloatDisabler);
};

}  // namespace gfx

#endif  // UI_GFX_MATH_UTIL_H_
