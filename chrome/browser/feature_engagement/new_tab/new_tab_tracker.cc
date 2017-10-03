// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/feature_engagement/new_tab/new_tab_tracker.h"

#include "base/time/time.h"
#include "chrome/browser/metrics/desktop_session_duration/desktop_session_duration_tracker.h"
#include "chrome/browser/ui/browser_list.h"
#include "chrome/browser/ui/views/feature_promos/new_tab_promo_bubble_view.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/browser/ui/views/tabs/new_tab_button.h"
#include "chrome/browser/ui/views/tabs/tab_strip.h"
#include "components/feature_engagement/public/event_constants.h"
#include "components/feature_engagement/public/feature_constants.h"
#include "components/feature_engagement/public/tracker.h"

namespace {

const int kDefaultPromoShowTimeInHours = 2;

}  // namespace

namespace feature_engagement {

NewTabTracker::NewTabTracker(Profile* profile,
                             SessionDurationUpdater* session_duration_updater)
    : FeatureTracker(profile,
                     session_duration_updater,
                     &kIPHNewTabFeature,
                     base::TimeDelta::FromHours(kDefaultPromoShowTimeInHours)),
      new_tab_promo_observer_(this) {}

NewTabTracker::NewTabTracker(SessionDurationUpdater* session_duration_updater)
    : NewTabTracker(nullptr, session_duration_updater) {}

NewTabTracker::~NewTabTracker() = default;

void NewTabTracker::OnNewTabOpened() {
  GetTracker()->NotifyEvent(events::kNewTabOpened);
}

void NewTabTracker::OnOmniboxNavigation() {
  GetTracker()->NotifyEvent(events::kOmniboxInteraction);
}

void NewTabTracker::OnOmniboxFocused() {
  if (ShouldShowPromo())
    ShowPromo();
}

void NewTabTracker::OnPromoClosed() {
  GetTracker()->Dismissed(kIPHNewTabFeature);
}

void NewTabTracker::OnSessionTimeMet() {
  GetTracker()->NotifyEvent(events::kNewTabSessionTimeMet);
}

void NewTabTracker::CloseBubble() {
  if (new_tab_promo_)
    new_tab_promo_->CloseBubble();
}

void NewTabTracker::ShowPromo() {
  DCHECK(!new_tab_promo_);
  auto* browser = BrowserView::GetBrowserViewForBrowser(
      BrowserList::GetInstance()->GetLastActive());
  DCHECK(browser);
  DCHECK(browser->IsActive());
  DCHECK(browser->tabstrip());
  DCHECK(browser->tabstrip()->new_tab_button());
  auto* new_tab_button = browser->tabstrip()->new_tab_button();

  // Owned by its native widget. Will be destroyed when its widget is destroyed.
  new_tab_promo_ = NewTabPromoBubbleView::CreateOwned(new_tab_button);
  views::Widget* widget = new_tab_promo_->GetWidget();
  new_tab_promo_observer_.Add(widget);
  new_tab_button->set_prominent_color_needed(true);
  new_tab_button->SchedulePaint();
}

void NewTabTracker::OnWidgetDestroying(views::Widget* widget) {
  OnPromoClosed();

  auto* browser = BrowserView::GetBrowserViewForBrowser(
      BrowserList::GetInstance()->GetLastActive());
  auto* new_tab_button = browser->tabstrip()->new_tab_button();

  if (new_tab_promo_observer_.IsObserving(widget)) {
    new_tab_promo_observer_.Remove(widget);
    new_tab_promo_ = nullptr;
    new_tab_button->set_prominent_color_needed(false);
    new_tab_button->SchedulePaint();
  }
}
}  // namespace feature_engagement
