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

// ProportionalImageViews scale images to kNotificationImageWidth then
// vertically center-crops them if necessary.
class MESSAGE_CENTER_EXPORT ProportionalImageView : public views::View {
 public:
  enum Type { FIT_WIDTH, CONTAIN };

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

  void CalcCropRectAndDrawSize(const gfx::Size& content_size,
                               gfx::Rect* out_crop_rect,
                               gfx::Size* out_draw_size);

 private:
  FRIEND_TEST_ALL_PREFIXES(NotificationViewTest, TestImageSizingFitWidth);
  FRIEND_TEST_ALL_PREFIXES(NotificationViewMDTest, TestImageSizingFitWidth);

  Type type_;
  gfx::ImageSkia image_;
  gfx::Size max_image_size_;

  DISALLOW_COPY_AND_ASSIGN(ProportionalImageView);
};

}  // namespace message_center

#endif // UI_MESSAGE_CENTER_VIEWS_PROPORTIONAL_IMAGE_VIEW_H_
