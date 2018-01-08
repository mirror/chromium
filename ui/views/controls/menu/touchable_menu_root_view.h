// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_VIEWS_CONTROLS_MENU_TOUCHABLE_MENU_ROOT_VIEW_H_
#define UI_VIEWS_CONTROLS_MENU_TOUCHABLE_MENU_ROOT_VIEW_H_

#include <map>

#include "base/macros.h"
#include "ui/views/bubble/bubble_dialog_delegate.h"
#include "ui/views/controls/button/button.h"
#include "ui/views/views_export.h"

namespace ui {
class MenuModel;
}

namespace views {

class TouchableMenuItemView;

// TouchableMenuRootView -----------------------------------------------------

// TouchableMenuRootView represents the host view of multiple touchable menu
// items. Unlike MenuItemView, this view does not support nested menus and it is
// only meant for single level menus.
class VIEWS_EXPORT TouchableMenuRootView : public BubbleDialogDelegateView {
  // The host view for the touchable context menu. Displays all elements in
  // MenuModel.
 public:
  TouchableMenuRootView(gfx::NativeWindow parent,
                        ui::MenuModel* menu_model,
                        views::View* anchor_view);

  void Show();

  void Init() override;

  void ActivateAt(int index);

  // Overridden from BubbleDialogDelegateView:
  int GetDialogButtons() const override;
  bool ShouldShowCloseButton() const override;

  TouchableMenuItemView* get_touchable_menu_item_view_for_test(int index);

 private:
  TouchableMenuItemView* CreateButton(int i);
  int GetIndexForButton(Button* button);

  gfx::NativeWindow parent_window_;
  // List of context menu options.
  ui::MenuModel* menu_model_;    // Not owned.
  views::View* container_view_;  // Owned by this view, contains all items in
                                 // |menu_model_| as TouchableMenuItemView.
  DISALLOW_COPY_AND_ASSIGN(TouchableMenuRootView);
};

}  // namespace views

#endif  // UI_VIEWS_CONTROLS_MENU_TOUCHABLE_MENU_ROOT_VIEW_H_
