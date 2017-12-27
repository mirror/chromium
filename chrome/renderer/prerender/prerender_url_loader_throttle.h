// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_RENDERER_PRERENDER_PRERENDER_URL_LOADER_THROTTLE_H_
#define CHROME_RENDERER_PRERENDER_PRERENDER_URL_LOADER_THROTTLE_H_

#include "base/optional.h"
#include "content/public/common/url_loader_throttle.h"
#include "net/base/request_priority.h"

namespace prerender {

class PrerendererURLLoaderThrottle : public content::URLLoaderThrottle {
 public:
  explicit PrerendererURLLoaderThrottle(int render_frame_id);
  ~PrerendererURLLoaderThrottle() override;

 private:
  // content::URLLoaderThrottle implementation.
  void DetachFromCurrentSequence() override;
  void WillStartRequest(content::ResourceRequest* request,
                        bool* defer) override;
  void WillRedirectRequest(const net::RedirectInfo& redirect_info,
                           bool* defer) override;
  void WillProcessResponse(const GURL& response_url,
                           const content::ResourceResponseHead& response_head,
                           bool* defer) override;

  int render_frame_id_;

  // The throttle changes most request priorities to IDLE during prerendering.
  // The priority is reset back to the original priority when prerendering is
  // finished.
  base::Optional<net::RequestPriority> original_request_priority_;
};

}  // namespace prerender

#endif  // CHROME_RENDERER_PRERENDER_PRERENDER_URL_LOADER_THROTTLE_H_
