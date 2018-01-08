// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/profiles/accounts_menu.h"

#include <vector>

#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/platform_util.h"
#include "chrome/browser/profiles/profile_avatar_icon_util.h"
#include "chrome/browser/ui/views/hover_button.h"
#include "chrome/grit/generated_resources.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/gfx/image/image.h"
#include "ui/views/controls/button/button.h"
#include "ui/views/layout/box_layout.h"
#include "ui/views/view.h"
#include "ui/views/widget/widget.h"

namespace {

// Helpers --------------------------------------------------------------------

constexpr int kFixedMenuWidth = 240;
constexpr int kIconSize = 16;
constexpr int kVerticalSpace = 8;

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
  // Create a list of hover buttons for each account in |accounts_| and add a
  // button to "Use another account" at the end of the list.
  std::unique_ptr<views::BoxLayout> layout = std::make_unique<views::BoxLayout>(
      views::BoxLayout::kVertical, gfx::Insets(kVerticalSpace, 0), 0);
  layout->set_minimum_cross_axis_size(kFixedMenuWidth);
  this->SetLayoutManager(std::move(layout));

  // TODO(http://crbug.com/794522): Use the account pictures instead of the
  // default avatar.
  gfx::Image default_icon =
      ui::ResourceBundle::GetSharedInstance().GetImageNamed(
          profiles::GetPlaceholderAvatarIconResourceID());
  gfx::Image default_icon_circle = profiles::GetSizedAvatarIcon(
      default_icon, true, kIconSize, kIconSize, profiles::SHAPE_CIRCLE);
  for (const AccountInfo& account : accounts_) {
    views::View* hover_button =
        new HoverButton(listener_, *default_icon_circle.ToImageSkia(),
                        base::UTF8ToUTF16(account.email));
    this->AddChildView(hover_button);
  }
  views::View* use_another_account = new HoverButton(
      listener_, *default_icon_circle.ToImageSkia(),
      l10n_util::GetStringUTF16(IDS_PROFILES_DICE_USE_ANOTHER_ACCOUNT_BUTTON));
  this->AddChildView(use_another_account);
}

int AccountsMenu::GetDialogButtons() const {
  return ui::DIALOG_BUTTON_NONE;
}

void AccountsMenu::WindowClosing() {
  menu_widget = nullptr;
}

void AccountsMenu::OnWidgetDestroyed(views::Widget* widget) {
  // Make sure that the parent window closes when it is not active anymore, e.g.
  // when the user clicks outside.
  views::Widget* parent_widget = GetAnchorView()->GetWidget();
  gfx::NativeWindow window =
      platform_util::GetTopLevel(parent_widget->GetNativeView());
  if (!platform_util::IsWindowActive(window))
    parent_widget->Close();
}

AccountsMenu::~AccountsMenu() {}
