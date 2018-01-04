// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_SYSTEM_POWER_OFF_MENU_ITEM_VIEW_H_
#define ASH_SYSTEM_POWER_OFF_MENU_ITEM_VIEW_H_

#include "ash/ash_export.h"
#include "ash/resources/vector_icons/vector_icons.h"
#include "ui/views/controls/button/image_button.h"

namespace views {
class Label;
class ImageView;
}  // namespace views

namespace ash {

class ASH_EXPORT PowerOffMenuItemView : public views::ImageButton {
 public:
  PowerOffMenuItemView(views::ButtonListener* listener,
                       const gfx::VectorIcon& icon,
                       base::string16 title_text);
  ~PowerOffMenuItemView() override;

 private:
  // views::View:
  void Layout() override;

  // views::ImageButton:
  void PaintButtonContents(gfx::Canvas* canvas) override;

  views::ImageView* icon_view_;
  views::Label* title_;

  DISALLOW_COPY_AND_ASSIGN(PowerOffMenuItemView);
};

}  // namespace ash

#endif  // ASH_SYSTEM_POWER_OFF_MENU_ITEM_VIEW_H_
