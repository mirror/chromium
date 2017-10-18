// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/resource_coordinator/tab_discardable_unit_source.h"

#include "base/containers/flat_set.h"
#include "base/stl_util.h"
#include "chrome/browser/resource_coordinator/tab_discardable_unit.h"
#include "chrome/browser/resource_coordinator/tab_manager.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"

namespace resource_coordinator {

TabDiscardableUnitSource::TabDiscardableUnitSource(TabManager* tab_manager)
    : browser_tab_strip_tracker_(this, nullptr, this),
      tab_manager_(tab_manager) {
  DCHECK(tab_manager_);
  browser_tab_strip_tracker_.Init();
}

TabDiscardableUnitSource::~TabDiscardableUnitSource() = default;

void TabDiscardableUnitSource::NotifyFocusedTab() {
  // TODO: add active_tab_strip_model_for_testing_

  Browser* const active_browser = chrome::FindBrowserWithActiveWindow();
  if (!active_browser)
    return;
  content::WebContents* const active_contents =
      active_browser->tab_strip_model()->GetActiveWebContents();
  auto it = tabs_.find(active_contents);
  DCHECK(it != tabs_.end());
  tab_manager_->OnDiscardableUnitFocused(it->second.get());
}

void TabDiscardableUnitSource::TabInsertedAt(TabStripModel* tab_strip_model,
                                             content::WebContents* contents,
                                             int index,
                                             bool foreground) {
  auto it = tabs_.find(contents);
  if (it == tabs_.end()) {
    auto discardable_unit = std::make_unique<TabDiscardableUnit>(
        tab_manager_, contents, tab_strip_model);
    TabDiscardableUnit* discardable_unit_raw = discardable_unit.get();
    tabs_[contents] = std::move(discardable_unit);
    tab_manager_->OnDiscardableUnitCreated(discardable_unit_raw);
  } else {
    it->second->SetTabStripModel(tab_strip_model);
  }
}

void TabDiscardableUnitSource::TabClosingAt(TabStripModel* tab_strip_model,
                                            content::WebContents* contents,
                                            int index) {
  auto it = tabs_.find(contents);
  DCHECK(it != tabs_.end());
  tab_manager_->OnDiscardableUnitDestroyed(it->second.get());
  tabs_.erase(it);
}

void TabDiscardableUnitSource::ActiveTabChanged(
    content::WebContents* old_contents,
    content::WebContents* new_contents,
    int index,
    int reason) {
  NotifyFocusedTab();
}

void TabDiscardableUnitSource::TabReplacedAt(TabStripModel* tab_strip_model,
                                             content::WebContents* old_contents,
                                             content::WebContents* new_contents,
                                             int index) {
  DCHECK(!base::ContainsKey(tabs_, new_contents));
  auto it = tabs_.find(old_contents);
  DCHECK(it != tabs_.end());
  it->second->SetWebContents(new_contents);
}

void TabDiscardableUnitSource::OnBrowserSetLastActive(Browser* browser) {
  NotifyFocusedTab();
}

}  // namespace resource_coordinator
