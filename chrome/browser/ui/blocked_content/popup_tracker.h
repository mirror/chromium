// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_BLOCKED_CONTENT_POPUP_TRACKER_H_
#define CHROME_BROWSER_UI_BLOCKED_CONTENT_POPUP_TRACKER_H_

#include <memory>

#include "base/macros.h"
#include "base/time/time.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/browser/web_contents_user_data.h"
#include "third_party/WebKit/public/web/WebTriggeringEventInfo.h"

namespace content {
class NavigationHandle;
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
  // This enum backs a histogram. Make sure all new additions are inserted at
  // the end before |kLast|, and enums.xml is properly updated.
  enum class Type : int {
    // The user explicitly clicked on this popup from the native popup blocking
    // UI.
    kClickedThrough,

    // The popup was generated via the OpenURL code path (e.g. shift click,
    // control click, context menu navigations).
    //
    // The JS triggering event for this navigation has isTrusted = false.
    kOpenUrlUntrustedEvent,

    // The JS triggering event for this navigation has isTrusted = true.
    kOpenUrlTrustedEvent,

    // There is no JS triggering event for this navigation.
    kOpenUrlNotFromEvent,

    // The isTrusted bit is unknown for this navigation.
    kOpenUrlTrustedEventUnknown,

    // The popup was generated via the window.open path, which also includes
    // navigations to blank targets.
    // TODO(csharrison): Add additional new window cases when that information
    // is plumbed from the render process.
    kNewWindowTrustedEventUnknown,

    // Put new entries before this one.
    kLast
  };

  static void CreateForWebContents(content::WebContents* web_contents,
                                   Type type);

  // The OpenURL code path has extra information about the event that triggered
  // the navigation. This method vends the proper type for those navigations.
  // TODO(csharrison): Thread the triggering event through the window.open path
  // as well.
  static Type PopupTrackerTypeForTriggeringEvent(
      blink::WebTriggeringEventInfo triggering_event_info);

  ~PopupTracker() override;

 private:
  friend class content::WebContentsUserData<PopupTracker>;

  explicit PopupTracker(content::WebContents* web_contents, Type type);

  // content::WebContentsObserver:
  void DidFinishNavigation(
      content::NavigationHandle* navigation_handle) override;
  void WasShown() override;
  void WasHidden() override;

  std::unique_ptr<ScopedVisibilityTracker> visibility_tracker_;

  Type type_;

  DISALLOW_COPY_AND_ASSIGN(PopupTracker);
};

#endif  // CHROME_BROWSER_UI_BLOCKED_CONTENT_POPUP_TRACKER_H_
