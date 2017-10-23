// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_TAB_UI_HELPER_H_
#define CHROME_BROWSER_UI_TAB_UI_HELPER_H_

#include "base/macros.h"
#include "chrome/browser/sessions/session_restore_observer.h"
#include "content/public/browser/web_contents_user_data.h"

class TabUIHelper : public SessionRestoreObserver,
                    public content::WebContentsObserver,
                    public content::WebContentsUserData<TabUIHelper> {
 public:
  ~TabUIHelper() override;

  // SessionRestoreObserver implementation
  void OnWillRestoreTab(content::WebContents* contents) override;

  // content::WebContentsObserver implementation
  void DidStopLoading() override;

  void SetWasActive(bool was_active) { was_active_ = was_active; }
  void SetIsNavigationDelayed(bool is_navigation_delayed) {
    is_navigation_delayed_ = is_navigation_delayed;
  }

  bool was_active() const { return was_active_; }
  bool is_navigation_delayed() const { return is_navigation_delayed_; }
  bool created_by_session_restore() const {
    return created_by_session_restore_;
  }

 private:
  explicit TabUIHelper(content::WebContents* contents);
  friend class content::WebContentsUserData<TabUIHelper>;

  bool was_active_ = false;
  bool is_navigation_delayed_ = false;
  bool created_by_session_restore_ = false;

  DISALLOW_COPY_AND_ASSIGN(TabUIHelper);
};

#endif  // CHROME_BROWSER_UI_TAB_UI_HELPER_H_
