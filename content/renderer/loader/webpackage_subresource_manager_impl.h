// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_LOADER_WEBPACKAGE_SUBRESOURCE_MANAGER_IMPL_H_
#define CONTENT_RENDERER_LOADER_WEBPACKAGE_SUBRESOURCE_MANAGER_IMPL_H_

#include "content/common/webpackage_subresource_manager.mojom.h"

#include <set>
#include <map>

#include "base/memory/weak_ptr.h"
#include "content/common/webpackage_response_handler.h"
#include "content/public/common/url_loader.mojom.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "url/gurl.h"

namespace content {

class ChildURLLoaderFactoryGetter;

class WebPackageSubresourceManagerImpl :
    public mojom::WebPackageSubresourceManager {
 public:
  using ResourceCallback = WebPackageResourceCallback;
  using CompletionCallback = WebPackageCompletionCallback;
  using RequestURLAndMethod = std::pair<GURL, std::string>;
  using PendingCallbacks = WebPackagePendingCallbacks;

  WebPackageSubresourceManagerImpl(
      mojom::WebPackageSubresourceManagerRequest request,
      scoped_refptr<ChildURLLoaderFactoryGetter> loader_factory_getter);
  ~WebPackageSubresourceManagerImpl() override;

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

  enum State {
    kInitial,
    kRequestStarted,
    kResponseStarted,
    kFinished,
  };

  // mojom::WebPackageSubresourceManager:
  void OnRequest(const GURL& url, const std::string& method) override;
  void OnResponse(const GURL& url,
                  const std::string& method,
                  const content::ResourceResponseHead& response_head,
                  mojo::ScopedDataPipeConsumerHandle response_body,
                  mojom::WebPackageResponseCompletionCallbackRequest
                      callback_request) override;
  void OnResponseHandlerFinished(const RequestURLAndMethod& request);
  void AbortNotFoundPendingRequests();

  std::set<RequestURLAndMethod> request_set_;
  std::map<RequestURLAndMethod, std::unique_ptr<ResponseHandler>>
      response_map_;
  std::map<RequestURLAndMethod, PendingCallbacks> pending_request_map_;

  State state_ = kInitial;
  mojo::Binding<WebPackageSubresourceManager> binding_;

  base::WeakPtrFactory<WebPackageSubresourceManagerImpl> weak_factory_;
};

}  // namespace content

#endif  // CONTENT_RENDERER_LOADER_WEBPACKAGE_SUBRESOURCE_MANAGER_IMPL_H_
