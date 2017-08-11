// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/common/multi_url_loader_throttle.h"

#include "base/bind.h"
#include "base/logging.h"
#include "base/memory/ptr_util.h"

namespace content {

class MultiURLLoaderThrottleDelegate : public URLLoaderThrottle::Delegate {
 public:
  using CancelWithErrorCallback = base::Callback<void(int)>;
  using ResumeCallback = base::Closure;

  MultiURLLoaderThrottleDelegate(const CancelWithErrorCallback& cancel_callback,
                                 const ResumeCallback& resume_callback)
      : cancel_callback_(cancel_callback), resume_callback_(resume_callback) {}

  ~MultiURLLoaderThrottleDelegate() override = default;

  // URLLoaderThrottle::Delegate implementation.
  void CancelWithError(int error_code) override {
    cancel_callback_.Run(error_code);
  }

  void Resume() override { resume_callback_.Run(); }

 private:
  CancelWithErrorCallback cancel_callback_;
  ResumeCallback resume_callback_;
};

MultiURLLoaderThrottle::MultiURLLoaderThrottle(
    std::vector<std::unique_ptr<URLLoaderThrottle>> throttles)
    : cancel_called_(false), throttles_(std::move(throttles)) {
  for (auto& throttle : throttles_) {
    auto delegate = base::MakeUnique<MultiURLLoaderThrottleDelegate>(
        base::Bind(&MultiURLLoaderThrottle::CancelWithError,
                   base::Unretained(this)),
        base::Bind(&MultiURLLoaderThrottle::Resume, base::Unretained(this),
                   throttle.get()));
    throttle->set_delegate(delegate.get());
    delegates_.insert(std::move(delegate));
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
  // TODO(dtrainor): Should we DCHECK(delegate_) here?  Is it even possible to
  // get a cancel without a delegate?  In the short term, don't even pretend we
  // actually cancelled and don't set cancelled_ to |true|.
  if (!delegate_ || cancel_called_)
    return;

  cancel_called_ = true;
  delegate_->CancelWithError(error_code);
}

void MultiURLLoaderThrottle::Resume(URLLoaderThrottle* throttle) {
  if (blocking_throttles_.find(throttle) == blocking_throttles_.end())
    return;

  blocking_throttles_.erase(throttle);

  if (blocking_throttles_.size() == 0 && delegate_ && !cancel_called_)
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
