// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_SYSTEM_PALETTE_PALETTE_WELCOME_BUBBLE_TEST_API_H_
#define ASH_SYSTEM_PALETTE_PALETTE_WELCOME_BUBBLE_TEST_API_H_

#include "base/macros.h"
#include "ui/gfx/geometry/rect.h"

namespace views {
class ImageButton;
}

namespace ash {
class ImageButton;
class PaletteWelcomeBubble;

// Use the api in this class to test PaletteWelcomeBubble.
class PaletteWelcomeBubbleTestAPI {
 public:
  explicit PaletteWelcomeBubbleTestAPI(
      PaletteWelcomeBubble* palette_welcome_bubble);
  ~PaletteWelcomeBubbleTestAPI() = default;

  views::ImageButton* GetBubbleCloseButton();

  gfx::Rect GetBubbleBounds();

 private:
  PaletteWelcomeBubble* palette_welcome_bubble_;

  DISALLOW_COPY_AND_ASSIGN(PaletteWelcomeBubbleTestAPI);
};

}  // namespace ash

#endif  // ASH_SYSTEM_PALETTE_PALETTE_WELCOME_BUBBLE_TEST_API_H_
