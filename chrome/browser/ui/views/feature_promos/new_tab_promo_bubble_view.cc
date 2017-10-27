// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/feature_promos/new_tab_promo_bubble_view.h"

#include "base/logging.h"
#include "base/strings/string_number_conversions.h"
#include "chrome/browser/feature_engagement/new_tab/new_tab_tracker.h"
#include "chrome/browser/feature_engagement/new_tab/new_tab_tracker_factory.h"
#include "chrome/browser/ui/views/feature_promos/feature_promo_bubble_view.h"
#include "chrome/browser/ui/views/tabs/new_tab_button.h"
#include "chrome/grit/generated_resources.h"
#include "components/variations/variations_associated_data.h"

// static
NewTabPromoBubbleView* NewTabPromoBubbleView::CreateOwned(
    views::View* anchor_view) {
  return new NewTabPromoBubbleView(anchor_view);
}

NewTabPromoBubbleView::NewTabPromoBubbleView(views::View* anchor_view)
    : FeaturePromoBubbleView(anchor_view,
                             views::BubbleBorder::LEFT_CENTER,
                             GetStringSpecifier(),
                             ActivationAction::DO_NOT_ACTIVATE),
      new_tab_promo_observer_(this) {}

NewTabPromoBubbleView::~NewTabPromoBubbleView() = default;

// static
int NewTabPromoBubbleView::GetStringSpecifier() {
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

void NewTabPromoBubbleView::ShowPromoBubble() {
  DCHECK(!new_tab_promo_);
  new_tab_promo_ = this;

  views::Widget* widget = new_tab_promo_->GetWidget();
  new_tab_promo_observer_.Add(widget);

  new_tab_button()->set_prominent_color_needed(true);
  new_tab_button()->SchedulePaint();
}

void NewTabPromoBubbleView::ClosePromoBubble() {
  DCHECK(new_tab_promo_);
  new_tab_promo_->CloseWidget();
}

void NewTabPromoBubbleView::OnWidgetDestroying(views::Widget* widget) {
  feature_engagement::NewTabTrackerFactory::GetInstance()
      ->GetForProfile(browser()->profile())
      ->OnPromoClosed();

  if (new_tab_promo_observer_.IsObserving(widget)) {
    new_tab_promo_observer_.Remove(widget);
    new_tab_promo_ = nullptr;
    new_tab_button()->set_prominent_color_needed(false);
    new_tab_button()->SchedulePaint();
  }
}
