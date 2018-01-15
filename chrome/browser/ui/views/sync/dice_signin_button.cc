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
#include "ui/views/controls/button/label_button_label.h"

namespace {

// Drop down arrow sizes.
constexpr int kDropDownArrowIconSize = 15;
constexpr int kDropDownArrowButtonWidth = 60;

// Divider sizes
constexpr int kDividerLeftPadding = 4;
constexpr int kDividerWidth = 1;
constexpr int kDividerVerticalPadding = 10;

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
  subtitle_ = new views::LabelButtonLabel(base::ASCIIToUTF16(account_->email),
                                          views::style::CONTEXT_BUTTON);
  subtitle_->SetAutoColorReadabilityEnabled(false);
  subtitle_->SetEnabledColor(SK_ColorWHITE);
  subtitle_->SetDisabledColor(SK_ColorWHITE);
  AddChildView(subtitle_);

  divider_ = new views::View();
  divider_->SetBackground(views::CreateSolidBackground(SK_ColorWHITE));
  AddChildViewAt(divider_, 0);

  arrow_ = views::MdTextButton::CreateSecondaryUiBlueButton(button_listener,
                                                            base::string16());
  arrow_->SetImage(
      views::Button::STATE_NORMAL,
      gfx::CreateVectorIcon(kSigninButtonDropDownArrowIcon,
                            kDropDownArrowIconSize, SK_ColorWHITE));
  AddChildViewAt(arrow_, 1);

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
  if (!account_)
    return MdTextButton::GetChildAreaBounds();

  gfx::Size s = size();
  s.set_width(s.width() - kDividerLeftPadding - kDividerWidth -
              kDropDownArrowButtonWidth);
  return gfx::Rect(s);
}

void DiceSigninButton::Layout() {
  views::MdTextButton::Layout();

  if (!account_)
    return;

  DCHECK(arrow_);

  // Move the title label up to make space for the subtitle.
  gfx::Size label_size = label()->size();
  label()->SetSize(gfx::Size(label_size.width(), label_size.height() / 2));

  // Lay out the subtitle right below the title label.
  gfx::Rect label_bounds = label()->bounds();
  subtitle_->SetBoundsRect(
      gfx::Rect(label_bounds.bottom_left(), subtitle_->GetPreferredSize()));
  // subtitle_->SetEnabledColor(label()->enabled_color());

  // Lay the divider and the arrow on the right.
  gfx::Size s = size();
  divider_->SetBounds(s.width() - kDividerLeftPadding - kDividerWidth,
                      kDividerVerticalPadding, kDividerWidth,
                      s.height() - 2 * kDividerVerticalPadding);
  arrow_->SetBounds(s.width() - kDropDownArrowButtonWidth, 0,
                    kDropDownArrowButtonWidth, s.height());
}
