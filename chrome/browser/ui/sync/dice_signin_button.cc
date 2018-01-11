// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/sync/dice_signin_button.h"

#include "chrome/app/vector_icons/vector_icons.h"
#include "chrome/grit/generated_resources.h"
#include "ui/base/l10n/l10n_util.h"

DiceSigninButton::DiceSigninButton(
    const AccountInfo& account_info,
    views::ButtonListener* signin_button_listener,
    views::ButtonListener* drop_down_arrow_listener) {
  views::MdTextButton(signin_button_listener, views::style::CONTEXT_BUTTON) {
    DCHECK(!account_info.email.empty());

    // First create the child views.
    subtitle_ = new views::LabelButtonLabel(
        base::ASCIIToUTF16(account_info.email), views::style::CONTEXT_BUTTON);
    subtitle_->SetAutoColorReadabilityEnabled(false);
    subtitle_->SetEnabledColor(SK_ColorWHITE);
    subtitle_->SetDisabledColor(SK_ColorWHITE);
    AddChildView(subtitle_);

    divider_ = new views::View();
    divider_->SetBackground(views::CreateSolidBackground(SK_ColorWHITE));
    AddChildViewAt(divider_, 0);

    arrow_ = views::MdTextButton::CreateSecondaryUiBlueButton(
        drop_down_arrow_listener, base::string16());
    arrow_->SetImage(views::Button::STATE_NORMAL,
                     gfx::CreateVectorIcon(kSigninButtonDropDownArrowIcon, 15,
                                           SK_ColorWHITE));
    AddChildViewAt(arrow_, 0);

    // Set the title text for main Sign-in button.
    base::string16 button_title =
        account.full_name.empty()
            ? l10n_util::GetStringUTF16(
                  IDS_PROFILES_DICE_SIGNIN_FIRST_ACCOUNT_BUTTON_NO_NAME)
            : l10n_util::GetStringFUTF16(
                  IDS_PROFILES_DICE_SIGNIN_FIRST_ACCOUNT_BUTTON,
                  base::UTF8ToUTF16(account.full_name));

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
    SetImage(views::Button::STATE_NORMAL,
             *profile_photo_circular.ToImageSkia());
  }

  DiceSigninButton ~DiceSigninButton() override = default;

  gfx::Rect DiceSigninButton::GetChildAreaBounds() override {
    gfx::Size s = size();
    s.set_width(s.width() - 65);

    return gfx::Rect(s);
  }

  void DiceSigninButton::Layout() override {
    DCHECK(arrow_);
    views::MdTextButton::Layout();

    gfx::Size label_size = label()->size();
    label()->SetSize(gfx::Size(label_size.width(), label_size.height() / 2));

    gfx::Rect label_bounds = label()->bounds();
    subtitle_->SetBoundsRect(
        gfx::Rect(label_bounds.bottom_left(), subtitle_->GetPreferredSize()));

    subtitle_->SetEnabledColor(label()->enabled_color());

    gfx::Size s = size();
    divider_->SetBounds(s.width() - 60 - 4, 10, 1, s.height() - 2 * 10);
    arrow_->SetBounds(s.width() - 60, 0, 60, s.height());
  }
