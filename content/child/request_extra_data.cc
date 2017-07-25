// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/child/request_extra_data.h"

#include "content/common/service_worker/service_worker_types.h"
#include "content/public/common/resource_request.h"
#include "content/public/common/service_worker_modes.h"
#include "ipc/ipc_message.h"

using blink::WebString;

namespace content {

RequestExtraData::RequestExtraData()
    : transition_type_(ui::PAGE_TRANSITION_LINK),
      service_worker_provider_id_(kInvalidServiceWorkerProviderId),
      originated_from_service_worker_(false),
      initiated_in_secure_context_(false),
      is_prefetch_(false),
      download_to_network_cache_only_(false),
      block_mixed_plugin_content_(false),
      navigation_initiated_by_renderer_(false) {}

RequestExtraData::~RequestExtraData() {
}

void RequestExtraData::CopyToResourceRequest(ResourceRequest* request) const {
  request->transition_type = transition_type_;
  request->service_worker_provider_id = service_worker_provider_id_;
  request->originated_from_service_worker = originated_from_service_worker_;

  request->initiated_in_secure_context = initiated_in_secure_context_;
  request->download_to_network_cache_only = download_to_network_cache_only_;
}

}  // namespace content
