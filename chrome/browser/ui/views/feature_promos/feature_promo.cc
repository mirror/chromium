// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/feature_promos/feature_promo.h"

#include "base/memory/ptr_util.h"
#include "base/strings/string_number_conversions.h"
#include "chrome/grit/generated_resources.h"
#include "components/variations/variations_associated_data.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/text_utils.h"
#include "ui/views/controls/label.h"
#include "ui/views/event_monitor.h"
#include "ui/views/layout/box_layout.h"
#include "ui/views/layout/fill_layout.h"

// static
FeaturePromoBubbleView* FeaturePromoBubbleView::Create(
    views::View* anchor_view,
    const gfx::Rect& anchor_rect) {
  return new FeaturePromoBubbleView(anchor_view, anchor_rect,
                                    views::BubbleBorder::NONE);
}

base::TimeDelta FeaturePromoBubbleView::DelayDefault() {
  return base::TimeDelta::FromSeconds(5);
}

base::TimeDelta FeaturePromoBubbleView::DelayShort() {
  return base::TimeDelta::FromSecondsD(2.5);
}

FeaturePromoBubbleView::FeaturePromoBubbleView(views::View* anchor_view,
                                               const gfx::Rect& anchor_rect,
                                               views::BubbleBorder::Arrow arrow)
    : BubbleDialogDelegateView(anchor_view, arrow) {
  set_arrow(arrow);

  if (!anchor_view)
    SetAnchorRect(anchor_rect);

  auto box_layout = base::MakeUnique<views::BoxLayout>(
      views::BoxLayout::kVertical, gfx::Insets(), 0);
  box_layout->set_main_axis_alignment(
      views::BoxLayout::MAIN_AXIS_ALIGNMENT_CENTER);
  box_layout->set_cross_axis_alignment(
      views::BoxLayout::CROSS_AXIS_ALIGNMENT_CENTER);
  SetLayoutManager(box_layout.release());

  AddChildView(
      new views::Label(l10n_util::GetStringUTF16(GetStringSpecifier())));

  views::Widget* widget = views::BubbleDialogDelegateView::CreateBubble(this);
  UseCompactMargins();
  widget->Show();
  StartAutoCloseTimer(DelayDefault());
}

FeaturePromoBubbleView::~FeaturePromoBubbleView() = default;

int FeaturePromoBubbleView::GetDialogButtons() const {
  return ui::DIALOG_BUTTON_NONE;
}

bool FeaturePromoBubbleView::OnMousePressed(const ui::MouseEvent& event) {
  CloseBubble();
  return true;
}

void FeaturePromoBubbleView::OnMouseEntered(const ui::MouseEvent& event) {
  timer_.Stop();
}

void FeaturePromoBubbleView::OnMouseExited(const ui::MouseEvent& event) {
  StartAutoCloseTimer(DelayShort());
}

void FeaturePromoBubbleView::CloseBubble() {
  GetWidget()->Close();
}

int FeaturePromoBubbleView::GetStringSpecifier() {
  static constexpr int kTextIds[] = {IDS_NEWTAB_PROMO_0, IDS_NEWTAB_PROMO_1,
                                     IDS_NEWTAB_PROMO_2};
  const std::string& str = variations::GetVariationParamValue(
      "NewTabInProductHelp", "x_promo_string");
  size_t text_specifier;
  if (!base::StringToSizeT(str, &text_specifier) ||
      text_specifier >= arraysize(kTextIds)) {
    text_specifier = 0;
  }

  return kTextIds[text_specifier];
}

void FeaturePromoBubbleView::StartAutoCloseTimer(
    base::TimeDelta auto_close_duration) {
  timer_.Start(FROM_HERE, auto_close_duration, this,
               &FeaturePromoBubbleView::CloseBubble);
}
