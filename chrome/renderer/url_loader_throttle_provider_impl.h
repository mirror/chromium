// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_RENDERER_URL_LOADER_THROTTLE_PROVIDER_IMPL_H_
#define CHROME_RENDERER_URL_LOADER_THROTTLE_PROVIDER_IMPL_H_

#include "components/safe_browsing/common/safe_browsing.mojom.h"
#include "content/public/renderer/url_loader_throttle_provider.h"

class URLLoaderThrottleProviderImpl
    : public content::URLLoaderThrottleProvider {
 public:
  explicit URLLoaderThrottleProviderImpl(ProviderType type);

  ~URLLoaderThrottleProviderImpl() override;

  // content::URLLoaderThrottleProvider implementation.
  std::vector<std::unique_ptr<content::URLLoaderThrottle>> CreateThrottles(
      int render_frame_id,
      const blink::WebURL& url,
      content::ResourceType resource_type) override;

 private:
  ProviderType type_;

  safe_browsing::mojom::SafeBrowsingPtrInfo safe_browsing_info_;
  safe_browsing::mojom::SafeBrowsingPtr safe_browsing_;

  DISALLOW_COPY_AND_ASSIGN(URLLoaderThrottleProviderImpl);
};

#endif  // CHROME_RENDERER_URL_LOADER_THROTTLE_PROVIDER_IMPL_H_
