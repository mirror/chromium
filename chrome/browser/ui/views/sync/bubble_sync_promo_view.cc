// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/sync/bubble_sync_promo_view.h"

#include <stddef.h>

#include "base/strings/string16.h"
#include "chrome/browser/ui/sync/bubble_sync_promo_delegate.h"
#include "chrome/browser/ui/views/harmony/chrome_typography.h"
#include "third_party/skia/include/core/SkColor.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/gfx/font.h"
#include "ui/views/controls/styled_label.h"
#include "ui/views/layout/fill_layout.h"

#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/ui/views/harmony/chrome_layout_provider.h"
#include "ui/views/controls/button/md_text_button.h"
#include "ui/views/layout/box_layout.h"

#include "chrome/browser/profiles/profile_avatar_icon_util.h"
#include "chrome/browser/ui/views/hover_button.h"
#include "chrome/browser/ui/views/profiles/badged_profile_photo.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/views/border.h"
#include "ui/views/controls/button/label_button_label.h"

std::unique_ptr<BadgedProfilePhoto> GetAccountIcon() {
  gfx::Image account_icon =
      ui::ResourceBundle::GetSharedInstance().GetImageNamed(
          profiles::GetPlaceholderAvatarIconResourceID());
  return std::make_unique<BadgedProfilePhoto>(
      BadgedProfilePhoto::BADGE_TYPE_NONE, account_icon);
}

BubbleSyncPromoView::BubbleSyncPromoView(BubbleSyncPromoDelegate* delegate,
                                         int link_text_resource_id,
                                         int message_text_resource_id)
    : StyledLabel(base::string16(), this), delegate_(delegate) {
  size_t offset = 0;
  base::string16 link_text = l10n_util::GetStringUTF16(link_text_resource_id);
  base::string16 promo_text =
      l10n_util::GetStringFUTF16(message_text_resource_id, link_text, &offset);
  SetText(promo_text);

  AddStyleRange(gfx::Range(offset, offset + link_text.length()),
                views::StyledLabel::RangeStyleInfo::CreateForLink());

  views::StyledLabel::RangeStyleInfo promo_style;
  promo_style.text_style = STYLE_SECONDARY;
  gfx::Range before_link_range(0, offset);
  if (!before_link_range.is_empty())
    AddStyleRange(before_link_range, promo_style);
  gfx::Range after_link_range(offset + link_text.length(), promo_text.length());
  if (!after_link_range.is_empty())
    AddStyleRange(after_link_range, promo_style);
}

BubbleSyncPromoView::~BubbleSyncPromoView() {}

const char* BubbleSyncPromoView::GetClassName() const {
  return "BubbleSyncPromoView";
}

void BubbleSyncPromoView::StyledLabelLinkClicked(views::StyledLabel* label,
                                                 const gfx::Range& range,
                                                 int event_flags) {
  delegate_->OnSignInLinkClicked();
}

class DiceSigninButton : public views::MdTextButton {
 public:
  DiceSigninButton(views::ButtonListener* listener)
      :  // views::MdTextButton(listener, views::style::CONTEXT_BUTTON_MD) {
        views::MdTextButton(listener, views::style::CONTEXT_BUTTON) {
    // First create the child views.

    subtitle_ = new views::LabelButtonLabel(
        base::ASCIIToUTF16("msarda@google.com"), views::style::CONTEXT_BUTTON);
    subtitle_->SetAutoColorReadabilityEnabled(false);
    subtitle_->SetEnabledColor(SK_ColorWHITE);
    subtitle_->SetDisabledColor(SK_ColorWHITE);
    AddChildView(subtitle_);

    arrow_ = views::MdTextButton::CreateSecondaryUiBlueButton(
        nullptr, base::ASCIIToUTF16(">"));
    AddChildView(arrow_);

    SetBorder(views::CreateEmptyBorder(50, 0, 50, 0));

    // Set the text.
    SetText(base::ASCIIToUTF16("Sync to Mihai Sardarescu"));
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
  ~DiceSigninButton() override = default;

  gfx::Rect GetChildAreaBounds() override {
    gfx::Size s = size();
    s.set_width(s.width() - 60);

    return gfx::Rect(s);
  }

  void Layout() override {
    DCHECK(arrow_);
    views::MdTextButton::Layout();

    gfx::Size label_size = label()->size();
    label()->SetSize(gfx::Size(label_size.width(), label_size.height() / 2));

    gfx::Rect label_bounds = label()->bounds();
    subtitle_->SetBoundsRect(
        gfx::Rect(label_bounds.bottom_left(), subtitle_->GetPreferredSize()));

    subtitle_->SetEnabledColor(label()->enabled_color());

    gfx::Size s = size();
    arrow_->SetBounds(s.width() - 60, 0, 60, s.height());
  }

 private:
  views::LabelButtonLabel* subtitle_;
  views::LabelButton* arrow_;
  DISALLOW_COPY_AND_ASSIGN(DiceSigninButton);
};

DiceBubbleSyncPromoView::DiceBubbleSyncPromoView(
    BubbleSyncPromoDelegate* delegate,
    int link_text_resource_id,
    int message_text_resource_id)
    : View() {
  views::BoxLayout* layout =
      new views::BoxLayout(views::BoxLayout::kVertical, gfx::Insets(),
                           ChromeLayoutProvider::Get()->GetDistanceMetric(
                               views::DISTANCE_RELATED_CONTROL_VERTICAL));
  SetLayoutManager(layout);

  BubbleSyncPromoView* promo_text = new BubbleSyncPromoView(
      delegate, link_text_resource_id, message_text_resource_id);
  AddChildView(promo_text);

  //  auto* signin_button = views::MdTextButton::CreateSecondaryUiBlueButton(
  //      nullptr, base::ASCIIToUTF16("Sign in"));
  //  auto* title = views::MdTextButton::CreateSecondaryUiBlueButton(
  //      nullptr, base::ASCIIToUTF16(">>>"));
  //  signin_button->AddChildView(title);
  AddChildView(new DiceSigninButton(nullptr));
}

DiceBubbleSyncPromoView::~DiceBubbleSyncPromoView() {}

void DiceBubbleSyncPromoView::Layout() {
  views::View::Layout();
  LOG(ERROR) << PrintViewGraph(true);
}
