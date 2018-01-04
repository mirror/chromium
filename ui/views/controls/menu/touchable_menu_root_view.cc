// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#include "ui/views/controls/menu/touchable_menu_root_view.h"

#include <iostream>

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

namespace views {

/*
 *
 *
 *
 * TODO:
 *
 *       Write tests!
 *       Add images, make them clickable.
 *       Then re-start the design doc.
 *
 *
 *
 */
TouchableMenuRootView::TouchableMenuRootView(gfx::NativeWindow parent,
                                             ui::MenuModel* menu_model,
                                             views::View* anchor_view)
    : parent_window_(parent),
      menu_model_(menu_model),
      widget_(new views::Widget) {
  // TODO(newcomer): do bubble left/right if on left/right side of shelf.
  // Support side shelf.
  set_arrow(views::BubbleBorder::BOTTOM_RIGHT);
  set_shadow(views::BubbleBorder::NO_SHADOW);
  SetLayoutManager(std::make_unique<views::FillLayout>());
  SetAnchorView(anchor_view);
  // Align the arrow to the middle of the app icon.
  int vertical_inset = anchor_view->bounds().height() / 2;
  set_anchor_view_insets(gfx::Insets(vertical_inset, 0));
  set_color(SK_ColorLTGRAY);

  // Set up the child views.
  container_view_ = new views::View();
  views::GridLayout* layout = container_view_->SetLayoutManager(
      std::make_unique<views::GridLayout>(container_view_));
  views::ColumnSet* columns = layout->AddColumnSet(0);
  layout->StartRow(0, 0);
  for (int i = 0; i < menu_model_->GetItemCount(); i++) {
    TouchableMenuItemView* view = CreateButton(i);
    columns->AddColumn(views::GridLayout::CENTER, views::GridLayout::CENTER, 1,
                       views::GridLayout::USE_PREF, 0, 100);
    layout->AddView(view);
  }

  AddChildView(container_view_);
}

TouchableMenuRootView::~TouchableMenuRootView() {}

void TouchableMenuRootView::Show() {
  views::Widget::InitParams params =
      GetDialogWidgetInitParams(this, nullptr, nullptr, gfx::Rect());
  params.parent = parent_window_;
  widget_->Init(params);
  widget_->AddObserver(this);
  widget_->Show();
}

void TouchableMenuRootView::ActivateAt(int index) {
  // test me...
  menu_model_->ActivatedAt(index, 0);
  widget_->Deactivate();
  // GetWidget()->Deactivate();
}

void TouchableMenuRootView::CloseMenu() {
  widget_->Deactivate();
  // GetWidget()->Deactivate();
}

TouchableMenuItemView* TouchableMenuRootView::CreateButton(int i) {
  base::string16 test = menu_model_->GetLabelAt(i);
  TouchableMenuItemView* item_view = new TouchableMenuItemView(this, test, i);
  // configure.
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
