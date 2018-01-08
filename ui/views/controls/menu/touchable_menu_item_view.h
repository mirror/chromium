// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_VIEWS_CONTROLS_MENU_TOUCHABLE_MENU_ITEM_VIEW_H
#define UI_VIEWS_CONTROLS_MENU_TOUCHABLE_MENU_ITEM_VIEW_H

#include "base/macros.h"
#include "ui/views/controls/button/button.h"
#include "ui/views/views_export.h"

namespace views {

class TouchableMenuRootView;
class Label;

class VIEWS_EXPORT TouchableMenuItemView : public Button,
                                           public ButtonListener {
 public:
  TouchableMenuItemView(TouchableMenuRootView* root_view,
                        base::string16 label_text,
                        int index);
  // Overridden from ButtonListener:
  void ButtonPressed(Button* sender, const ui::Event& event) override;

 private:
  TouchableMenuRootView* root_view_;
  views::Label* label_;
  int index_;  // The index of the menu command, used by MenuModel.
};

}  // namespace views

#endif  // UI_VIEWS_CONTROLS_MENU_TOUCHABLE_MENU_ITEM_VIEW_H
