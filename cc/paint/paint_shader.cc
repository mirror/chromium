// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/paint/paint_shader.h"

#include "base/memory/ptr_util.h"
#include "cc/paint/paint_record.h"
#include "third_party/skia/include/effects/SkGradientShader.h"

namespace cc {

std::unique_ptr<PaintShader> PaintShader::MakeColor(SkColor color) {
  auto shader = base::WrapUnique(new PaintShader(kColor));

  // Just one color. Store it in the fallback color. Easy.
  shader->fallback_color_ = color;

  return shader;
}

std::unique_ptr<PaintShader> PaintShader::MakeLinearGradient(
    const SkPoint points[],
    const SkColor colors[],
    const SkScalar pos[],
    int count,
    SkShader::TileMode mode,
    uint32_t flags,
    const SkMatrix* local_matrix,
    SkColor fallback_color) {
  auto shader = base::WrapUnique(new PaintShader(kLinearGradient));

  // There are always two points, the start and the end.
  shader->start_point_ = points[0];
  shader->end_point_ = points[1];
  // There are |count| number of colors, which is at least 2.
  DCHECK_GE(count, 2);
  shader->colors_.assign(colors, colors + count);
  // There are |count| number of positions, but it could be nullptr.
  if (pos)
    shader->positions_.assign(pos, pos + count);
  shader->tx_ = shader->ty_ = mode;
  // |local_matrix| can be nullptr.
  if (local_matrix)
    shader->local_matrix_ = *local_matrix;
  shader->flags_ = flags;
  shader->fallback_color_ = fallback_color;

  return shader;
}

std::unique_ptr<PaintShader> PaintShader::MakeRadialGradient(
    const SkPoint& center,
    SkScalar radius,
    const SkColor colors[],
    const SkScalar pos[],
    int count,
    SkShader::TileMode mode,
    uint32_t flags,
    const SkMatrix* local_matrix,
    SkColor fallback_color) {
  auto shader = base::WrapUnique(new PaintShader(kRadialGradient));

  shader->center_ = center;
  shader->start_radius_ = shader->end_radius_ = radius;
  // There are |count| number of colors, which is at least 2.
  DCHECK_GE(count, 2);
  shader->colors_.assign(colors, colors + count);
  // There are |count| number of positions, but it could be nullptr.
  if (pos)
    shader->positions_.assign(pos, pos + count);
  shader->tx_ = shader->ty_ = mode;
  shader->flags_ = flags;
  // |local_matrix| can be nullptr.
  if (local_matrix)
    shader->local_matrix_ = *local_matrix;
  shader->flags_ = flags;
  shader->fallback_color_ = fallback_color;

  return shader;
}

std::unique_ptr<PaintShader> PaintShader::MakeTwoPointConicalGradient(
    const SkPoint& start,
    SkScalar start_radius,
    const SkPoint& end,
    SkScalar end_radius,
    const SkColor colors[],
    const SkScalar pos[],
    int count,
    SkShader::TileMode mode,
    uint32_t flags,
    const SkMatrix* local_matrix,
    SkColor fallback_color) {
  auto shader = base::WrapUnique(new PaintShader(kTwoPointConicalGradient));

  shader->start_point_ = start;
  shader->end_point_ = end;
  shader->start_radius_ = start_radius;
  shader->end_radius_ = end_radius;
  // There are |count| number of colors, which is at least 2.
  DCHECK_GE(count, 2);
  shader->colors_.assign(colors, colors + count);
  // There are |count| number of positions, but it could be nullptr.
  if (pos)
    shader->positions_.assign(pos, pos + count);
  shader->tx_ = shader->ty_ = mode;
  // |local_matrix| can be nullptr.
  if (local_matrix)
    shader->local_matrix_ = *local_matrix;
  shader->flags_ = flags;
  shader->fallback_color_ = fallback_color;

  return shader;
}

std::unique_ptr<PaintShader> PaintShader::MakeSweepGradient(
    SkScalar cx,
    SkScalar cy,
    const SkColor colors[],
    const SkScalar pos[],
    int count,
    uint32_t flags,
    const SkMatrix* local_matrix,
    SkColor fallback_color) {
  auto shader = base::WrapUnique(new PaintShader(kTwoPointConicalGradient));

  shader->center_ = SkPoint::Make(cx, cy);
  // There are |count| number of colors, which is at least 2.
  DCHECK_GE(count, 2);
  shader->colors_.assign(colors, colors + count);
  // There are |count| number of positions, but it could be nullptr.
  if (pos)
    shader->positions_.assign(pos, pos + count);
  // |local_matrix| can be nullptr.
  if (local_matrix)
    shader->local_matrix_ = *local_matrix;
  shader->flags_ = flags;
  shader->fallback_color_ = fallback_color;

  return shader;
}

std::unique_ptr<PaintShader> PaintShader::MakeImage(
    sk_sp<const SkImage> image,
    SkShader::TileMode tx,
    SkShader::TileMode ty,
    const SkMatrix* local_matrix) {
  auto shader = base::WrapUnique(new PaintShader(kImage));

  shader->image_ = std::move(image);
  shader->tx_ = tx;
  shader->ty_ = ty;
  // |local_matrix| can be nullptr.
  if (local_matrix)
    shader->local_matrix_ = *local_matrix;

  return shader;
}

std::unique_ptr<PaintShader> PaintShader::MakePaintRecord(
    sk_sp<PaintRecord> record,
    const SkRect& tile,
    SkShader::TileMode tx,
    SkShader::TileMode ty,
    const SkMatrix* local_matrix) {
  auto shader = base::WrapUnique(new PaintShader(kPaintRecord));

  shader->record_ = std::move(record);
  shader->tile_ = tile;
  shader->tx_ = tx;
  shader->ty_ = ty;

  // |local_matrix| can be nullptr.
  if (local_matrix)
    shader->local_matrix_ = *local_matrix;

  return shader;
}

PaintShader::PaintShader(Type type) : shader_type_(type) {}
PaintShader::PaintShader(const PaintShader& other) = default;
PaintShader::PaintShader(PaintShader&& other) = default;
PaintShader::~PaintShader() = default;

PaintShader& PaintShader::operator=(const PaintShader& other) = default;
PaintShader& PaintShader::operator=(PaintShader&& other) = default;

sk_sp<SkShader> PaintShader::GetSkShader() const {
  if (cached_shader_)
    return cached_shader_;

  switch (shader_type_) {
    case kColor:
      // This will be handled by the fallback check below.
      break;
    case kLinearGradient: {
      SkPoint points[2] = {start_point_, end_point_};
      cached_shader_ = SkGradientShader::MakeLinear(
          points, colors_.data(),
          positions_.empty() ? nullptr : positions_.data(), colors_.size(), tx_,
          flags_, local_matrix_ ? &*local_matrix_ : nullptr);
      break;
    }
    case kRadialGradient:
      cached_shader_ = SkGradientShader::MakeRadial(
          center_, start_radius_, colors_.data(),
          positions_.empty() ? nullptr : positions_.data(), colors_.size(), tx_,
          flags_, local_matrix_ ? &*local_matrix_ : nullptr);
      break;
    case kTwoPointConicalGradient:
      cached_shader_ = SkGradientShader::MakeTwoPointConical(
          start_point_, start_radius_, end_point_, end_radius_, colors_.data(),
          positions_.empty() ? nullptr : positions_.data(), colors_.size(), tx_,
          flags_, local_matrix_ ? &*local_matrix_ : nullptr);
      break;
    case kSweepGradient:
      cached_shader_ = SkGradientShader::MakeSweep(
          center_.x(), center_.y(), colors_.data(),
          positions_.empty() ? nullptr : positions_.data(), colors_.size(),
          flags_, local_matrix_ ? &*local_matrix_ : nullptr);
      break;
    case kImage:
      cached_shader_ = image_->makeShader(
          tx_, ty_, local_matrix_ ? &*local_matrix_ : nullptr);
      break;
    case kPaintRecord:
      cached_shader_ = SkShader::MakePictureShader(
          ToSkPicture(record_, tile_), tx_, ty_,
          local_matrix_ ? &*local_matrix_ : nullptr, nullptr);
      break;
    case kShaderCount:
      NOTREACHED();
      break;
  }

  // If we didn't create a shader for whatever reason, create a fallback color
  // one.
  if (!cached_shader_)
    cached_shader_ = SkShader::MakeColorShader(fallback_color_);
  return cached_shader_;
}

}  // namespace cc
