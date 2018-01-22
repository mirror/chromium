// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/profiles/dice_accounts_menu.h"

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

constexpr int kFixedMenuWidth = 240;
constexpr int kAvatarIconSize = 16;
constexpr int kVerticalSpace = 8;

// Sets the close_on_deactivate boolean of the given |bubble| to false and
// restores it's initial value once the class object goes out of scope.
class ScopedCloseBubbleOnDeactivate {
 public:
  ScopedCloseBubbleOnDeactivate(views::BubbleDialogDelegateView* bubble)
      : bubble_(bubble),
        initial_close_on_deactivate_(bubble->close_on_deactivate()) {
    bubble_->set_close_on_deactivate(false);
  }
  ~ScopedCloseBubbleOnDeactivate() {
    bubble_->set_close_on_deactivate(initial_close_on_deactivate_);
  }

 private:
  views::BubbleDialogDelegateView* bubble_;
  const bool initial_close_on_deactivate_;
};

}  // namespace

// static
DiceAccountsMenu* DiceAccountsMenu::menu_bubble_ = nullptr;

views::Widget* DiceAccountsMenu::ShowBubble(
    views::BubbleDialogDelegateView* parent_bubble,
    const std::vector<AccountInfo>& accounts,
    views::View* anchor_view) {
  if (menu_bubble_)
    return menu_bubble_->GetWidget();
  ScopedCloseBubbleOnDeactivate parent_bubble_saver(parent_bubble);
  menu_bubble_ = new DiceAccountsMenu(parent_bubble, accounts, anchor_view);
  views::Widget* widget =
      views::BubbleDialogDelegateView::CreateBubble(menu_bubble_);
  menu_bubble_->SetAlignment(views::BubbleBorder::ALIGN_ARROW_TO_MID_ANCHOR);
  menu_bubble_->SetArrowPaintType(views::BubbleBorder::PAINT_NONE);
  widget->Show();
  return widget;
}

DiceAccountsMenu::DiceAccountsMenu(
    views::BubbleDialogDelegateView* parent_bubble,
    const std::vector<AccountInfo>& accounts,
    views::View* anchor_view)
    : BubbleDialogDelegateView(anchor_view, views::BubbleBorder::NONE),
      parent_bubble_(parent_bubble),
      accounts_(accounts) {}

DiceAccountsMenu::~DiceAccountsMenu() = default;

void DiceAccountsMenu::Init() {
  // Create a hover button for each account in |accounts_| and add a
  // button "Use another account" at the end of the list.
  views::BoxLayout* layout =
      this->SetLayoutManager(std::make_unique<views::BoxLayout>(
          views::BoxLayout::kVertical, gfx::Insets(kVerticalSpace, 0), 0));
  layout->set_minimum_cross_axis_size(kFixedMenuWidth);

  // TODO(http://crbug.com/794522): Use the account pictures instead of the
  // default avatar.
  gfx::Image default_icon =
      ui::ResourceBundle::GetSharedInstance().GetImageNamed(
          profiles::GetPlaceholderAvatarIconResourceID());
  gfx::Image default_icon_circle =
      profiles::GetSizedAvatarIcon(default_icon, true, kAvatarIconSize,
                                   kAvatarIconSize, profiles::SHAPE_CIRCLE);
  for (const AccountInfo& account : accounts_) {
    views::View* account_button =
        new HoverButton(nullptr, *default_icon_circle.ToImageSkia(),
                        base::UTF8ToUTF16(account.email));
    this->AddChildView(account_button);
  }
  views::View* use_another_account = new HoverButton(
      nullptr, *default_icon_circle.ToImageSkia(),
      l10n_util::GetStringUTF16(IDS_PROFILES_DICE_USE_ANOTHER_ACCOUNT_BUTTON));
  this->AddChildView(use_another_account);
}

int DiceAccountsMenu::GetDialogButtons() const {
  return ui::DIALOG_BUTTON_NONE;
}

void DiceAccountsMenu::WindowClosing() {
  DCHECK_EQ(this, menu_bubble_);
  menu_bubble_ = nullptr;
}

void DiceAccountsMenu::OnWidgetDestroyed(views::Widget* widget) {
  // Make sure that the parent window closes when it is not active anymore, e.g.
  // when the user clicks outside the accounts menu.
  views::Widget* parent_widget = parent_bubble_->GetWidget();
  gfx::NativeWindow window =
      platform_util::GetTopLevel(parent_widget->GetNativeView());
  if (!platform_util::IsWindowActive(window))
    parent_widget->Close();
}
