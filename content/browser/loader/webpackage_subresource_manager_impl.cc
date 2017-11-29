// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/loader/webpackage_subresource_manager_impl.h"

#include "content/browser/loader/webpackage_reader_adapter.h"

namespace content {

WebPackageSubresourceManagerImpl::WebPackageSubresourceManagerImpl(
    scoped_refptr<WebPackageReaderAdapter> reader) {
}

WebPackageSubresourceManagerImpl::~WebPackageSubresourceManagerImpl() {
}

void WebPackageSubresourceManagerImpl::OnResponse(
    mojom::WebPackageRequestPtr request,
    mojom::WebPackageResponsePtr response) {
}

}  // namespace content
