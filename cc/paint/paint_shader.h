// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CC_PAINT_PAINT_SHADER_H_
#define CC_PAINT_PAINT_SHADER_H_

#include <memory>
#include <vector>

#include "base/optional.h"
#include "cc/paint/paint_export.h"
#include "third_party/skia/include/core/SkImage.h"
#include "third_party/skia/include/core/SkScalar.h"
#include "third_party/skia/include/core/SkShader.h"

namespace cc {

class PaintOpBuffer;
using PaintRecord = PaintOpBuffer;

class CC_PAINT_EXPORT PaintShader {
 public:
  static std::unique_ptr<PaintShader> MakeColor(SkColor color);

  static std::unique_ptr<PaintShader> MakeLinearGradient(
      const SkPoint* points,
      const SkColor* colors,
      const SkScalar* pos,
      int count,
      SkShader::TileMode mode,
      uint32_t flags = 0,
      const SkMatrix* local_matrix = nullptr,
      SkColor fallback_color = SK_ColorTRANSPARENT);

  static std::unique_ptr<PaintShader> MakeRadialGradient(
      const SkPoint& center,
      SkScalar radius,
      const SkColor colors[],
      const SkScalar pos[],
      int color_count,
      SkShader::TileMode mode,
      uint32_t flags = 0,
      const SkMatrix* local_matrix = nullptr,
      SkColor fallback_color = SK_ColorTRANSPARENT);

  static std::unique_ptr<PaintShader> MakeTwoPointConicalGradient(
      const SkPoint& start,
      SkScalar start_radius,
      const SkPoint& end,
      SkScalar end_radius,
      const SkColor colors[],
      const SkScalar pos[],
      int color_count,
      SkShader::TileMode mode,
      uint32_t flags = 0,
      const SkMatrix* local_matrix = nullptr,
      SkColor fallback_color = SK_ColorTRANSPARENT);

  static std::unique_ptr<PaintShader> MakeSweepGradient(
      SkScalar cx,
      SkScalar cy,
      const SkColor colors[],
      const SkScalar pos[],
      int color_count,
      uint32_t flags = 0,
      const SkMatrix* local_matrix = nullptr,
      SkColor fallback_color = SK_ColorTRANSPARENT);

  static std::unique_ptr<PaintShader> MakeImage(sk_sp<const SkImage> image,
                                                SkShader::TileMode tx,
                                                SkShader::TileMode ty,
                                                const SkMatrix* local_matrix);

  static std::unique_ptr<PaintShader> MakePaintRecord(
      sk_sp<PaintRecord> record,
      const SkRect& tile,
      SkShader::TileMode tx,
      SkShader::TileMode ty,
      const SkMatrix* local_matrix);

  PaintShader(const PaintShader& other);
  PaintShader(PaintShader&& other);
  ~PaintShader();

  PaintShader& operator=(const PaintShader& other);
  PaintShader& operator=(PaintShader&& other);

 private:
  friend class PaintFlags;

  enum Type {
    kColor,
    kLinearGradient,
    kRadialGradient,
    kTwoPointConicalGradient,
    kSweepGradient,
    kImage,
    kPaintRecord,
    kShaderCount
  };

  explicit PaintShader(Type type);

  sk_sp<SkShader> GetSkShader() const;

  Type shader_type_ = kShaderCount;

  uint32_t flags_ = 0;
  SkScalar end_radius_ = 0;
  SkScalar start_radius_ = 0;
  SkShader::TileMode tx_ = SkShader::kClamp_TileMode;
  SkShader::TileMode ty_ = SkShader::kClamp_TileMode;
  SkColor fallback_color_ = SK_ColorTRANSPARENT;

  base::Optional<SkMatrix> local_matrix_;
  SkPoint center_ = SkPoint::Make(0, 0);
  SkRect tile_ = SkRect::MakeEmpty();

  SkPoint start_point_ = SkPoint::Make(0, 0);
  SkPoint end_point_ = SkPoint::Make(0, 0);

  sk_sp<const SkImage> image_;
  sk_sp<PaintRecord> record_;

  std::vector<SkColor> colors_;
  std::vector<SkScalar> positions_;

  mutable sk_sp<SkShader> cached_shader_;
};

}  // namespace cc

#endif  // CC_PAINT_PAINT_SHADER_H_
