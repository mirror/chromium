// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/network/resource_scheduler_client.h"

#include "content/network/network_context.h"
#include "net/url_request/url_request_context.h"

namespace content {

ResourceSchedulerClient::ResourceSchedulerClient(int child_id,
                                                 int route_id,
                                                 NetworkContext* context)
    : child_id_(child_id), route_id_(route_id), context_(context) {}

ResourceSchedulerClient::~ResourceSchedulerClient() {
  if (has_actually_created_client_)
    context_->resource_scheduler()->OnClientDeleted(child_id_, route_id_);
}

std::unique_ptr<ResourceScheduler::ScheduledResourceRequest>
ResourceSchedulerClient::ScheduleRequest(bool is_async,
                                         net::URLRequest* url_request) {
  if (!has_actually_created_client_) {
    has_actually_created_client_ = true;
    context_->resource_scheduler()->OnClientCreated(
        child_id_, route_id_,
        context_->url_request_context()->network_quality_estimator());
    // TODO(yhirano): Remove this once RendererSideResourceScheduler is
    // shipped.
    context_->resource_scheduler()->DeprecatedOnNavigate(child_id_, route_id_);
  }
  return context_->resource_scheduler()->ScheduleRequest(child_id_, route_id_,
                                                         is_async, url_request);
}

void ResourceSchedulerClient::ReprioritizeRequest(
    net::URLRequest* request,
    net::RequestPriority new_priority,
    int intra_priority_value) {
  context_->resource_scheduler()->ReprioritizeRequest(request, new_priority,
                                                      intra_priority_value);
}
void ResourceSchedulerClient::OnReceivedSpdyProxiedHttpResponse() {
  context_->resource_scheduler()->OnReceivedSpdyProxiedHttpResponse(child_id_,
                                                                    route_id_);
}

}  // namespace content
