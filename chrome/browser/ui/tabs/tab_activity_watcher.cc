// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/tabs/tab_activity_watcher.h"

#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/tabs/tab_snapshot_logger.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/browser/web_contents_user_data.h"
#include "services/metrics/public/cpp/ukm_source_id.h"

DEFINE_WEB_CONTENTS_USER_DATA_KEY(TabActivityWatcher::WebContentsData);

class TabActivityWatcher::WebContentsData
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
    if (!navigation_handle->HasCommitted() ||
        !navigation_handle->IsInMainFrame() ||
        navigation_handle->IsSameDocument()) {
      return;
    }

    // Use the same SourceId that SourceUrlRecorderWebContentsObserver populates
    // and updates.
    ukm_source_id_ = ukm::ConvertToSourceId(
        navigation_handle->GetNavigationId(), ukm::SourceIdType::NAVIGATION_ID);
  }
  void WasHidden() override {
    TabActivityWatcher::GetInstance()->OnWasHidden(web_contents());
  }

  // Updated when a navigation is finished.
  ukm::SourceId ukm_source_id_ = 0;

  DISALLOW_COPY_AND_ASSIGN(WebContentsData);
};

TabActivityWatcher::TabActivityWatcher()
    : tab_snapshot_logger_(std::make_unique<TabSnapshotLogger>()),
      browser_tab_strip_tracker_(this, nullptr, nullptr) {
  browser_tab_strip_tracker_.Init();
}

TabActivityWatcher::~TabActivityWatcher() = default;

void TabActivityWatcher::OnWasHidden(content::WebContents* web_contents) {
  DCHECK(web_contents);

  // Log the state of the hidden tab.
  TabActivityWatcher::WebContentsData* web_contents_data =
      TabActivityWatcher::WebContentsData::FromWebContents(web_contents);
  DCHECK(web_contents_data);
  tab_snapshot_logger_->LogBackgroundTab(web_contents_data->ukm_source_id(),
                                         web_contents);
}

void TabActivityWatcher::TabPinnedStateChanged(TabStripModel* tab_strip_model,
                                               content::WebContents* contents,
                                               int index) {
  // We only want UKMs for background tabs.
  if (tab_strip_model->active_index() == index)
    return;

  TabActivityWatcher::WebContentsData* web_contents_data =
      TabActivityWatcher::WebContentsData::FromWebContents(contents);
  DCHECK(web_contents_data);
  ukm::SourceId ukm_source_id = web_contents_data->ukm_source_id();
  if (!ukm_source_id)
    return;
  tab_snapshot_logger_->LogBackgroundTab(ukm_source_id, contents);
}

void TabActivityWatcher::SetTabSnapshotLoggerForTest(
    std::unique_ptr<TabSnapshotLogger> tab_snapshot_logger) {
  tab_snapshot_logger_ = std::move(tab_snapshot_logger);
}

// static
TabActivityWatcher* TabActivityWatcher::GetInstance() {
  CR_DEFINE_STATIC_LOCAL(TabActivityWatcher, instance, ());
  return &instance;
}

// static
void TabActivityWatcher::InitializeWebContentsDataForWebContents(
    content::WebContents* web_contents) {
  TabActivityWatcher::WebContentsData::CreateForWebContents(web_contents);
}
