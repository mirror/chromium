// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/views/controls/menu/touchable_menu_item_view.h"

#include "ui/views/controls/label.h"
#include "ui/views/controls/menu/touchable_menu_root_view.h"
#include "ui/views/layout/fill_layout.h"

#include <iostream>

namespace {
// Stolen from shelf_constants in ash...
// Ink drop color for shelf items.
constexpr SkColor kShelfInkDropBaseColor = SK_ColorWHITE;
// Opacity of the ink drop ripple for shelf items when the ripple is visible.
constexpr float kShelfInkDropVisibleOpacity = 0.2f;
}  // namespace

namespace views {

TouchableMenuItemView::TouchableMenuItemView(TouchableMenuRootView* root_view,
                                             base::string16 label_text,
                                             int index)
    : Button(this),
      root_view_(root_view),
      label_(new views::Label(label_text)),
      index_(index) {
  SetFocusBehavior(FocusBehavior::ALWAYS);
  DCHECK(root_view_);
  SetInkDropMode(InkDropMode::ON);
  set_ink_drop_base_color(kShelfInkDropBaseColor);
  set_ink_drop_visible_opacity(kShelfInkDropVisibleOpacity);
  set_animate_on_state_change(true);
  SetPreferredSize(gfx::Size(75, 10));
  SetLayoutManager(std::make_unique<views::FillLayout>());
  label_->SetEnabledColor(SK_ColorBLACK);
  label_->SetFocusBehavior(FocusBehavior::NEVER);

  AddChildView(label_);
}

void TouchableMenuItemView::ButtonPressed(Button* sender,
                                          const ui::Event& event) {
  root_view_->ActivateAt(index_);
}

}  // namespace views.
