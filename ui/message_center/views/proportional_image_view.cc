// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/message_center/views/proportional_image_view.h"

#include "ui/gfx/canvas.h"
#include "ui/gfx/image/image_skia_operations.h"
#include "ui/message_center/message_center_style.h"

namespace message_center {

const char ProportionalImageView::kViewClassName[] = "ProportionalImageView";

ProportionalImageView::ProportionalImageView(Type type) : type_(type) {}

ProportionalImageView::~ProportionalImageView() = default;

void ProportionalImageView::SetImage(const gfx::ImageSkia& image,
                                     const gfx::Size& max_view_size,
                                     const gfx::Size& max_image_size) {
  image_ = image;
  max_image_size_ = max_image_size;

  switch (type_) {
    case Type::FIT_WIDTH: {
      gfx::Size scaled_size = CalcScaledImageSize(max_image_size_);

      // scaled_size might be taller than max_image_size_.
      gfx::Size draw_size = scaled_size;
      draw_size.SetToMin(max_image_size_);

      int border_width_sum = max_view_size.width() - max_image_size_.width();
      int border_height_sum = max_view_size.height() - max_image_size_.height();
      gfx::Size view_size(draw_size.width() + border_width_sum,
                          draw_size.height() + border_height_sum);
      SetPreferredSize(view_size);
      break;
    }
    case Type::CONTAIN: {
      SetPreferredSize(max_view_size);
      break;
    }
  }

  SchedulePaint();
}

void ProportionalImageView::OnPaint(gfx::Canvas* canvas) {
  views::View::OnPaint(canvas);

  if (!visible())
    return;

  gfx::Size scaled_size = CalcScaledImageSize(GetContentsBounds().size());

  // If type_ is FIT_TO_WIDTH, scaled_size might be taller than max_image_size_.
  gfx::Size draw_size = scaled_size;
  draw_size.SetToMin(max_image_size_);
  if (draw_size.IsEmpty())
    return;

  gfx::Rect draw_bounds = GetContentsBounds();
  draw_bounds.ClampToCenteredSize(draw_size);

  gfx::ImageSkia image = image_;

  if (scaled_size != draw_size) {
    // Aspect ratio of FIT_TO_WIDTH image is too tall. Crop the top and bottom
    // of the image, before downscaling (hopefully that's faster).
    DCHECK_EQ(type_, Type::FIT_WIDTH);
    DCHECK_LE(scaled_size.width(), draw_size.width())
        << "Only height should exceed draw_size";

    // Total amount to crop from the top/bottom of the image.
    float crop_height_fraction = (scaled_size.height() - draw_size.height()) /
                                 static_cast<float>(scaled_size.height());
    int crop_height = std::lround(image.height() * crop_height_fraction);
    int crop_from_top = crop_height / 2;
    image = gfx::ImageSkiaOperations::ExtractSubset(
        image, gfx::Rect(0, crop_from_top, image.width(),
                         image.height() - crop_height));
  }

  if (image.size() != draw_size) {
    // Image needs downscaling.
    image = gfx::ImageSkiaOperations::CreateResizedImage(
        image, skia::ImageOperations::RESIZE_BEST, draw_size);
  }

  canvas->DrawImageInt(image, draw_bounds.x(), draw_bounds.y());
}

const char* ProportionalImageView::GetClassName() const {
  return kViewClassName;
}

gfx::Size ProportionalImageView::CalcScaledImageSize(
    const gfx::Size& content_size) {
  gfx::Size max_size = max_image_size_;
  max_size.SetToMin(content_size);
  if (max_size.IsEmpty() || image_.size().IsEmpty())
    return gfx::Size();

  gfx::Size scaled_size = image_.size();

  double aspect_ratio =
      scaled_size.width() / static_cast<double>(scaled_size.height());

  // First try fitting to width. Min height of 1 to avoid returning empty size.
  scaled_size.SetSize(max_size.width(),
                      std::max(0.5 + max_size.width() / aspect_ratio, 1.0));

  // If type_ is FIT_WIDTH, then it's ok for the image to be taller than
  // max_size. If instead type_ is CONTAIN, check if it's too tall, and if so
  // fit to height instead.
  if (type_ == Type::CONTAIN && scaled_size.height() > max_size.height()) {
    scaled_size.SetSize(std::max(0.5 + max_size.height() * aspect_ratio, 1.0),
                        max_size.height());
  }

  return scaled_size;
}

}  // namespace message_center
