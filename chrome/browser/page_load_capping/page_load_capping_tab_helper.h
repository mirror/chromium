// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_PAGE_LOAD_CAPPING_PAGE_LOAD_CAPPING_INFOBAR_TAB_HELPER_H_
#define CHROME_BROWSER_PAGE_LOAD_CAPPING_PAGE_LOAD_CAPPING_INFOBAR_TAB_HELPER_H_

#include <memory>

#include "base/macros.h"
#include "content/public/browser/global_request_id.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/browser/web_contents_user_data.h"

// Tracks whether a page_load_capping infobar has been shown for a page. Handles
// showing the infobar when the main frame response indicates a Lite Page.
class PageLoadCappingTabHelper
    : public content::WebContentsObserver,
      public content::WebContentsUserData<PageLoadCappingTabHelper> {
 public:
  ~PageLoadCappingTabHelper() override;

  content::GlobalRequestID navigation_id() { return navigation_id_; }

  void ReadyToCommitNavigation(
      content::NavigationHandle* navigation_handle) override;

  void DidFinishNavigation(
      content::NavigationHandle* navigation_handle) override;

  void UpdateInfobar(int64_t kilo_bytes, bool is_video);

  void Pause();
  void UnPause(bool create_new);
  void StopVideo();

 private:
  friend class content::WebContentsUserData<PageLoadCappingTabHelper>;

  explicit PageLoadCappingTabHelper(content::WebContents* web_contents);

  content::GlobalRequestID navigation_id_;

  class PageLoadCappingInfoBarObserver;

  std::unique_ptr<PageLoadCappingInfoBarObserver> infobar_observer_;

  bool info_bar_created_;
  bool committed_;
  bool paused_;
  bool is_video_;
  int64_t last_data_update_;

  DISALLOW_COPY_AND_ASSIGN(PageLoadCappingTabHelper);
};

#endif  // CHROME_BROWSER_PAGE_LOAD_CAPPING_PAGE_LOAD_CAPPING_INFOBAR_TAB_HELPER_H_
