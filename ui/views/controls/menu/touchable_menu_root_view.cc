// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#include "ui/views/controls/menu/touchable_menu_root_view.h"

#include <iostream>

#include "ui/aura/window.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/models/menu_model.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/strings/grit/ui_strings.h"
#include "ui/views/bubble/bubble_dialog_delegate.h"
#include "ui/views/controls/button/button.h"
#include "ui/views/controls/label.h"
#include "ui/views/controls/menu/touchable_menu_item_view.h"
#include "ui/views/layout/fill_layout.h"
#include "ui/views/layout/grid_layout.h"
#include "ui/views/widget/widget.h"

namespace {

// Equivalent to ash::ShellWindowId::kShellWindowId_MenuContainer
constexpr int kShellWindowId_MenuContainer = 20;

}  // namespace

namespace views {

/*
 * TODO: Write tests to pin down what's been done.
 *       Clean up implementation with unique_ptr...
 *       Add images, make them clickable.
 *       Add hover state animations to buttons(might just work with images).
 *       Handle the app icon drag case for shelf and applist.
 *       Then re-start the design doc.
 *       Split up into X cls.
 */
TouchableMenuRootView::TouchableMenuRootView(ui::MenuModel* menu_model,
                                             views::View* anchor_view)
    : BubbleDialogDelegateView(anchor_view, views::BubbleBorder::BOTTOM_RIGHT), menu_model_(menu_model) {
  SetAnchorView(anchor_view);
}

void TouchableMenuRootView::Init() {
  SetFocusBehavior(FocusBehavior::ALWAYS);
  //set_arrow(views::BubbleBorder::BOTTOM_RIGHT);
  set_shadow(views::BubbleBorder::NO_SHADOW);
  SetLayoutManager(std::make_unique<views::FillLayout>());
  set_color(SK_ColorLTGRAY);

  // doesn't test well.
  // Place the bubble in the Menu's root window.

  // Align the arrow to the middle of the app icon.
  int vertical_inset = GetAnchorView()->bounds().height() / 2;
  set_anchor_view_insets(gfx::Insets(vertical_inset, 0));

  // Set up the child views.
  container_view_ = new views::View();
  views::GridLayout* layout = container_view_->SetLayoutManager(
      std::make_unique<views::GridLayout>(container_view_));
  views::ColumnSet* columns = layout->AddColumnSet(0);
  layout->StartRow(0, 0);

  for (int i = 0; i < menu_model_->GetItemCount(); i++) {
    TouchableMenuItemView* view = CreateButton(i);
    columns->AddColumn(views::GridLayout::FILL, views::GridLayout::FILL, 1,
                       views::GridLayout::USE_PREF, 0, 0);
    layout->AddView(view);
  }

  AddChildView(container_view_);
}

void TouchableMenuRootView::SetParentWindow() {
   set_parent_window(GetAnchorView()
                         ->GetWidget()
                         ->GetNativeWindow()
                         ->GetRootWindow()
                         ->GetChildById(kShellWindowId_MenuContainer));
}

void TouchableMenuRootView::ActivateAt(int index) {
  // test me...
  menu_model_->ActivatedAt(index, 0);
  GetWidget()->CloseNow();
}

TouchableMenuItemView* TouchableMenuRootView::CreateButton(int i) {
  base::string16 test = menu_model_->GetLabelAt(i);
  TouchableMenuItemView* item_view = new TouchableMenuItemView(this, test, i);
  return item_view;
}

int TouchableMenuRootView::GetDialogButtons() const {
  return ui::DIALOG_BUTTON_NONE;
}
bool TouchableMenuRootView::ShouldShowCloseButton() const {
  return false;
}

TouchableMenuItemView*
TouchableMenuRootView::get_touchable_menu_item_view_for_test(int index) {
  return static_cast<TouchableMenuItemView*>(container_view_->child_at(index));
}

}  // namespace views
