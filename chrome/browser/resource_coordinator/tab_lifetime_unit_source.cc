// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/resource_coordinator/tab_lifetime_unit_source.h"

#include "base/stl_util.h"
#include "chrome/browser/resource_coordinator/lifetime_unit_sink.h"
#include "chrome/browser/resource_coordinator/tab_lifetime_unit_impl.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/browser_list.h"
#include "chrome/browser/ui/browser_window.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"

namespace resource_coordinator {

TabLifetimeUnitSource::TabLifetimeUnitSource(LifetimeUnitSink* sink)
    : browser_tab_strip_tracker_(this, nullptr, this), sink_(sink) {
  DCHECK(sink_);
  DCHECK(BrowserList::GetInstance()->empty());
  browser_tab_strip_tracker_.Init();
}

TabLifetimeUnitSource::~TabLifetimeUnitSource() = default;

void TabLifetimeUnitSource::AddObserver(TabLifetimeObserver* observer) {
  observers_.AddObserver(observer);
}

void TabLifetimeUnitSource::RemoveObserver(TabLifetimeObserver* observer) {
  observers_.RemoveObserver(observer);
}

void TabLifetimeUnitSource::SetFocusedTabStripModelForTesting(
    TabStripModel* tab_strip) {
  focused_tab_strip_model_for_testing_ = tab_strip;
  UpdateFocusedTab();
}

void TabLifetimeUnitSource::UpdateFocusedTab() {
  Browser* focused_browser = chrome::FindBrowserWithActiveWindow();
  TabStripModel* focused_tab_strip_model =
      focused_tab_strip_model_for_testing_
          ? focused_tab_strip_model_for_testing_
          : (focused_browser ? focused_browser->tab_strip_model() : nullptr);
  content::WebContents* focused_web_contents =
      focused_tab_strip_model ? focused_tab_strip_model->GetActiveWebContents()
                              : nullptr;
  DCHECK(!focused_web_contents ||
         base::ContainsKey(tabs_, focused_web_contents));
  TabLifetimeUnitImpl* new_focused_tab_lifetime_unit =
      focused_web_contents ? tabs_[focused_web_contents].get() : nullptr;
  if (new_focused_tab_lifetime_unit != focused_tab_lifetime_unit_) {
    if (focused_tab_lifetime_unit_)
      focused_tab_lifetime_unit_->SetFocused(false);
    if (new_focused_tab_lifetime_unit)
      new_focused_tab_lifetime_unit->SetFocused(true);
    focused_tab_lifetime_unit_ = new_focused_tab_lifetime_unit;
  }
}

void TabLifetimeUnitSource::TabInsertedAt(TabStripModel* tab_strip_model,
                                          content::WebContents* contents,
                                          int index,
                                          bool foreground) {
  auto it = tabs_.find(contents);
  if (it == tabs_.end()) {
    // A tab was created.
    auto res = tabs_.insert(
        std::make_pair(contents, std::make_unique<TabLifetimeUnitImpl>(
                                     &observers_, contents, tab_strip_model)));
    sink_->OnLifetimeUnitCreated(res.first->second.get());
  } else {
    // A tab was moved to another window.
    it->second->set_tab_strip_model(tab_strip_model);
  }
  if (foreground)
    UpdateFocusedTab();
}

void TabLifetimeUnitSource::TabClosingAt(TabStripModel* tab_strip_model,
                                         content::WebContents* contents,
                                         int index) {
  auto it = tabs_.find(contents);
  DCHECK(it != tabs_.end());
  TabLifetimeUnitImpl* lifetime_unit = it->second.get();
  if (focused_tab_lifetime_unit_ == lifetime_unit)
    focused_tab_lifetime_unit_ = nullptr;
  sink_->OnLifetimeUnitDestroyed(lifetime_unit);
  tabs_.erase(contents);
}

void TabLifetimeUnitSource::ActiveTabChanged(content::WebContents* old_contents,
                                             content::WebContents* new_contents,
                                             int index,
                                             int reason) {
  UpdateFocusedTab();
}

void TabLifetimeUnitSource::TabReplacedAt(TabStripModel* tab_strip_model,
                                          content::WebContents* old_contents,
                                          content::WebContents* new_contents,
                                          int index) {
  DCHECK(!base::ContainsKey(tabs_, new_contents));
  auto it = tabs_.find(old_contents);
  DCHECK(it != tabs_.end());
  std::unique_ptr<TabLifetimeUnitImpl> lifetime_unit = std::move(it->second);
  lifetime_unit->SetWebContents(new_contents);
  tabs_.erase(it);
  tabs_[new_contents] = std::move(lifetime_unit);
}

void TabLifetimeUnitSource::TabChangedAt(content::WebContents* contents,
                                         int index,
                                         TabChangeType change_type) {
  if (change_type == TabChangeType::ALL) {
    DCHECK(base::ContainsKey(tabs_, contents));
    tabs_[contents]->SetRecentlyAudible(contents->WasRecentlyAudible());
  }
}

void TabLifetimeUnitSource::OnBrowserSetLastActive(Browser* browser) {
  UpdateFocusedTab();
}

void TabLifetimeUnitSource::OnBrowserNoLongerActive(Browser* browser) {
  UpdateFocusedTab();
}

}  // namespace resource_coordinator
