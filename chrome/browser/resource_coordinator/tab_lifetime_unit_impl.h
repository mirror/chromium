// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_RESOURCE_COORDINATOR_TAB_LIFETIME_UNIT_IMPL_H_
#define CHROME_BROWSER_RESOURCE_COORDINATOR_TAB_LIFETIME_UNIT_IMPL_H_

#include "base/macros.h"
#include "base/observer_list.h"
#include "base/time/time.h"
#include "chrome/browser/resource_coordinator/tab_lifetime_unit.h"
#include "content/public/browser/web_contents_observer.h"

class TabStripModel;

namespace resource_coordinator {

class TabLifetimeObserver;

class TabLifetimeUnitImpl : public TabLifetimeUnit,
                            public content::WebContentsObserver {
 public:
  // |observers| is a list of observers that are notified when the discarded
  // state or the auto-discardable state of this tab changes. |web_contents| is
  // the WebContents associated with this tab. |tab_strip_model| is the
  // TabStripModel to which this tab belongs.
  TabLifetimeUnitImpl(base::ObserverList<TabLifetimeObserver>* observers,
                      content::WebContents* web_contents,
                      TabStripModel* tab_strip_model);
  ~TabLifetimeUnitImpl() override;

  // Sets the TabStripModel associated with this tab. The TabStripModel
  // associated with a tab changes when it is dragged between browser windows.
  void set_tab_strip_model(TabStripModel* tab_strip_model) {
    DCHECK(tab_strip_model_);
    tab_strip_model_ = tab_strip_model;
  }

  // Sets the WebContents associated with this tab. The WebContents associated
  // with a tab changes when the tab is discarded and when prerendered or
  // distilled content is displayed.
  void SetWebContents(content::WebContents* web_contents);

  // Invoked when the tab gains or loses focus.
  void SetFocused(bool focused);

  // Invoked when the "recently audible" bit of the tab changes (this is the bit
  // that determines whether a speaker icon is displayed next to the tab in the
  // tab strip).
  void SetRecentlyAudible(bool recently_audible);

  // TabLifetimeUnit:
  content::WebContents* GetWebContents() const override;
  content::RenderProcessHost* GetRenderProcessHost() const override;
  bool IsMediaTab() const override;
  bool IsAutoDiscardable() const override;
  void SetAutoDiscardable(bool auto_discardable) override;

  // LifetimeUnit:
  TabLifetimeUnit* AsTabLifetimeUnit() override;
  base::string16 GetTitle() const override;
  base::ProcessHandle GetProcessHandle() const override;
  SortKey GetSortKey() const override;
  State GetState() const override;
  int GetEstimatedMemoryFreedOnDiscardKB() const override;
  bool CanDiscard(DiscardCondition system_condition) const override;
  bool Discard(DiscardCondition system_condition) override;

#if !defined(OS_CHROMEOS)
  // Time duing which a tab cannot be discarded after having been active.
  static constexpr base::TimeDelta kRecentUsageProtectionTime =
      base::TimeDelta::FromMinutes(10);
#endif

 private:
  // Invoked when the state goes from DISCARDED to non-DISCARDED and vice-versa.
  void OnDiscardedStateChange();

  // content::WebContentsObserver:
  void DidStartLoading() override;

  base::ObserverList<TabLifetimeObserver>* observers_;
  TabStripModel* tab_strip_model_;
  State state_ = State::ALIVE;
  base::TimeTicks last_focused_time_;
  int discard_count_ = 0;
  bool auto_discardable_ = true;
  bool recently_audible_ = false;
  base::TimeTicks recently_audible_change_time_;

  DISALLOW_COPY_AND_ASSIGN(TabLifetimeUnitImpl);
};

}  // namespace resource_coordinator

#endif  // CHROME_BROWSER_RESOURCE_COORDINATOR_LIFETIME_UNIT_IMPL_H_
