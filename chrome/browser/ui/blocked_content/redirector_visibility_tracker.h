// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_BLOCKED_CONTENT_REDIRECTOR_VISIBILITY_TRACKER_H_
#define CHROME_BROWSER_UI_BLOCKED_CONTENT_REDIRECTOR_VISIBILITY_TRACKER_H_

#include <memory>

#include "base/macros.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/browser/web_contents_user_data.h"

namespace content {
class WebContents;
}

class ScopedVisibilityTracker;

// This class tracks the visible lifetime of a tab after it successfully
// navigates cross-origin while not visible.
class RedirectorVisibilityTracker
    : public content::WebContentsObserver,
      public content::WebContentsUserData<RedirectorVisibilityTracker> {
 public:
  ~RedirectorVisibilityTracker() override;

 private:
  friend class content::WebContentsUserData<RedirectorVisibilityTracker>;

  explicit RedirectorVisibilityTracker(content::WebContents* web_contents);

  // content::WebContentsObserver:
  void DidFinishNavigation(
      content::NavigationHandle* navigation_handle) override;
  void WasShown() override;
  void WasHidden() override;

  // The |visibility_tracker| tracks the time this WebContents is in the
  // foreground.
  std::unique_ptr<ScopedVisibilityTracker> visibility_tracker_;

  DISALLOW_COPY_AND_ASSIGN(RedirectorVisibilityTracker);
};

#endif  // CHROME_BROWSER_UI_BLOCKED_CONTENT_REDIRECTOR_VISIBILITY_TRACKER_H_
