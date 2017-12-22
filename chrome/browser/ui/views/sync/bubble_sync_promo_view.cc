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
    arrow_ = views::MdTextButton::CreateSecondaryUiBlueButton(
        nullptr, base::ASCIIToUTF16(">"));
    AddChildView(arrow_);

    // Do the layout
    SetText(base::ASCIIToUTF16("Sign in to Chrome"));
    SetFocusForPlatform();
    SetProminent(true);
  }
  ~DiceSigninButton() override = default;

  gfx::Rect GetChildAreaBounds() override {
    gfx::Size s = size();
    s.set_width(s.width() - 30);
    return gfx::Rect(s);
  }

  void Layout() override {
    DCHECK(arrow_);
    views::MdTextButton::Layout();
    gfx::Size s = size();
    arrow_->SetBounds(s.width() - 60, 0, 60, s.height());
  }

 private:
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
