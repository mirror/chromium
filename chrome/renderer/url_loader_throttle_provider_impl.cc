// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/renderer/url_loader_throttle_provider_impl.h"

#include <memory>

#include "base/feature_list.h"
#include "components/safe_browsing/renderer/renderer_url_loader_throttle.h"
#include "content/public/common/content_features.h"
#include "content/public/common/service_names.mojom.h"
#include "content/public/renderer/render_thread.h"
#include "services/service_manager/public/cpp/connector.h"

URLLoaderThrottleProviderImpl::URLLoaderThrottleProviderImpl(ProviderType type)
    : type_(type) {
  DETACH_FROM_THREAD(thread_checker_);

  if (base::FeatureList::IsEnabled(features::kNetworkService)) {
    content::RenderThread::Get()->GetConnector()->BindInterface(
        content::mojom::kBrowserServiceName,
        mojo::MakeRequest(&safe_browsing_info_));
  }
}

URLLoaderThrottleProviderImpl::~URLLoaderThrottleProviderImpl() {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
}

std::vector<std::unique_ptr<content::URLLoaderThrottle>>
URLLoaderThrottleProviderImpl::CreateThrottles(
    int render_frame_id,
    const blink::WebURL& url,
    content::ResourceType resource_type) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);

  std::vector<std::unique_ptr<content::URLLoaderThrottle>> throttles;

  if (!base::FeatureList::IsEnabled(features::kNetworkService))
    return throttles;

  // Don't add these throttles for frame requests, because they've already been
  // added in the browser.
  if (type_ == FOR_MAIN_THREAD && content::IsResourceTypeFrame(resource_type))
    return throttles;

  if (safe_browsing_info_)
    safe_browsing_.Bind(std::move(safe_browsing_info_));
  throttles.push_back(
      std::make_unique<safe_browsing::RendererURLLoaderThrottle>(
          safe_browsing_.get(), render_frame_id));
  return throttles;
}
