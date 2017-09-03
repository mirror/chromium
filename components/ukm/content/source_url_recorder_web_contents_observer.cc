// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/ukm/content/source_url_recorder_web_contents_observer.h"

#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/web_contents.h"
#include "services/metrics/public/cpp/ukm_recorder.h"
#include "services/metrics/public/cpp/ukm_source_id.h"
#include "url/gurl.h"

DEFINE_WEB_CONTENTS_USER_DATA_KEY(ukm::SourceUrlRecorderWebContentsObserver);

namespace ukm {

// static
void SourceUrlRecorderWebContentsObserver::CreateForWebContents(
    content::WebContents* web_contents,
    UkmRecorder* ukm_recorder) {
  // If we don't have a UkmRecorder, or there's already a
  // SourceUrlRecorderWebContentsObserver instance for this WebContents, then
  // exit early.
  if (!ukm_recorder || FromWebContents(web_contents))
    return;

  web_contents->SetUserData(
      UserDataKey(), base::WrapUnique(new SourceUrlRecorderWebContentsObserver(
                         web_contents, ukm_recorder)));
}

SourceUrlRecorderWebContentsObserver::SourceUrlRecorderWebContentsObserver(
    content::WebContents* web_contents,
    UkmRecorder* ukm_recorder)
    : content::WebContentsObserver(web_contents), ukm_recorder_(ukm_recorder) {
  CHECK_NE(nullptr, ukm_recorder_);
}

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
  if (!navigation_handle->IsInMainFrame() ||
      navigation_handle->IsSameDocument()) {
    return;
  }

  const GURL& url = navigation_handle->GetURL();
  if (!url.SchemeIsHTTPOrHTTPS() && !url.SchemeIs("chrome-extension"))
    return;

  ukm::SourceId source_id = ukm::ConvertToSourceId(
      navigation_handle->GetNavigationId(), ukm::SourceIdType::NAVIGATION_ID);
  ukm_recorder_->UpdateSourceURL(source_id, url);
}

}  // namespace ukm
