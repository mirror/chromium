// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/paint/scoped_image_flags.h"

#include "cc/paint/image_provider.h"
#include "cc/paint/paint_image_builder.h"

namespace cc {
namespace {
SkIRect RoundOutRect(const SkRect& rect) {
  SkIRect result;
  rect.roundOut(&result);
  return result;
}
}  // namespace

ScopedImageFlags::ScopedImageFlags(ImageProvider* image_provider,
                                   DecodedImageStash* decoded_image_stash,
                                   const PaintFlags& flags,
                                   const SkMatrix& ctm) {
  if (!decoded_image_stash) {
    owned_decoded_image_stash_.emplace();
    decoded_image_stash = &owned_decoded_image_stash_.value();
  }

  if (flags.getShader()->shader_type() == PaintShader::Type::kImage) {
    DecodeImageShader(image_provider, decoded_image_stash, flags, ctm);
  } else {
    DCHECK_EQ(flags.getShader()->shader_type(),
              PaintShader::Type::kPaintRecord);
    DecodeRecordShader(image_provider, decoded_image_stash, flags, ctm);
  }
}

ScopedImageFlags::~ScopedImageFlags() = default;

void ScopedImageFlags::DecodeImageShader(ImageProvider* image_provider,
                                         DecodedImageStash* decoded_image_stash,
                                         const PaintFlags& flags,
                                         const SkMatrix& ctm) {
  const PaintImage& paint_image = flags.getShader()->paint_image();
  SkMatrix matrix = flags.getShader()->GetLocalMatrix();

  SkMatrix total_image_matrix = matrix;
  total_image_matrix.preConcat(ctm);
  SkRect src_rect = SkRect::MakeIWH(paint_image.width(), paint_image.height());
  DrawImage draw_image(paint_image, RoundOutRect(src_rect),
                       flags.getFilterQuality(), total_image_matrix);
  auto decoded_draw_image = image_provider->GetDecodedDrawImage(draw_image);

  if (!decoded_draw_image)
    return;

  decoded_image_stash->AddDecodedImage(std::move(decoded_draw_image));

  const auto& decoded_image = decoded_draw_image.decoded_image();
  DCHECK(decoded_image.image());

  bool need_scale = !decoded_image.is_scale_adjustment_identity();
  if (need_scale) {
    matrix.preScale(1.f / decoded_image.scale_adjustment().width(),
                    1.f / decoded_image.scale_adjustment().height());
  }

  sk_sp<SkImage> sk_image =
      sk_ref_sp<SkImage>(const_cast<SkImage*>(decoded_image.image().get()));
  PaintImage decoded_paint_image = PaintImageBuilder()
                                       .set_id(paint_image.stable_id())
                                       .set_image(std::move(sk_image))
                                       .TakePaintImage();
  decoded_flags_.emplace(flags);
  decoded_flags_.value().setFilterQuality(decoded_image.filter_quality());
  decoded_flags_.value().setShader(
      PaintShader::MakeImage(decoded_paint_image, flags.getShader()->tx(),
                             flags.getShader()->ty(), &matrix));
}

void ScopedImageFlags::DecodeRecordShader(
    ImageProvider* image_provider,
    DecodedImageStash* decoded_image_stash,
    const PaintFlags& flags,
    const SkMatrix& ctm) {
  auto decoded_shader = flags.getShader()->CreateDecodedPaintRecord(
      ctm, image_provider, decoded_image_stash);
  if (!decoded_shader)
    return;

  decoded_flags_.emplace(flags);
  decoded_flags_.value().setShader(std::move(decoded_shader));
}

}  // namespace cc
