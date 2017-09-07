// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/ukm/content/source_url_recorder_web_contents_observer.h"

#include "base/macros.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/browser/web_contents_user_data.h"
#include "services/metrics/public/cpp/ukm_recorder.h"
#include "services/metrics/public/cpp/ukm_source_id.h"
#include "url/gurl.h"

namespace {

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

SourceUrlRecorderWebContentsObserver::SourceUrlRecorderWebContentsObserver(
    content::WebContents* web_contents)
    : content::WebContentsObserver(web_contents) {}

void SourceUrlRecorderWebContentsObserver::DidStartNavigation(
    content::NavigationHandle* navigation_handle) {
  // Record the initial URL for the navigation.
  MaybeRecordUrl(navigation_handle);
}

void SourceUrlRecorderWebContentsObserver::DidFinishNavigation(
    content::NavigationHandle* navigation_handle) {
  // Record the final URL for the navigation, after processing any redirects.
  MaybeRecordUrl(navigation_handle);
}

void SourceUrlRecorderWebContentsObserver::MaybeRecordUrl(
    content::NavigationHandle* navigation_handle) {
  // Please get approval from chrome-privacy-core@google.com before modifying
  // the UKM URL recording policy below.
  if (!ukm::UkmRecorder::Get() || !navigation_handle->IsInMainFrame() ||
      navigation_handle->IsSameDocument()) {
    return;
  }

  const GURL& url = navigation_handle->GetURL();
  if (!url.SchemeIsHTTPOrHTTPS() && !url.SchemeIs("chrome-extension"))
    return;

  ukm::SourceId source_id = ukm::ConvertToSourceId(
      navigation_handle->GetNavigationId(), ukm::SourceIdType::NAVIGATION_ID);
  ukm::UkmRecorder::Get()->UpdateSourceURL(source_id, url);
}

// static
void SourceUrlRecorderWebContentsObserver::CreateForWebContents(
    content::WebContents* web_contents) {
  if (!SourceUrlRecorderWebContentsObserver::FromWebContents(web_contents)) {
    web_contents->SetUserData(
        SourceUrlRecorderWebContentsObserver::UserDataKey(),
        base::WrapUnique(
            new SourceUrlRecorderWebContentsObserver(web_contents)));
  }
}

}  // namespace

DEFINE_WEB_CONTENTS_USER_DATA_KEY(SourceUrlRecorderWebContentsObserver);

namespace ukm {

void InitializeSourceUrlRecorderForWebContents(
    content::WebContents* web_contents) {
  SourceUrlRecorderWebContentsObserver::CreateForWebContents(web_contents);
}

}  // namespace ukm
