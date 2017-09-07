// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_UKM_CONTENT_SOURCE_URL_RECORDER_WEB_CONTENTS_OBSERVER_H_
#define COMPONENTS_UKM_CONTENT_SOURCE_URL_RECORDER_WEB_CONTENTS_OBSERVER_H_

#include "base/macros.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/browser/web_contents_user_data.h"

namespace content {
class NavigationHandle;
}  // namespace content

namespace ukm {

// SourceUrlRecorderWebContentsObserver is responsible for recording UKM source
// URLs, for all main frame navigations in a given WebContents.
// SourceUrlRecorderWebContentsObserver records both the final URL for a
// navigation, and, if the navigation was redirected, the initial URL as well.
class SourceUrlRecorderWebContentsObserver
    : public content::WebContentsObserver,
      public content::WebContentsUserData<
          SourceUrlRecorderWebContentsObserver> {
 public:
  // Creates a SourceUrlRecorderWebContentsObserver for the given
  // WebContents. If a SourceUrlRecorderWebContentsObserver is already
  // associated with the WebContents, this method is a no-op.
  static void CreateForWebContents(content::WebContents* web_contents);

  void DidStartNavigation(
      content::NavigationHandle* navigation_handle) override;
  void DidFinishNavigation(
      content::NavigationHandle* navigation_handle) override;

 private:
  explicit SourceUrlRecorderWebContentsObserver(
      content::WebContents* web_contents);
  friend class content::WebContentsUserData<
      SourceUrlRecorderWebContentsObserver>;

  void MaybeRecordUrl(content::NavigationHandle* navigation_handle);

  DISALLOW_COPY_AND_ASSIGN(SourceUrlRecorderWebContentsObserver);
};

}  // namespace ukm

#endif  // COMPONENTS_UKM_CONTENT_SOURCE_URL_RECORDER_WEB_CONTENTS_OBSERVER_H_
