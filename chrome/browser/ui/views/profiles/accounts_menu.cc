// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/profiles/accounts_menu.h"

#include <vector>

#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/platform_util.h"
#include "chrome/browser/ui/views/hover_button.h"
#include "ui/views/controls/button/button.h"
#include "ui/views/layout/box_layout.h"
#include "ui/views/view.h"
#include "ui/views/widget/widget.h"

namespace {

// Helpers --------------------------------------------------------------------

constexpr int kFixedMenuWidth = 240;

}  // namespace

// static
views::Widget* menu_widget = nullptr;

void AccountsMenu::ShowBubble(const std::vector<AccountInfo>& accounts,
                              views::ButtonListener* listener,
                              views::View* anchor_view) {
  if (menu_widget)
    return;

  AccountsMenu* bubble = new AccountsMenu(accounts, listener, anchor_view);
  menu_widget = views::BubbleDialogDelegateView::CreateBubble(bubble);
  bubble->SetAlignment(views::BubbleBorder::ALIGN_ARROW_TO_MID_ANCHOR);
  bubble->SetArrowPaintType(views::BubbleBorder::PAINT_NONE);
  menu_widget->Show();
}

AccountsMenu::AccountsMenu(const std::vector<AccountInfo>& accounts,
                           views::ButtonListener* listener,
                           views::View* anchor_view)
    : BubbleDialogDelegateView(anchor_view, views::BubbleBorder::TOP_RIGHT),
      listener_(listener),
      accounts_(accounts) {}

void AccountsMenu::Init() {
  std::unique_ptr<views::BoxLayout> layout = std::make_unique<views::BoxLayout>(
      views::BoxLayout::kVertical, gfx::Insets(), 0);
  layout->set_minimum_cross_axis_size(kFixedMenuWidth);
  this->SetLayoutManager(std::move(layout));

  views::View* b =
      new HoverButton(listener_, base::ASCIIToUTF16("Test button"));
  this->AddChildView(b);
}

int AccountsMenu::GetDialogButtons() const {
  return ui::DIALOG_BUTTON_NONE;
}

void AccountsMenu::WindowClosing() {
  menu_widget = nullptr;
}

void AccountsMenu::OnWidgetDestroyed(views::Widget* widget) {
  views::Widget* parent_widget = GetAnchorView()->GetWidget();
  gfx::NativeWindow window =
      platform_util::GetTopLevel(parent_widget->GetNativeView());
  if (!platform_util::IsWindowActive(window))
    parent_widget->Close();
}

AccountsMenu::~AccountsMenu() {}
