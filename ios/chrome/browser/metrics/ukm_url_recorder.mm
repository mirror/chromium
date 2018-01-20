
// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ios/chrome/browser/metrics/ukm_url_recorder.h"

#include <utility>

#include "base/containers/flat_map.h"
#include "base/macros.h"
#include "ios/web/public/web_state/navigation_context.h"
#include "ios/web/public/web_state/web_state.h"
#include "ios/web/public/web_state/web_state_observer.h"
#include "ios/web/public/web_state/web_state_user_data.h"
#include "services/metrics/public/cpp/ukm_recorder.h"
#include "services/metrics/public/cpp/ukm_source_id.h"
#include "url/gurl.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace ukm {

namespace internal {

// Assign Ids to navigations.
uint64_t NewNavigationId() {
  static uint64_t last_id = 0;
  return ++last_id;
}

// SourceUrlRecorderWebStateObserver is responsible for recording UKM source
// URLs, for all (any only) main frame navigations in a given WebState.
// SourceUrlRecorderWebStateObserver records both the final URL for a
// navigation, and, if the navigation was redirected, the initial URL as well.
class SourceUrlRecorderWebStateObserver
    : public web::WebStateObserver,
      public web::WebStateUserData<SourceUrlRecorderWebStateObserver> {
 public:
  // Creates a SourceUrlRecorderWebStateObserver for the given
  // WebState. If a SourceUrlRecorderWebStateObserver is already
  // associated with the WebState, this method is a no-op.
  static void CreateForWebState(web::WebState* web_state);

  // web::WebStateObserver
  void DidFinishNavigation(web::WebState* web_state,
                           web::NavigationContext* navigation_context) override;
  void WebStateDestroyed(web::WebState* web_state) override;

  ukm::SourceId GetLastCommittedSourceId();

 private:
  explicit SourceUrlRecorderWebStateObserver(web::WebState* web_state);
  friend class web::WebStateUserData<SourceUrlRecorderWebStateObserver>;

  int64_t last_committed_source_id_;

  DISALLOW_COPY_AND_ASSIGN(SourceUrlRecorderWebStateObserver);
};

SourceUrlRecorderWebStateObserver::SourceUrlRecorderWebStateObserver(
    web::WebState* web_state)
    : last_committed_source_id_(ukm::kInvalidSourceId) {
  web_state->AddObserver(this);
}

void SourceUrlRecorderWebStateObserver::WebStateDestroyed(
    web::WebState* web_state) {
  web_state->RemoveObserver(this);
}

void SourceUrlRecorderWebStateObserver::DidFinishNavigation(
    web::WebState* web_state,
    web::NavigationContext* navigation_context) {
  // WebState:DidStartNavigation should only be called for main-frame
  // navigations, and UKM only records URLs for main frame navigations.
  // Additionally, at least for the time being, we don't
  // track metrics for same-document navigations (e.g. changes in URL fragment,
  // or URL changes due to history.pushState) in UKM.
  if (navigation_context->IsSameDocument()) {
    return;
  }

  // TODO(crbug.com/792662): Assign these to navigations earlier and retrieve
  // them here.
  uint64_t navigation_id = NewNavigationId();
  ukm::SourceId source_id =
      ukm::ConvertToSourceId(navigation_id, ukm::SourceIdType::NAVIGATION_ID);

  if (!navigation_context->GetError()) {
    last_committed_source_id_ = source_id;
  }

  // TODO(crbug.com/792662): Record initial URL too.
  const GURL& final_url = navigation_context->GetUrl();
  ukm::UkmRecorder::Get()->UpdateSourceURL(source_id, final_url);
}

ukm::SourceId SourceUrlRecorderWebStateObserver::GetLastCommittedSourceId() {
  return last_committed_source_id_;
}

// static
void SourceUrlRecorderWebStateObserver::CreateForWebState(
    web::WebState* web_state) {
  if (!SourceUrlRecorderWebStateObserver::FromWebState(web_state)) {
    web_state->SetUserData(
        SourceUrlRecorderWebStateObserver::UserDataKey(),
        base::WrapUnique(new SourceUrlRecorderWebStateObserver(web_state)));
  }
}

}  // namespace internal

void InitializeSourceUrlRecorderForWebState(web::WebState* web_state) {
  internal::SourceUrlRecorderWebStateObserver::CreateForWebState(web_state);
}

SourceId GetSourceIdForWebStateDocument(web::WebState* web_state) {
  internal::SourceUrlRecorderWebStateObserver* obs =
      internal::SourceUrlRecorderWebStateObserver::FromWebState(web_state);
  return obs ? obs->GetLastCommittedSourceId() : kInvalidSourceId;
}

}  // namespace ukm

DEFINE_WEB_STATE_USER_DATA_KEY(
    ukm::internal::SourceUrlRecorderWebStateObserver);
