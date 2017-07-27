// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/message_center/message_center_style.h"

#include <algorithm>

#include "ui/gfx/color_palette.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/gfx/image/image_skia.h"
#include "ui/gfx/paint_vector_icon.h"
#include "ui/message_center/vector_icons.h"

namespace message_center {

// Within a notification ///////////////////////////////////////////////////////

// Limits.

gfx::Size GetNotificationImageMaxSize(const gfx::Insets& border) {
  int max_image_width = kNotificationWidth - border.left() - border.right();

  // This 3.16:1 aspect ratio is chosen to approximately match common Android
  // devices running Nougat or above.
  constexpr double kMinImageAspectRatio = 3.16;
  int max_image_height =
      static_cast<int>(max_image_width / kMinImageAspectRatio);

  return gfx::Size(max_image_width, max_image_height);
}

// Icons.

gfx::ImageSkia GetCloseIcon() {
  return gfx::CreateVectorIcon(kNotificationCloseButtonIcon,
                               gfx::kChromeIconGrey);
}

gfx::ImageSkia GetSettingsIcon() {
  return gfx::CreateVectorIcon(kNotificationSettingsButtonIcon,
                               gfx::kChromeIconGrey);
}

}  // namespace message_center
