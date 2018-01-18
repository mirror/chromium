// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/sync/dice_signin_button.h"

#include "base/strings/string16.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/app/vector_icons/vector_icons.h"
#include "chrome/browser/profiles/profile_avatar_icon_util.h"
#include "chrome/grit/generated_resources.h"
#include "components/vector_icons/vector_icons.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/gfx/paint_vector_icon.h"
#include "ui/views/background.h"
#include "ui/views/border.h"

namespace {

// Drop down arrow sizes.
constexpr int kDropDownArrowIconSize = 15;
constexpr int kDropDownArrowButtonWidth = 50;

// Divider sizes
constexpr int kDividerHorizontalPadding = 4;
constexpr int kDividerWidth = 1;
constexpr int kDividerVerticalPadding = 10;

int GetDividerAndArrowReservedWidth() {
  return 2 * kDividerHorizontalPadding + kDividerWidth +
         kDropDownArrowButtonWidth;
}

}  // namespace

DiceSigninButton::DiceSigninButton(views::ButtonListener* button_listener)
    : views::MdTextButton(button_listener, views::style::CONTEXT_BUTTON),
      account_(base::nullopt) {
  // Regular MD text button when there is not account.
  SetText(l10n_util::GetStringUTF16(IDS_PROFILES_DICE_SIGNIN_BUTTON));
  SetFocusForPlatform();
  SetProminent(true);
  SetHorizontalAlignment(gfx::ALIGN_CENTER);
}

DiceSigninButton::DiceSigninButton(const AccountInfo& account,
                                   views::ButtonListener* button_listener)
    : views::MdTextButton(button_listener, views::style::CONTEXT_BUTTON),
      account_(account) {
  // First create the child views.
  subtitle_ = new views::Label(base::ASCIIToUTF16(account_->email));
  subtitle_->SetAutoColorReadabilityEnabled(false);
  subtitle_->SetEnabledColor(SK_ColorWHITE);
  subtitle_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  AddChildView(subtitle_);

  divider_ = new views::View();
  divider_->SetBackground(views::CreateSolidBackground(SK_ColorWHITE));
  AddChildView(divider_);

  arrow_ = new views::ImageButton(button_listener);
  arrow_->SetImageAlignment(views::ImageButton::ALIGN_CENTER,
                            views::ImageButton ::ALIGN_MIDDLE);
  arrow_->SetInkDropMode(views::InkDropHostView::InkDropMode::ON);
  arrow_->set_has_ink_drop_action_on_click(true);
  arrow_->SetFocusForPlatform();
  arrow_->SetFocusPainter(nullptr);
  arrow_->SetImage(
      views::Button::STATE_NORMAL,
      gfx::CreateVectorIcon(kSigninButtonDropDownArrowIcon,
                            kDropDownArrowIconSize, SK_ColorWHITE));
  AddChildView(arrow_);

  // Set the title text for main Sign-in button.
  base::string16 button_title =
      account_->full_name.empty()
          ? l10n_util::GetStringUTF16(
                IDS_PROFILES_DICE_SIGNIN_FIRST_ACCOUNT_BUTTON_NO_NAME)
          : l10n_util::GetStringFUTF16(
                IDS_PROFILES_DICE_SIGNIN_FIRST_ACCOUNT_BUTTON,
                base::UTF8ToUTF16(account_->full_name));
  SetText(button_title);
  SetFocusForPlatform();
  SetProminent(true);
  SetHorizontalAlignment(gfx::ALIGN_LEFT);

  // Set the image
  gfx::Image account_icon =
      ui::ResourceBundle::GetSharedInstance().GetImageNamed(
          profiles::GetPlaceholderAvatarIconResourceID());
  gfx::Image profile_photo_circular = profiles::GetSizedAvatarIcon(
      account_icon, true, 40, 40, profiles::SHAPE_CIRCLE);
  SetImage(views::Button::STATE_NORMAL, *profile_photo_circular.ToImageSkia());
}

DiceSigninButton::~DiceSigninButton() = default;

gfx::Rect DiceSigninButton::GetChildAreaBounds() {
  gfx::Rect child_area = MdTextButton::GetChildAreaBounds();
  if (!account_)
    return child_area;

  // Make room on the right for the divider and the drop-down arrow.
  child_area.set_width(child_area.width() - GetDividerAndArrowReservedWidth());
  return child_area;
}

int DiceSigninButton::GetHeightForWidth(int width) const {
  if (!account_)
    return MdTextButton::GetHeightForWidth(width);

  int available_width = MdTextButton::GetHeightForWidth(width) -
                        GetDividerAndArrowReservedWidth();
  int height_without_subtitle =
      MdTextButton::GetHeightForWidth(available_width);
  int title_subtitle_height = label()->GetHeightForWidth(available_width) +
                              subtitle_->GetHeightForWidth(available_width) +
                              GetInsets().height();
  return std::max(height_without_subtitle, title_subtitle_height);
}

gfx::Size DiceSigninButton::CalculatePreferredSize() const {
  NOTREACHED() << "DiceSigninButton is only used in bubbles that places "
                  " buttons using |GetHeightForWidth|. This method is "
                  " intentionally not implemented to avoid confusion about "
                  " the way this button is being layed out.";
  return MdTextButton::CalculatePreferredSize();
}

void DiceSigninButton::Layout() {
  views::MdTextButton::Layout();
  if (!account_)
    return;

  gfx::Rect bounds = GetLocalBounds();

  // By default, |title| takes the entire height of the button. Shink |title|
  // to make space for |subtitle_|.
  views::Label* title = label();
  gfx::Size title_size = title->size();
  int min_title_height = title->GetHeightForWidth(title_size.width());
  int title_height = std::max(min_title_height, title_size.height() / 2);
  title->SetSize(gfx::Size(title_size.width(), title_height));

  // Lay out |subtitle_| right below |title|.
  gfx::Point title_bottom_left = title->bounds().bottom_left();
  int subtitle_width = bounds.width() - GetInsets().left() -
                       title_bottom_left.x() -
                       GetDividerAndArrowReservedWidth();
  int subtitle_height = subtitle_->GetHeightForWidth(subtitle_width);
  subtitle_->SetBounds(title_bottom_left.x(), title_bottom_left.y(),
                       subtitle_width, subtitle_height);

  // Lay the divider and the arrow on the right.
  int divider_x = bounds.width() - GetDividerAndArrowReservedWidth() +
                  kDividerHorizontalPadding;
  divider_->SetBounds(divider_x, kDividerVerticalPadding, kDividerWidth,
                      bounds.height() - 2 * kDividerVerticalPadding);
  arrow_->SetBounds(divider_x + kDividerWidth + kDividerHorizontalPadding, 0,
                    kDropDownArrowButtonWidth, bounds.height());
}
