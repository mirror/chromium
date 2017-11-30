// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/loader/webpackage_subresource_manager_impl.h"

namespace content {

WebPackageSubresourceManagerImpl::WebPackageSubresourceManagerImpl(
    scoped_refptr<WebPackageReaderAdapter> reader) :
    reader_(std::move(reader)), weak_factory_(this) {
}

WebPackageSubresourceManagerImpl::~WebPackageSubresourceManagerImpl() {
}

void WebPackageSubresourceManagerImpl::OnResponse(
    mojom::WebPackageRequestPtr request,
    mojom::WebPackageResponsePtr response) {
  NOTIMPLEMENTED();
}

void WebPackageSubresourceManagerImpl::OnResponse(
    const GURL& request_url,
    const std::string& method,
    const ResourceResponseHead& response_head,
    mojo::ScopedDataPipeConsumerHandle body,
    CompletionCallback* callback) {
  NOTIMPLEMENTED();
}

}  // namespace content
