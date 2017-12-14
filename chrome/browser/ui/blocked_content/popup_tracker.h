// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_BLOCKED_CONTENT_POPUP_TRACKER_H_
#define CHROME_BROWSER_UI_BLOCKED_CONTENT_POPUP_TRACKER_H_

#include <memory>

#include "base/macros.h"
#include "base/optional.h"
#include "base/time/time.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/browser/web_contents_user_data.h"
#include "third_party/WebKit/public/web/WebTriggeringEventInfo.h"

namespace base {
class DefaultTickClock;
}

namespace content {
class WebContents;
}

class ScopedVisibilityTracker;

// This class tracks new popups, and is used to log metrics on the visibility
// time of the first document in the popup.
// TODO(csharrison): Consider adding more metrics like total visibility for the
// lifetime of the WebContents.
class PopupTracker : public content::WebContentsObserver,
                     public content::WebContentsUserData<PopupTracker> {
 public:
  // |untrusted| represented whether or not the popup was opened from an
  // untrusted event source in the renderer. This should correspond to the
  // blocking behavior in the safe_browsing_triggered_popup_blocker.
  // Note that there are a few unknowns such that classifying !untrusted as
  // trusted isn't quite right.
  static void CreateForWebContents(content::WebContents* contents,
                                   content::WebContents* opener,
                                   bool untrusted);
  ~PopupTracker() override;

 private:
  friend class content::WebContentsUserData<PopupTracker>;

  PopupTracker(content::WebContents* contents,
               content::WebContents* opener,
               bool untrusted);

  // content::WebContentsObserver:
  void DidFinishNavigation(
      content::NavigationHandle* navigation_handle) override;
  void WasShown() override;
  void WasHidden() override;

  base::Optional<base::TimeDelta> GetVisibleTimeOnFirstDocument() const;

  // Visible time in this WebContents before the first document commits.
  base::Optional<base::TimeDelta> visible_time_before_first_document_;

  // The time this WebContents was visible on its first document load. Will be
  // not set until the first navigation commits, when it will be set to 0.
  base::Optional<base::TimeDelta> visible_time_on_first_document_;

  // The clock which is used by the visibility tracker.
  std::unique_ptr<base::DefaultTickClock> tick_clock_;

  // The |visibility_tracker_| tracks the time this WebContents is in the
  // foreground for its entire lifetime.
  std::unique_ptr<ScopedVisibilityTracker> visibility_tracker_;

  bool is_untrusted_;

  DISALLOW_COPY_AND_ASSIGN(PopupTracker);
};

#endif  // CHROME_BROWSER_UI_BLOCKED_CONTENT_POPUP_TRACKER_H_
