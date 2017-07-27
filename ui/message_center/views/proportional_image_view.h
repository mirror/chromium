// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_MESSAGE_CENTER_VIEWS_PROPORTIONAL_IMAGE_VIEW_H_
#define UI_MESSAGE_CENTER_VIEWS_PROPORTIONAL_IMAGE_VIEW_H_

#include "base/macros.h"
#include "ui/gfx/geometry/size.h"
#include "ui/gfx/image/image_skia.h"
#include "ui/message_center/message_center_export.h"
#include "ui/views/view.h"

namespace message_center {

// ProportionalImageViews scale images then center-crop them if necessary.
class MESSAGE_CENTER_EXPORT ProportionalImageView : public views::View {
 public:
  enum class Type {
    // Up/downscale image to match the width of the view. If the scaled image
    // height is less than |max_image_size_.height()| the view will shrink; else
    // if the height is greater the image will be vertically center-cropped.
    FIT_WIDTH,

    // Up/downscale image to the largest size that is fully contained within the
    // |max_image_size_|, and center it. Never crops image nor shrinks view.
    CONTAIN
  };

  // Internal class name.
  static const char kViewClassName[];

  explicit ProportionalImageView(Type type);
  ~ProportionalImageView() override;

  void SetImage(const gfx::ImageSkia& image,
                const gfx::Size& max_view_size,
                const gfx::Size& max_image_size);

  // Overridden from views::View:
  void OnPaint(gfx::Canvas* canvas) override;
  const char* GetClassName() const override;

 private:
  FRIEND_TEST_ALL_PREFIXES(NotificationViewTest, TestImageSizingFitWidth);
  FRIEND_TEST_ALL_PREFIXES(NotificationViewMDTest, TestImageSizingFitWidth);

  void CalculateCropRectAndDrawSize(const gfx::Size& content_size,
                                    gfx::Rect* out_crop_rect,
                                    gfx::Size* out_draw_size);

  Type type_;
  gfx::ImageSkia image_;
  gfx::Size max_image_size_;

  DISALLOW_COPY_AND_ASSIGN(ProportionalImageView);
};

}  // namespace message_center

#endif // UI_MESSAGE_CENTER_VIEWS_PROPORTIONAL_IMAGE_VIEW_H_
