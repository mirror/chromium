// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/loader/throttling_resource_handler.h"

#include <utility>

#include "content/browser/loader/resource_controller.h"
#include "content/public/common/resource_response.h"
#include "net/url_request/url_request.h"

namespace content {

ThrottlingResourceHandler::ForwardingThrottleDelegate::
    ForwardingThrottleDelegate(ThrottlingResourceHandler* handler,
                               size_t throttle_index)
    : handler_(handler), throttle_index_(throttle_index) {}
ThrottlingResourceHandler::ForwardingThrottleDelegate::
    ~ForwardingThrottleDelegate() = default;

void ThrottlingResourceHandler::ForwardingThrottleDelegate::Cancel() {
  handler_->Cancel();
}

void ThrottlingResourceHandler::ForwardingThrottleDelegate::CancelWithError(
    int error_code) {
  handler_->CancelWithError(error_code);
}

void ThrottlingResourceHandler::ForwardingThrottleDelegate::Resume() {
  handler_->Resume();
}

void ThrottlingResourceHandler::ForwardingThrottleDelegate::
    PauseReadingBodyFromNet() {
  handler_->RequestToPauseReadingBodyFromNet(throttle_index_);
}

void ThrottlingResourceHandler::ForwardingThrottleDelegate::
    ResumeReadingBodyFromNet() {
  handler_->RequestToResumeReadingBodyFromNet(throttle_index_);
}

ThrottlingResourceHandler::ThrottlingResourceHandler(
    std::unique_ptr<ResourceHandler> next_handler,
    net::URLRequest* request,
    std::vector<std::unique_ptr<ResourceThrottle>> throttles)
    : LayeredResourceHandler(request, std::move(next_handler)),
      deferred_stage_(DEFERRED_NONE),
      throttles_(std::move(throttles)),
      next_index_(0),
      cancelled_by_resource_throttle_(false) {
  throttling_delegates_.reserve(throttles_.size());
  for (size_t i = 0; i < throttles_.size(); ++i) {
    const auto& throttle = throttles_[i];

    throttling_delegates_.push_back(ForwardingThrottleDelegate(this, i));
    throttle->set_delegate(&throttling_delegates_.back());
    // Throttles must have a name, as otherwise, bugs where a throttle fails
    // to resume a request can be very difficult to debug.
    DCHECK(throttle->GetNameForLogging());
  }
}

ThrottlingResourceHandler::~ThrottlingResourceHandler() {
}

void ThrottlingResourceHandler::OnRequestRedirected(
    const net::RedirectInfo& redirect_info,
    ResourceResponse* response,
    std::unique_ptr<ResourceController> controller) {
  DCHECK(!has_controller());
  DCHECK(!cancelled_by_resource_throttle_);

  HoldController(std::move(controller));
  while (next_index_ < throttles_.size()) {
    int index = next_index_;
    bool defer = false;
    throttles_[index]->WillRedirectRequest(redirect_info, &defer);
    next_index_++;
    if (cancelled_by_resource_throttle_)
      return;

    if (defer) {
      OnRequestDeferred(index);
      deferred_stage_ = DEFERRED_REDIRECT;
      deferred_redirect_ = redirect_info;
      deferred_response_ = response;
      return;
    }
  }

  next_index_ = 0;  // Reset for next time.

  next_handler_->OnRequestRedirected(redirect_info, response,
                                     ReleaseController());
}

void ThrottlingResourceHandler::OnWillStart(
    const GURL& url,
    std::unique_ptr<ResourceController> controller) {
  DCHECK(!cancelled_by_resource_throttle_);
  DCHECK(!has_controller());

  HoldController(std::move(controller));
  while (next_index_ < throttles_.size()) {
    int index = next_index_;
    bool defer = false;
    throttles_[index]->WillStartRequest(&defer);
    next_index_++;
    if (cancelled_by_resource_throttle_)
      return;
    if (defer) {
      OnRequestDeferred(index);
      deferred_stage_ = DEFERRED_START;
      deferred_url_ = url;
      return;
    }
  }

  next_index_ = 0;  // Reset for next time.

  return next_handler_->OnWillStart(url, ReleaseController());
}

void ThrottlingResourceHandler::OnResponseStarted(
    ResourceResponse* response,
    std::unique_ptr<ResourceController> controller) {
  DCHECK(!cancelled_by_resource_throttle_);
  DCHECK(!has_controller());

  HoldController(std::move(controller));
  while (next_index_ < throttles_.size()) {
    int index = next_index_;
    bool defer = false;
    throttles_[index]->WillProcessResponse(&defer);
    next_index_++;
    if (cancelled_by_resource_throttle_)
      return;
    if (defer) {
      OnRequestDeferred(index);
      deferred_stage_ = DEFERRED_RESPONSE;
      deferred_response_ = response;
      return;
    }
  }

  next_index_ = 0;  // Reset for next time.

  return next_handler_->OnResponseStarted(response, ReleaseController());
}

void ThrottlingResourceHandler::Cancel() {
  if (!has_controller()) {
    OutOfBandCancel(net::ERR_ABORTED, false /* tell_renderer */);
    return;
  }
  cancelled_by_resource_throttle_ = true;
  ResourceHandler::Cancel();
}

void ThrottlingResourceHandler::CancelWithError(int error_code) {
  if (!has_controller()) {
    OutOfBandCancel(error_code, false /* tell_renderer */);
    return;
  }
  cancelled_by_resource_throttle_ = true;
  ResourceHandler::CancelWithError(error_code);
}

void ThrottlingResourceHandler::Resume() {
  // Throttles expect to be able to cancel requests out-of-band, so just do
  // nothing if one request resumes after another cancels. Can't even recognize
  // out-of-band cancels and for synchronous teardown, since don't know if the
  // currently active throttle called Cancel() or if it was another one.
  if (cancelled_by_resource_throttle_)
    return;

  DCHECK(has_controller());

  DeferredStage last_deferred_stage = deferred_stage_;
  deferred_stage_ = DEFERRED_NONE;
  // Clear information about the throttle that delayed the request.
  request()->LogUnblocked();
  switch (last_deferred_stage) {
    case DEFERRED_NONE:
      NOTREACHED();
      break;
    case DEFERRED_START:
      ResumeStart();
      break;
    case DEFERRED_REDIRECT:
      ResumeRedirect();
      break;
    case DEFERRED_RESPONSE:
      ResumeResponse();
      break;
  }
}

void ThrottlingResourceHandler::RequestToPauseReadingBodyFromNet(
    size_t throttle_index) {
  bool should_call_pause = pausing_reading_body_from_net_throttles_.empty();
  pausing_reading_body_from_net_throttles_.insert(throttle_index);

  if (should_call_pause)
    PauseReadingBodyFromNet();
}

void ThrottlingResourceHandler::RequestToResumeReadingBodyFromNet(
    size_t throttle_index) {
  auto iter = pausing_reading_body_from_net_throttles_.find(throttle_index);
  if (iter == pausing_reading_body_from_net_throttles_.end())
    return;

  pausing_reading_body_from_net_throttles_.erase(iter);
  if (pausing_reading_body_from_net_throttles_.empty())
    ResumeReadingBodyFromNet();
}

void ThrottlingResourceHandler::ResumeStart() {
  DCHECK(!cancelled_by_resource_throttle_);
  DCHECK(has_controller());

  GURL url = deferred_url_;
  deferred_url_ = GURL();

  OnWillStart(url, ReleaseController());
}

void ThrottlingResourceHandler::ResumeRedirect() {
  DCHECK(!cancelled_by_resource_throttle_);
  DCHECK(has_controller());

  net::RedirectInfo redirect_info = deferred_redirect_;
  deferred_redirect_ = net::RedirectInfo();
  scoped_refptr<ResourceResponse> response;
  deferred_response_.swap(response);

  OnRequestRedirected(redirect_info, response.get(), ReleaseController());
}

void ThrottlingResourceHandler::ResumeResponse() {
  DCHECK(!cancelled_by_resource_throttle_);
  DCHECK(has_controller());

  scoped_refptr<ResourceResponse> response;
  deferred_response_.swap(response);

  OnResponseStarted(response.get(), ReleaseController());
}

void ThrottlingResourceHandler::OnRequestDeferred(int throttle_index) {
  request()->LogBlockedBy(throttles_[throttle_index]->GetNameForLogging());
}

}  // namespace content
