// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_BLOCKED_CONTENT_POPUP_OPENER_TRACKER_H_
#define CHROME_BROWSER_UI_BLOCKED_CONTENT_POPUP_OPENER_TRACKER_H_

#include <vector>

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/browser/web_contents_user_data.h"

namespace content {
class NavigationHandle;
class WebContents;
}  // namespace content

class PopupTracker;

// This class tracks all WebContents that open popups. It has an observer
// interface, and observers can respond to navigations with an indication that
// this tab should be closed.
class PopupOpenerTracker
    : public content::WebContentsObserver,
      public content::WebContentsUserData<PopupOpenerTracker> {
 public:
  ~PopupOpenerTracker() override;

  void CloseAsync();

  void AddPopup(PopupTracker* tracker) { last_popup_ = tracker; }
  void RemovePopup(PopupTracker* tracker);

 private:
  friend class content::WebContentsUserData<PopupOpenerTracker>;

  // content::WebContentsObserver:
  void DidFinishNavigation(content::NavigationHandle* handle) override;

  void CloseContents();

  explicit PopupOpenerTracker(content::WebContents* web_contents);

  PopupTracker* last_popup_ = nullptr;

  base::WeakPtrFactory<PopupOpenerTracker> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(PopupOpenerTracker);
};

#endif  // CHROME_BROWSER_UI_BLOCKED_CONTENT_POPUP_OPENER_TRACKER_H_
