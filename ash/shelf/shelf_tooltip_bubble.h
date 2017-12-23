// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_SHELF_SHELF_TOOLTIP_BUBBLE_H_
#define ASH_SHELF_SHELF_TOOLTIP_BUBBLE_H_

#include "ash/ash_export.h"
#include "ui/views/bubble/bubble_dialog_delegate.h"

namespace ash {

// The implementation of tooltip of the launcher.
class ASH_EXPORT ShelfTooltipBubble : public views::BubbleDialogDelegateView {
 public:
  explicit ShelfTooltipBubble(views::View* anchor,
                              views::BubbleBorder::Arrow arrow,
                              const base::string16& text,
                              ui::NativeTheme* theme,
                              bool asymmetrical_border);
  ~ShelfTooltipBubble() override;

 private:
  // views::BubbleDialogDelegateView:
  gfx::Size CalculatePreferredSize() const override;
  int GetDialogButtons() const override;

  DISALLOW_COPY_AND_ASSIGN(ShelfTooltipBubble);
};

}  // namespace ash

#endif  // ASH_SHELF_SHELF_TOOLTIP_BUBBLE_H_
