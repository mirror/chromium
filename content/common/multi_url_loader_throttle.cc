// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/common/multi_url_loader_throttle.h"

#include "base/bind.h"
#include "base/logging.h"
#include "base/memory/ptr_util.h"

namespace content {

MultiURLLoaderThrottle::MultiURLLoaderThrottleDelegate::
    MultiURLLoaderThrottleDelegate(MultiURLLoaderThrottle* delegate,
                                   URLLoaderThrottle* throttle)
    : delegate_(delegate), throttle_(throttle) {
  throttle_->set_delegate(this);
}

MultiURLLoaderThrottle::MultiURLLoaderThrottleDelegate::
    MultiURLLoaderThrottleDelegate(
        MultiURLLoaderThrottleDelegate&& other) noexcept
    : MultiURLLoaderThrottleDelegate(other.delegate_, other.throttle_) {}

void MultiURLLoaderThrottle::MultiURLLoaderThrottleDelegate::CancelWithError(
    int error_code) {
  delegate_->CancelWithError(error_code);
}

void MultiURLLoaderThrottle::MultiURLLoaderThrottleDelegate::Resume() {
  delegate_->Resume(throttle_);
}

MultiURLLoaderThrottle::MultiURLLoaderThrottle(
    std::vector<std::unique_ptr<URLLoaderThrottle>> throttles)
    : cancel_called_(false), throttles_(std::move(throttles)) {
  delegates_.reserve(throttles_.size());
  for (auto& throttle : throttles_) {
    delegates_.emplace_back(this, throttle.get());
  }
}

MultiURLLoaderThrottle::~MultiURLLoaderThrottle() = default;

void MultiURLLoaderThrottle::WillStartRequest(const ResourceRequest& request,
                                              bool* defer) {
  if (cancel_called_)
    return;

  DCHECK_EQ(0U, blocking_throttles_.size());
  for (auto& throttle : throttles_) {
    bool child_defers = false;
    throttle->WillStartRequest(request, &child_defers);
    if (!HandleDeferResponse(throttle.get(), child_defers, defer))
      return;
  }
}

void MultiURLLoaderThrottle::WillRedirectRequest(
    const net::RedirectInfo& redirect_info,
    bool* defer) {
  if (cancel_called_)
    return;

  DCHECK_EQ(0U, blocking_throttles_.size());
  for (auto& throttle : throttles_) {
    bool child_defers = false;
    throttle->WillRedirectRequest(redirect_info, &child_defers);
    if (!HandleDeferResponse(throttle.get(), child_defers, defer))
      return;
  }
}

void MultiURLLoaderThrottle::WillProcessResponse(
    const ResourceResponseInfo& response_info,
    bool* defer) {
  if (cancel_called_)
    return;

  DCHECK_EQ(0U, blocking_throttles_.size());
  for (auto& throttle : throttles_) {
    bool child_defers = false;
    throttle->WillProcessResponse(response_info, &child_defers);
    if (!HandleDeferResponse(throttle.get(), child_defers, defer))
      return;
  }
}

void MultiURLLoaderThrottle::CancelWithError(int error_code) {
  DCHECK(delegate_);
  if (cancel_called_)
    return;

  cancel_called_ = true;
  delegate_->CancelWithError(error_code);
}

void MultiURLLoaderThrottle::Resume(URLLoaderThrottle* throttle) {
  DCHECK(delegate_);
  if (blocking_throttles_.find(throttle) == blocking_throttles_.end())
    return;

  blocking_throttles_.erase(throttle);

  if (blocking_throttles_.empty() && !cancel_called_)
    delegate_->Resume();
}

bool MultiURLLoaderThrottle::HandleDeferResponse(URLLoaderThrottle* throttle,
                                                 bool throttle_defers,
                                                 bool* defer_out) {
  DCHECK(blocking_throttles_.find(throttle) == blocking_throttles_.end());

  if (cancel_called_)
    return false;

  *defer_out |= throttle_defers;
  if (throttle_defers)
    blocking_throttles_.insert(throttle);
  return true;
}

}  // namespace content
