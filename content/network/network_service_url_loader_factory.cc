// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/network/network_service_url_loader_factory.h"

#include "base/logging.h"
#include "content/network/network_context.h"
#include "content/network/network_service_impl.h"
#include "content/network/url_loader.h"
#include "net/url_request/url_request_context.h"
#include "services/network/public/cpp/resource_request.h"

namespace content {

NetworkServiceURLLoaderFactory::NetworkServiceURLLoaderFactory(
    NetworkContext* context,
    uint32_t process_id)
    : context_(context), process_id_(process_id) {
  ignore_result(process_id_);
}

NetworkServiceURLLoaderFactory::~NetworkServiceURLLoaderFactory() {
  if (scheduler_client_status_ == SchedulerClientStatus::kAttached) {
    context_->resource_scheduler()->OnClientDeleted(process_id_,
                                                    scheduler_client_id_);
  }
}

void NetworkServiceURLLoaderFactory::CreateLoaderAndStart(
    mojom::URLLoaderRequest request,
    int32_t routing_id,
    int32_t request_id,
    uint32_t options,
    const network::ResourceRequest& url_request,
    mojom::URLLoaderClientPtr client,
    const net::MutableNetworkTrafficAnnotationTag& traffic_annotation) {
  // A NetworkServiceURLLoaderFactory is attached with at most one frame.
  switch (scheduler_client_status_) {
    case SchedulerClientStatus::kNotYetAttached:
      if (url_request.render_frame_id > 0) {
        scheduler_client_status_ = SchedulerClientStatus::kAttached;
        scheduler_client_id_ = url_request.render_frame_id;
        context_->resource_scheduler()->OnClientCreated(
            process_id_, scheduler_client_id_,
            context_->url_request_context()->network_quality_estimator());
        // TODO(yhirano): Remove this once RendererSideResourceScheduler is
        // shipped.
        context_->resource_scheduler()->DeprecatedOnNavigate(
            process_id_, scheduler_client_id_);
      } else {
        scheduler_client_status_ = SchedulerClientStatus::kNotAttached;
      }
      break;
    case SchedulerClientStatus::kNotAttached:
      DCHECK_EQ(0, url_request.render_frame_id);
      break;
    case SchedulerClientStatus::kAttached:
      DCHECK(url_request.render_frame_id == scheduler_client_id_ ||
             url_request.render_frame_id == 0);
      break;
  }

  bool report_raw_headers = false;
  if (url_request.report_raw_headers) {
    const NetworkServiceImpl* service = context_->network_service();
    report_raw_headers = service && service->HasRawHeadersAccess(process_id_);
    if (!report_raw_headers)
      DLOG(ERROR) << "Denying raw headers request by process " << process_id_;
  }
  new URLLoader(
      context_, std::move(request), options, url_request, report_raw_headers,
      std::move(client),
      static_cast<net::NetworkTrafficAnnotationTag>(traffic_annotation),
      process_id_);
}

void NetworkServiceURLLoaderFactory::Clone(
    mojom::URLLoaderFactoryRequest request) {
  context_->CreateURLLoaderFactory(std::move(request), process_id_);
}

}  // namespace content
