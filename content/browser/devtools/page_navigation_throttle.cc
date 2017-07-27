// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/devtools/page_navigation_throttle.h"

#include "base/strings/stringprintf.h"
#include "content/browser/devtools/protocol/network_handler.h"
#include "content/browser/devtools/protocol/page.h"
#include "content/common/navigation_params.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/web_contents.h"
#include "url/url_constants.h"

namespace content {

PageNavigationThrottle::PageNavigationThrottle(
    base::WeakPtr<protocol::NetworkHandler> network_handler,
    content::NavigationHandle* navigation_handle)
    : content::NavigationThrottle(navigation_handle),
      network_handler_(network_handler),
      navigation_deferred_(false) {}

PageNavigationThrottle::~PageNavigationThrottle() {
  if (network_handler_)
    network_handler_->OnPageNavigationThrottleDisposed(this);
}

NavigationThrottle::ThrottleCheckResult
PageNavigationThrottle::WillStartRequest() {
  if (!network_handler_)
    return ThrottleCheckResult::PROCEED;
  if (navigation_handle()->IsPost())
    return ThrottleCheckResult::PROCEED;
  GURL url = navigation_handle()->GetURL();
  if (ShouldMakeNetworkRequestForURL(url) && !url.SchemeIs(url::kDataScheme))
    return ThrottleCheckResult::PROCEED;
  navigation_deferred_ = true;
  interception_id_ = base::StringPrintf("throttle-%d",
                                        network_handler_->GetNextThrottleId());
  network_handler_->frontend()->RequestIntercepted(
      interception_id_,
      protocol::NetworkHandler::CreateRequestForThrottle(
          url, "GET", navigation_handle()->GetReferrer()),
      protocol::Page::ResourceTypeEnum::Document,
      true);
  return ThrottleCheckResult::DEFER;
}

NavigationThrottle::ThrottleCheckResult
PageNavigationThrottle::WillProcessResponse() {
  if (!network_handler_)
    return ThrottleCheckResult::PROCEED;
  navigation_deferred_ = true;
  global_request_id_ = navigation_handle()->GetGlobalRequestID();
  return ThrottleCheckResult::DEFER;
}

const char* PageNavigationThrottle::GetNameForLogging() {
  return "PageNavigationThrottle";
}

void PageNavigationThrottle::AlwaysProceed() {
  // Makes WillStartRequest and WillRedirectRequest always return
  // ThrottleCheckResult::PROCEED.
  network_handler_.reset();
  Resume();
}

void PageNavigationThrottle::MarkForRequest(
    const GlobalRequestID global_request_id,
    const std::string& interception_id) {
  if (global_request_id_ == global_request_id)
    interception_id_ = interception_id;
}

// Resumes a deferred navigation request. Does nothing if a response isn't
// expected.
void PageNavigationThrottle::Resume() {
  if (!navigation_deferred_)
    return;
  navigation_deferred_ = false;
  content::NavigationThrottle::Resume();

  // Do not add code after this as the PageNavigationThrottle may be deleted by
  // the line above.
}

// Cancels a deferred navigation request. Does nothing if a response isn't
// expected.
void PageNavigationThrottle::CancelDeferredNavigation(
    NavigationThrottle::ThrottleCheckResult result) {
  if (!navigation_deferred_)
    return;
  navigation_deferred_ = false;
  content::NavigationThrottle::CancelDeferredNavigation(result);

  // Do not add code after this as the PageNavigationThrottle may be deleted by
  // the line above.
}

}  // namespace content
