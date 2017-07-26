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
      gfx::Rect crop_rect;
      gfx::Size draw_size;
      CalculateCropRectAndDrawSize(max_image_size_, &crop_rect, &draw_size);

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

  gfx::Rect crop_rect;
  gfx::Size draw_size;
  CalculateCropRectAndDrawSize(GetContentsBounds().size(), &crop_rect,
                               &draw_size);
  if (draw_size.IsEmpty())
    return;

  gfx::ImageSkia image = image_;

  if (crop_rect.size() != image.size()) {
    // Crop to the part of the image that will be visibile.
    image = gfx::ImageSkiaOperations::ExtractSubset(image, crop_rect);
  }

  if (draw_size != image.size()) {
    // Up/downscale the image.
    image = gfx::ImageSkiaOperations::CreateResizedImage(
        image, skia::ImageOperations::RESIZE_BEST, draw_size);
  }

  gfx::Rect draw_bounds = GetContentsBounds();
  draw_bounds.ClampToCenteredSize(draw_size);

  canvas->DrawImageInt(image, draw_bounds.x(), draw_bounds.y());
}

const char* ProportionalImageView::GetClassName() const {
  return kViewClassName;
}

void ProportionalImageView::CalculateCropRectAndDrawSize(
    const gfx::Size& content_size,
    gfx::Rect* out_crop_rect,
    gfx::Size* out_draw_size) {
  DCHECK(out_crop_rect);
  DCHECK(out_draw_size);

  gfx::Size max_size = max_image_size_;
  max_size.SetToMin(content_size);
  if (max_size.IsEmpty() || image_.size().IsEmpty()) {
    DCHECK(out_draw_size->IsEmpty());
    return;
  }

  double aspect_ratio = image_.width() / static_cast<double>(image_.height());

  gfx::Size scaled_size = image_.size();

  // First try fitting to width. Min height of 1 to avoid returning empty size.
  scaled_size.SetSize(max_size.width(),
                      std::max(0.5 + max_size.width() / aspect_ratio, 1.0));

  switch (type_) {
    case Type::FIT_WIDTH:
      // It's ok for these images to be too tall; they'll get cropped below.
      break;
    case Type::CONTAIN:
      if (scaled_size.height() > max_size.height()) {
        // Fit to height instead.
        scaled_size.SetSize(
            std::max(0.5 + max_size.height() * aspect_ratio, 1.0),
            max_size.height());
      }
      break;
  }

  // If type_ is FIT_TO_WIDTH, scaled_size might be taller than max_image_size_.
  *out_draw_size = scaled_size;
  out_draw_size->SetToMin(max_image_size_);

  if (*out_draw_size == scaled_size) {
    *out_crop_rect = gfx::Rect(image_.size());
  } else {
    // Crop the top and bottom of the image.
    DCHECK_EQ(type_, Type::FIT_WIDTH);
    DCHECK_LE(scaled_size.width(), out_draw_size->width())
        << "Only height should exceed out_draw_size";

    // Total amount to crop from the top/bottom of the image.
    float crop_height_fraction =
        (scaled_size.height() - out_draw_size->height()) /
        static_cast<float>(scaled_size.height());
    int crop_height = std::lround(image_.height() * crop_height_fraction);
    int crop_from_top = crop_height / 2;

    *out_crop_rect = gfx::Rect(0, crop_from_top, image_.width(),
                               image_.height() - crop_height);
  }
}

}  // namespace message_center
