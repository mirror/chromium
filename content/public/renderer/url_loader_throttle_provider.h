// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_PUBLIC_RENDERER_URL_LOADER_THROTTLE_PROVIDER_H_
#define CONTENT_PUBLIC_RENDERER_URL_LOADER_THROTTLE_PROVIDER_H_

#include <memory>
#include <vector>

#include "content/common/content_export.h"
#include "content/public/common/resource_type.h"
#include "content/public/common/url_loader_throttle.h"
#include "third_party/WebKit/public/platform/WebURL.h"

namespace content {

class CONTENT_EXPORT URLLoaderThrottleProvider {
 public:
  enum ProviderType { PROVIDER_TYPE_FRAME, PROVIDER_TYPE_WORKER };

  virtual ~URLLoaderThrottleProvider() {}

  // For requests from frames and dedicated workers, |render_frame_id| should be
  // set to the frame where the request comes from. For requests from shared or
  // service workers, |render_frame_id| should be set to MSG_ROUTING_NONE.
  virtual std::vector<std::unique_ptr<URLLoaderThrottle>> CreateThrottles(
      int render_frame_id,
      const blink::WebURL& url,
      ResourceType resource_type) = 0;
};

}  // namespace content

#endif  // CONTENT_PUBLIC_RENDERER_URL_LOADER_THROTTLE_PROVIDER_H_
