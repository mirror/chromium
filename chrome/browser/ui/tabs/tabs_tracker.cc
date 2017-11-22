// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/tabs/tabs_tracker.h"

#include "chrome/browser/browser_process.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/browser_tab_strip_tracker.h"
#include "chrome/browser/ui/tabs/tab_logger.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/browser/web_contents_user_data.h"
#include "services/metrics/public/cpp/ukm_source_id.h"

DEFINE_WEB_CONTENTS_USER_DATA_KEY(tabs::TabsTracker::WebContentsData);

namespace tabs {

class TabsTracker::WebContentsData
    : public content::WebContentsObserver,
      public content::WebContentsUserData<WebContentsData> {
 public:
  ~WebContentsData() override = default;

  ukm::SourceId ukm_source_id() const { return ukm_source_id_; }

 private:
  friend class content::WebContentsUserData<WebContentsData>;

  explicit WebContentsData(content::WebContents* web_contents)
      : WebContentsObserver(web_contents) {}

  // content::WebContentsObserver:
  void DidFinishNavigation(
      content::NavigationHandle* navigation_handle) override {
    if (!navigation_handle->IsInMainFrame() ||
        navigation_handle->IsSameDocument()) {
      return;
    }

    // Use the same SourceId that SourceUrlRecorderWebContentsObserver populates
    // and updates.
    ukm_source_id_ = ukm::ConvertToSourceId(
        navigation_handle->GetNavigationId(), ukm::SourceIdType::NAVIGATION_ID);
  }
  void WasHidden() override {
    TabsTracker::GetInstance()->OnWasHidden(web_contents());
  }

  // Updated when a navigation is finished.
  ukm::SourceId ukm_source_id_ = 0;

  DISALLOW_COPY_AND_ASSIGN(WebContentsData);
};

TabsTracker::TabsTracker()
    : tab_logger_(std::make_unique<tabs::TabLogger>()),
      browser_tab_strip_tracker_(
          std::make_unique<BrowserTabStripTracker>(this, nullptr, nullptr)) {
  browser_tab_strip_tracker_->Init();
}

TabsTracker::~TabsTracker() = default;

void TabsTracker::OnWasHidden(content::WebContents* web_contents) {
  DCHECK(web_contents);

  // Log the state of the hidden tab.
  Browser* browser = chrome::FindBrowserWithWebContents(web_contents);
  if (!browser)
    return;

  TabStripModel* model = browser->tab_strip_model();
  int old_index = model->GetIndexOfWebContents(web_contents);
  DCHECK_NE(old_index, TabStripModel::kNoTab);

  TabsTracker::WebContentsData* web_contents_data =
      TabsTracker::WebContentsData::FromWebContents(web_contents);
  DCHECK(web_contents_data);
  tab_logger_->LogBackgroundTab(web_contents_data->ukm_source_id(),
                                web_contents, model, old_index);
}

void TabsTracker::TabPinnedStateChanged(TabStripModel* tab_strip_model,
                                        content::WebContents* contents,
                                        int index) {
  // We only log background tabs.
  if (tab_strip_model->active_index() == index)
    return;

  TabsTracker::WebContentsData* web_contents_data =
      TabsTracker::WebContentsData::FromWebContents(contents);
  DCHECK(web_contents_data);
  ukm::SourceId ukm_source_id = web_contents_data->ukm_source_id();
  if (!ukm_source_id)
    return;
  tab_logger_->LogBackgroundTab(ukm_source_id, contents, tab_strip_model,
                                index);
}

void TabsTracker::SetTabLoggerForTest(
    std::unique_ptr<tabs::TabLogger> tab_logger) {
  tab_logger_ = std::move(tab_logger);
}

// static
TabsTracker* TabsTracker::GetInstance() {
  CR_DEFINE_STATIC_LOCAL(TabsTracker, instance, ());
  return &instance;
}

// static
void TabsTracker::InitializeWebContentsDataForWebContents(
    content::WebContents* web_contents) {
  TabsTracker::WebContentsData::CreateForWebContents(web_contents);
}

}  // namespace tabs
