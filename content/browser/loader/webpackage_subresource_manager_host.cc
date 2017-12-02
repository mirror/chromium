// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/loader/webpackage_subresource_manager_host.h"

namespace content {

namespace {

class CompletionCallbackWrapper {
 public:
  explicit CompletionCallbackWrapper(
      mojom::WebPackageResponseCompletionCallbackPtr callback)
      : callback_(std::move(callback)) {}

  void OnComplete(const network::URLLoaderCompletionStatus& status) {
    callback_->OnComplete(status);
  }

 private:
  mojom::WebPackageResponseCompletionCallbackPtr callback_;
};

}  // namespace

WebPackageSubresourceManagerHost::WebPackageSubresourceManagerHost(
    scoped_refptr<WebPackageReaderAdapter> reader,
    mojom::WebPackageSubresourceManagerPtr subresource_manager) :
    reader_(std::move(reader)),
    subresource_manager_(std::move(subresource_manager)),
    weak_factory_(this) {
  subresource_manager_.set_connection_error_handler(
      base::Bind(&WebPackageSubresourceManagerHost::OnConnectionError,
                 base::Unretained(this)));
}

WebPackageSubresourceManagerHost::~WebPackageSubresourceManagerHost() {
}

void WebPackageSubresourceManagerHost::OnRequest(
    const GURL& request_url,
    const std::string& method) {
  subresource_manager_->OnRequest(request_url, method);
}

void WebPackageSubresourceManagerHost::OnResponse(
    const GURL& request_url,
    const std::string& method,
    const ResourceResponseHead& response_head,
    mojo::ScopedDataPipeConsumerHandle body,
    CompletionCallback* callback) {
  mojom::WebPackageResponseCompletionCallbackPtr callback_ptr;
  subresource_manager_->OnResponse(request_url, method, response_head,
                                   std::move(body),
                                   mojo::MakeRequest(&callback_ptr));
  *callback = base::BindOnce(
          &CompletionCallbackWrapper::OnComplete,
          std::make_unique<CompletionCallbackWrapper>(
              std::move(callback_ptr)));
}

void WebPackageSubresourceManagerHost::OnConnectionError() {
  delete this;
}

}  // namespace content
