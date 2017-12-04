// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_LOADER_WEBPACKAGE_SUBRESOURCE_MANAGER_IMPL_H_
#define CONTENT_RENDERER_LOADER_WEBPACKAGE_SUBRESOURCE_MANAGER_IMPL_H_

#include "content/common/webpackage_subresource_manager.mojom.h"

#include <map>

#include "base/containers/flat_set.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "content/common/webpackage_response_handler.h"
#include "content/public/common/url_loader.mojom.h"
#include "content/public/common/url_loader_factory.mojom.h"
#include "content/public/renderer/child_url_loader_factory_getter.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "url/gurl.h"

namespace content {

class ChildURLLoaderFactoryGetter;

class WebPackageSubresourceManagerImpl
    : public mojom::WebPackageSubresourceManager,
      public base::RefCounted<WebPackageSubresourceManagerImpl> {
 public:
  using ResourceCallback = WebPackageResourceCallback;
  using CompletionCallback = WebPackageCompletionCallback;
  using RequestURLAndMethod = std::pair<GURL, std::string>;
  using PendingCallbacks = WebPackagePendingCallbacks;

  static void CreateLoaderFactory(
      mojom::URLLoaderFactoryRequest factory_request,
      mojom::WebPackageSubresourceManagerRequest subresource_manager_request,
      ChildURLLoaderFactoryGetter::Info factory_getter_info);

  explicit WebPackageSubresourceManagerImpl(
      mojom::WebPackageSubresourceManagerRequest request);

  // If no resource is found |callback| is fired with non-zero |error_code|
  // (which encodes network error code).
  // If |error_code| is net::OK (which is zero), valid |response_head| and
  // |handle| must be also given.
  // |completion_callback| is additionally called only if |callback| is fired
  // with net::OK, and when streaming the response body is finished.
  void GetResource(const ResourceRequest& resource_request,
                   ResourceCallback callback,
                   CompletionCallback completion_callback);

 private:
  class ResponseHandler;
  friend class base::RefCounted<WebPackageSubresourceManagerImpl>;

  enum State {
    kInitial,
    kRequestStarted,
    kResponseStarted,
    kFinished,
  };

  ~WebPackageSubresourceManagerImpl() override;

  // mojom::WebPackageSubresourceManager:
  // void OnRequest(mojom::WebPackageResourceRequestPtr request) override;
  void OnRequests(
      std::vector<mojom::WebPackageResourceRequestPtr> requests) override;
  void OnResponse(mojom::WebPackageResourceRequestPtr request,
                  const ResourceResponseHead& response_head,
                  mojo::ScopedDataPipeConsumerHandle response_body,
                  mojom::WebPackageResponseCompletionCallbackRequest
                      callback_request) override;
  void OnResponseHandlerFinished(const RequestURLAndMethod& request);
  void AbortNotFoundPendingRequests();

  struct PendingResponse {
    PendingResponse(
        const ResourceResponseHead& response_head,
        mojo::ScopedDataPipeConsumerHandle body_handle,
        mojom::WebPackageResponseCompletionCallbackRequest callback_request);
    ~PendingResponse();
    PendingResponse(PendingResponse&& other);
    PendingResponse& operator=(PendingResponse&& other);

    ResourceResponseHead response_head;
    mojo::ScopedDataPipeConsumerHandle body_handle;
    mojom::WebPackageResponseCompletionCallbackRequest callback_request;
  };

  base::flat_set<RequestURLAndMethod> request_set_;
  std::map<RequestURLAndMethod, std::unique_ptr<ResponseHandler>> response_map_;
  std::map<RequestURLAndMethod, PendingResponse> pending_response_map_;
  std::map<RequestURLAndMethod, PendingCallbacks> pending_request_map_;

  State state_ = kInitial;
  mojo::Binding<WebPackageSubresourceManager> binding_;
};

}  // namespace content

#endif  // CONTENT_RENDERER_LOADER_WEBPACKAGE_SUBRESOURCE_MANAGER_IMPL_H_
