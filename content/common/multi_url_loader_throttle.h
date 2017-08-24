// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_COMMON_MULTI_URL_LOADER_THROTTLE_H_
#define CONTENT_COMMON_MULTI_URL_LOADER_THROTTLE_H_

#include <memory>
#include <set>
#include <vector>

#include "base/macros.h"
#include "content/common/content_export.h"
#include "content/public/common/url_loader_throttle.h"

namespace content {

// Helper implementation of URLLoaderThrottle that wraps a list of other
// URLLoaderThrottles.  This has the following behavior characteristics:
//
// From the perspective of a URLLoaderThrottle::Delegate:
// - Only the first call to CancelWithError() will be propagated.  After this no
//   Resume() calls will make it through and no further URLLoaderThrottle calls
//   will be propagated to the list of throttles.
// - Resume() will only be called once all deferring URLLoaderThrottles have
//   called Resume().  If a throttle in the list calls Resume() twice subsequent
//   resume calls will be ignored.
//
// From the perspective of a URLLoaderThrottle:
// - All URLLoaderThrottle calls will be propagated to the whole list except if
//   the CancelWithError() Delegate API is called.  At that point no more calls
//   will make it through.
// - |defer| on all calls will be |true| if any throttle in the list attempts to
//   defer the request.  All throttles will still be notified.  This is
//   different from the ThrottlingResourceHandler, which does not notify
//   subsequent throttles until the first blocking one resumes.
class CONTENT_EXPORT MultiURLLoaderThrottle : public URLLoaderThrottle {
 public:
  explicit MultiURLLoaderThrottle(
      std::vector<std::unique_ptr<URLLoaderThrottle>> throttles);
  ~MultiURLLoaderThrottle() override;

  // URLLoaderThrottle implementation.
  void WillStartRequest(const ResourceRequest& request, bool* defer) override;
  void WillRedirectRequest(const net::RedirectInfo& redirect_info,
                           bool* defer) override;
  void WillProcessResponse(const ResourceResponseInfo& response_info,
                           bool* defer) override;

 private:
  // Helper URLLoaderThrottle::Delegate used internally for rerouting delegate
  // calls to this class and associating each call with the respective
  // URLLoaderThrottle that made it.
  class MultiURLLoaderThrottleDelegate : public URLLoaderThrottle::Delegate {
   public:
    MultiURLLoaderThrottleDelegate(MultiURLLoaderThrottle* delegate,
                                   URLLoaderThrottle* throttle);
    MultiURLLoaderThrottleDelegate(
        MultiURLLoaderThrottleDelegate&& other) noexcept;
    ~MultiURLLoaderThrottleDelegate() override = default;

    // URLLoaderThrottle::Delegate implementation.
    void CancelWithError(int error_code) override;
    void Resume() override;

   private:
    MultiURLLoaderThrottle* const delegate_;
    URLLoaderThrottle* const throttle_;

    DISALLOW_COPY_AND_ASSIGN(MultiURLLoaderThrottleDelegate);
  };

  // URLLoaderThrottle::Delegate implementation that also exposes the calling
  // throttle.
  void CancelWithError(int error_code);
  void Resume(URLLoaderThrottle* throttle);

  // Helper method to handle a defer response from a URLLoaderThrottle call to
  // an owned throttle.  Returns |true| if it's ok to continue calling out to
  // other throttles and |false| otherwise.
  bool HandleDeferResponse(URLLoaderThrottle* throttle,
                           bool throttle_defers,
                           bool* defer_out);

  // One of the owned URLLoaderThrottles has cancelled the request.  This will
  // block propagation of other calls to this class or it's underlying
  // throttles.
  bool cancel_called_;

  std::vector<MultiURLLoaderThrottleDelegate> delegates_;

  // Needs to be destroyed before |delegates_| as each URLLoaderThrottle has a
  // reference to an associated URLLoaderThrottle::Delegate.
  std::vector<std::unique_ptr<URLLoaderThrottle>> throttles_;

  // A list of URLLoaderThrottles that are currently blocking the URLLoader.
  std::set<URLLoaderThrottle*> blocking_throttles_;

  DISALLOW_COPY_AND_ASSIGN(MultiURLLoaderThrottle);
};

}  // namespace content

#endif  // CONTENT_COMMON_MULTI_URL_LOADER_THROTTLE_H_
