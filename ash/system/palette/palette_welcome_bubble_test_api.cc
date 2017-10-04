// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/system/palette/palette_welcome_bubble_test_api.h"

#include "ash/system/palette/palette_welcome_bubble.h"
#include "ui/views/bubble/bubble_dialog_delegate.h"
#include "ui/views/controls/button/image_button.h"

namespace ash {

PaletteWelcomeBubbleTestAPI::PaletteWelcomeBubbleTestAPI(
    PaletteWelcomeBubble* palette_welcome_bubble)
    : palette_welcome_bubble_(palette_welcome_bubble) {}

views::ImageButton* PaletteWelcomeBubbleTestAPI::GetBubbleCloseButton() {
  return palette_welcome_bubble_->bubble_close_button_;
}

gfx::Rect PaletteWelcomeBubbleTestAPI::GetBubbleBounds() {
  if (palette_welcome_bubble_->bubble_view_)
    return palette_welcome_bubble_->bubble_view_->GetBoundsInScreen();

  return gfx::Rect();
}

}  // namespace ash
