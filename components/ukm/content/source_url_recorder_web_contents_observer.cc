// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/ukm/content/source_url_recorder_web_contents_observer.h"

#include <utility>

#include "base/containers/flat_map.h"
#include "base/macros.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/browser/web_contents_user_data.h"
#include "services/metrics/public/cpp/ukm_recorder.h"
#include "services/metrics/public/cpp/ukm_source_id.h"
#include "url/gurl.h"

namespace {

bool IsValidUkmUrl(const GURL& url) {
  return url.SchemeIsHTTPOrHTTPS() || url.SchemeIs("chrome");
}

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

  void MaybeRecordUrl(content::NavigationHandle* navigation_handle,
                      const GURL& initial_url);

  // Map from navigation ID to the initial URL for that navigation.
  base::flat_map<int64_t, GURL> pending_navigations_;

  DISALLOW_COPY_AND_ASSIGN(SourceUrlRecorderWebContentsObserver);
};

SourceUrlRecorderWebContentsObserver::SourceUrlRecorderWebContentsObserver(
    content::WebContents* web_contents)
    : content::WebContentsObserver(web_contents) {}

void SourceUrlRecorderWebContentsObserver::DidStartNavigation(
    content::NavigationHandle* navigation_handle) {
  // UKM only records URLs for main frame (web page) navigations, so ignore
  // non-main frame navs. Additionally, at least for the time being, we don't
  // track metrics for same-document navigations (e.g. changes in URL fragment,
  // or URL changes due to history.pushState) in UKM.
  if (!navigation_handle->IsInMainFrame() ||
      navigation_handle->IsSameDocument()) {
    return;
  }

  // UKM doesn't want to record URLs for downloads. However, at the point a
  // navigation is started, we don't yet know if the navigation will result in a
  // download. Thus, we record the URL at the time a navigation was initiated,
  // and only record it later, once we verify that the navigation didn't result
  // in a download.
  pending_navigations_.insert(std::make_pair(
      navigation_handle->GetNavigationId(), navigation_handle->GetURL()));
}

void SourceUrlRecorderWebContentsObserver::DidFinishNavigation(
    content::NavigationHandle* navigation_handle) {
  auto it = pending_navigations_.find(navigation_handle->GetNavigationId());
  if (it == pending_navigations_.end())
    return;

  GURL initial_url = std::move(it->second);
  pending_navigations_.erase(it);

  MaybeRecordUrl(navigation_handle, initial_url);
}

void SourceUrlRecorderWebContentsObserver::MaybeRecordUrl(
    content::NavigationHandle* navigation_handle,
    const GURL& initial_url) {
  ukm::UkmRecorder* ukm_recorder = ukm::UkmRecorder::Get();
  if (!ukm_recorder)
    return;

  // Navigations that result in HTTP 204/205s or downloads don't commit
  // (i.e. they do not replace the document currently being hosted in the
  // associated frame). These navigations have valid HTTP response headers, but
  // are assigned an ERR_ABORTED error code by the content layer. We don't want
  // to log UKM for downloads, and 204s/205s aren't interesting to record
  // metrics for, so we ignore both.
  if (!navigation_handle->HasCommitted() &&
      navigation_handle->GetNetErrorCode() == net::ERR_ABORTED &&
      navigation_handle->GetResponseHeaders()) {
    return;
  }

  const ukm::SourceId source_id = ukm::ConvertToSourceId(
      navigation_handle->GetNavigationId(), ukm::SourceIdType::NAVIGATION_ID);

  if (IsValidUkmUrl(initial_url))
    ukm_recorder->UpdateSourceURL(source_id, initial_url);

  const GURL& final_url = navigation_handle->GetURL();
  if (final_url != initial_url && IsValidUkmUrl(final_url))
    ukm_recorder->UpdateSourceURL(source_id, final_url);
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
