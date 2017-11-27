// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/public/test/url_loader_interceptor.h"

#include "content/browser/loader/resource_message_filter.h"
#include "content/browser/loader/url_loader_factory_impl.h"
#include "content/browser/storage_partition_impl.h"
#include "content/browser/url_loader_factory_getter.h"
#include "content/public/common/content_features.h"
#include "content/public/common/url_loader.mojom.h"

namespace content {

class URLLoaderInterceptor::Interceptor : public mojom::URLLoaderFactory {
 public:
  typedef base::Callback<int()> ProcessIdGetter;
  typedef base::Callback<mojom::URLLoaderFactory*()> OriginalFactoryGetter;

  Interceptor(URLLoaderInterceptor* parent,
              const ProcessIdGetter& process_id_getter,
              const OriginalFactoryGetter& original_factory_getter)
      : parent_(parent),
        process_id_getter_(process_id_getter),
        original_factory_getter_(original_factory_getter) {}

  ~Interceptor() override {}

 private:
  // mojom::URLLoaderFactory implementation:
  void CreateLoaderAndStart(mojom::URLLoaderRequest request,
                            int32_t routing_id,
                            int32_t request_id,
                            uint32_t options,
                            const ResourceRequest& url_request,
                            mojom::URLLoaderClientPtr client,
                            const net::MutableNetworkTrafficAnnotationTag&
                                traffic_annotation) override {
    RequestParams params;
    params.request = std::move(request);
    params.routing_id = routing_id;
    params.request_id = request_id;
    params.options = options;
    params.url_request = std::move(url_request);
    params.client = std::move(client);
    params.traffic_annotation = traffic_annotation;
    // We don't intercept the blob URL for plznavigate requests.
    if (url_request.resource_body_stream_url.is_empty() &&
        parent_->callback_.Run(&params, process_id_getter_.Run()))
      return;

    original_factory_getter_.Run()->CreateLoaderAndStart(
        std::move(params.request), params.routing_id, params.request_id,
        params.options, std::move(params.url_request), std::move(params.client),
        params.traffic_annotation);
  }

  void Clone(mojom::URLLoaderFactoryRequest request) override { NOTREACHED(); }

  URLLoaderInterceptor* parent_;
  ProcessIdGetter process_id_getter_;
  OriginalFactoryGetter original_factory_getter_;

  DISALLOW_COPY_AND_ASSIGN(Interceptor);
};

URLLoaderInterceptor::RequestParams::RequestParams() = default;
URLLoaderInterceptor::RequestParams::~RequestParams() = default;

URLLoaderInterceptor::URLLoaderInterceptor(const InterceptCallback& callback,
                                           StoragePartition* storage_partition,
                                           bool intercept_frame_requests,
                                           bool intercept_subresources)
    : callback_(callback), storage_partition_(storage_partition) {
  if (intercept_frame_requests) {
    if (base::FeatureList::IsEnabled(features::kNetworkService)) {
      auto factory_getter =
          static_cast<StoragePartitionImpl*>(storage_partition)
              ->url_loader_factory_getter();
      frame_interceptor_ = std::make_unique<Interceptor>(
          this, base::Bind([]() { return 0; }),
          base::Bind(
              [](scoped_refptr<URLLoaderFactoryGetter> factory_getter) {
                mojom::URLLoaderFactory* factory =
                    factory_getter->network_factory_for_testing()->get();
                return factory;
              },
              factory_getter));
      BrowserThread::PostTask(
          BrowserThread::IO, FROM_HERE,
          base::Bind(&URLLoaderFactoryGetter::SetNetworkFactoryForTesting,
                     factory_getter, frame_interceptor_.get()));
    } else {
      // Once http://crbug.com/747130 is fixed, the codepath above will work for
      // the non-network service code path.
    }
  }

  if (intercept_subresources) {
    if (base::FeatureList::IsEnabled(features::kNetworkService)) {
    } else {
      rmf_interceptor_ = std::make_unique<Interceptor>(
          this, base::Bind([]() {
            return ResourceMessageFilter::GetCurrentForTesting()->child_id();
          }),
          base::Bind([]() {
            mojom::URLLoaderFactory* factory =
                ResourceMessageFilter::GetCurrentForTesting();
            return factory;
          }));
      BrowserThread::PostTask(
          BrowserThread::IO, FROM_HERE,
          base::Bind(&ResourceMessageFilter::SetNetworkFactoryForTesting,
                     rmf_interceptor_.get()));
    }
  }
}

URLLoaderInterceptor::~URLLoaderInterceptor() {
  if (frame_interceptor_) {
    auto factory_getter = static_cast<StoragePartitionImpl*>(storage_partition_)
                              ->url_loader_factory_getter();
    BrowserThread::PostTask(
        BrowserThread::IO, FROM_HERE,
        base::Bind(&URLLoaderFactoryGetter::SetNetworkFactoryForTesting,
                   factory_getter, nullptr));
  }

  if (rmf_interceptor_) {
    BrowserThread::PostTask(
        BrowserThread::IO, FROM_HERE,
        base::Bind(&ResourceMessageFilter::SetNetworkFactoryForTesting,
                   nullptr));
  }

  if (frame_interceptor_ || rmf_interceptor_) {
    base::RunLoop run_loop;
    BrowserThread::PostTask(BrowserThread::IO, FROM_HERE,
                            run_loop.QuitClosure());
  }
}

void URLLoaderInterceptor::CreateURLLoaderFactory(
    mojom::URLLoaderFactoryRequest request,
    uint32_t process_id) {}

}  // namespace content
