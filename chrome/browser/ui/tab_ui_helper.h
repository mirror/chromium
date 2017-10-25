// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_TAB_UI_HELPER_H_
#define CHROME_BROWSER_UI_TAB_UI_HELPER_H_

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/strings/string16.h"
#include "base/strings/utf_string_conversions.h"
#include "base/task/cancelable_task_tracker.h"
#include "components/favicon_base/favicon_types.h"
#include "content/public/browser/web_contents_user_data.h"
#include "ui/gfx/image/image.h"
#include "ui/gfx/image/image_skia.h"

// The helper class that stores the WebContents' information so that different
// tab UI (favicon/title) can be shown according to the information.
class TabUIHelper : public content::WebContentsObserver,
                    public content::WebContentsUserData<TabUIHelper> {
 public:
  ~TabUIHelper() override;

  // content::WebContentsObserver implementation
  void DidStopLoading() override;

  void NotifyNavigationDelayed(bool is_navigation_delayed);

  void set_was_active_at_least_once() { was_active_at_least_once_ = true; }
  void set_created_by_session_restore(bool created_by_session_restore) {
    created_by_session_restore_ = created_by_session_restore;
  }

  // Returns whether this tab has been active since it is created.
  bool was_active_at_least_once() const { return was_active_at_least_once_; }

  // Returns if this tab's navigation is delayed. Only the initial navigation of
  // the tab is considered.
  bool is_navigation_delayed() const { return is_navigation_delayed_; }

  // Returns if this tab is created by session restore. Only the initial
  // navigation of the tab is considered.
  bool created_by_session_restore() const {
    return created_by_session_restore_;
  }

  // Get the title of the tab. When the associated WebContents' title is empty,
  // a customized title is used.
  const base::string16& GetTitle() const;

  gfx::ImageSkia GetFavicon() const;

 private:
  friend class content::WebContentsUserData<TabUIHelper>;

  explicit TabUIHelper(content::WebContents* contents);
  void OnFaviconFetched(const favicon_base::FaviconImageResult& favicon);

  bool was_active_at_least_once_ = false;
  bool is_navigation_delayed_ = false;
  bool created_by_session_restore_ = false;
  base::string16 title_;
  gfx::Image favicon_;

  base::CancelableTaskTracker favicon_tracker_;
  base::WeakPtrFactory<TabUIHelper> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(TabUIHelper);
};

#endif  // CHROME_BROWSER_UI_TAB_UI_HELPER_H_
