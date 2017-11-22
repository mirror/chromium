// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/loader/webpackage_reader_adapter.h"

#include "base/bind.h"
#include "base/location.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/time/time.h"
#include "content/public/common/resource_request.h"

namespace content {

WebPackageManifest::WebPackageManifest() = default;
WebPackageManifest::~WebPackageManifest() = default;

WebPackageReaderAdapter::WebPackageReaderAdapter(
    WebPackageReaderAdapterClient* client,
    const GURL& package_url,
    scoped_refptr<URLLoaderFactoryGetter> loader_factory_getter)
    : client_(client),
      package_base_url_(package_url),
      loader_factory_getter_(loader_factory_getter),
      reader_(std::make_unique<wpkg::WebPackageReader>(this)),
      weak_factory_(this) {}

WebPackageReaderAdapter::~WebPackageReaderAdapter() = default;

void WebPackageReaderAdapter::Consume(mojo::ScopedDataPipeConsumerHandle body) {
  DCHECK(reader_);
  DCHECK(client_);
  // TODO(kinuko): implement.
}

bool WebPackageReaderAdapter::GetResource(
    const ResourceRequest& resource_request,
    ResourceCallback callback) {
  // TODO(kinuko): implement.
  return false;
}

}  // namespace content
