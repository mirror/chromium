// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_BLOCKED_CONTENT_POPUP_TRACKER_H_
#define CHROME_BROWSER_UI_BLOCKED_CONTENT_POPUP_TRACKER_H_

#include "base/macros.h"
#include "base/supports_user_data.h"
#include "content/public/browser/web_contents_observer.h"

namespace content {
class WebContents;
}  // namespace content

// This class tracks new popups, and is used to log metrics. Is isn't a
// WebContentsUserData to allow the class to remove itself when it is done
// logging metrics.
// TODO(csharrison): This class does nothing at the moment. Add some metrics
// logging.
class PopupTracker : public content::WebContentsObserver,
                     public base::SupportsUserData::Data {
 public:
  static const void* const kUserDataKey;
  // Creates and attaches a PopupTracker to the |web_contents|.
  static void CreateForWebContents(content::WebContents* web_contents);
  ~PopupTracker() override;

 private:
  explicit PopupTracker(content::WebContents* web_contents);
  DISALLOW_COPY_AND_ASSIGN(PopupTracker);
};

#endif  // CHROME_BROWSER_UI_BLOCKED_CONTENT_POPUP_TRACKER_H_
