// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/loader/webpackage_reader.h"

#include "base/bind.h"
#include "base/location.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/time/time.h"
#include "content/public/common/resource_request.h"

namespace content {

namespace {

// Dummy page:
//   - http://www.example.com/
//   - index.html
//   - logo.png

WebPackageManifest GetManifest() {
  WebPackageManifest manifest;
  manifest.start_url = GURL("http://www.example.com/");
  return manifest;
}

void CallOnFoundDummySubresource(base::WeakPtr<WebPackageReader> reader) {
  if (!reader || !reader->client())
    return;
  ResourceRequest request;
  request.method = "GET";
  request.url = GURL("http://www.example.com/logo.png");
  reader->client()->OnFoundResource(request);
}

}  // namespace

WebPackageManifest::WebPackageManifest() = default;
WebPackageManifest::~WebPackageManifest() = default;

WebPackageReader::WebPackageReader(
    WebPackageReaderClient* client,
    const GURL& package_url,
    scoped_refptr<URLLoaderFactoryGetter> loader_factory_getter)
    : client_(client),
      package_base_url_(package_url),
      loader_factory_getter_(loader_factory_getter),
      weak_factory_(this) {}

WebPackageReader::~WebPackageReader() = default;

void WebPackageReader::Consume(
    mojo::ScopedDataPipeConsumerHandle* body /* not used for now */) {
  DCHECK(client_);

  // Callback the client with the dummy manifest.
  base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE, base::BindOnce(&WebPackageReader::CallOnFoundManifest,
                                weak_factory_.GetWeakPtr()));
  // XXX This should be also called for the main resource.
  // (For this very initial dummy setting the package data ==
  // main resource)
  base::ThreadTaskRunnerHandle::Get()->PostDelayedTask(
      FROM_HERE,
      base::BindOnce(&CallOnFoundDummySubresource, weak_factory_.GetWeakPtr()),
      base::TimeDelta::FromMicroseconds(100));
}

bool WebPackageReader::CreateResourceLoader(
    mojom::URLLoaderRequest* loader_request,
    int32_t routing_id,
    int32_t request_id,
    uint32_t options,
    const ResourceRequest& resource_request,
    mojom::URLLoaderClientPtr* loader_client,
    const net::MutableNetworkTrafficAnnotationTag& traffic_annotation) {
  // XXX Works only for /logo.png subresource request, and assumes that
  // it is served under the dummy package top URL.
  if (resource_request.url.path() != "/logo.png")
    return false;
  ResourceRequest revised_request(resource_request);
  revised_request.url =
      package_base_url_.Resolve(resource_request.url.path().substr(1));
  loader_factory_getter_->GetNetworkFactory()->get()->CreateLoaderAndStart(
      std::move(*loader_request), routing_id, request_id, options,
      revised_request, std::move(*loader_client), traffic_annotation);
  return true;
}

void WebPackageReader::CallOnFoundManifest() {
  WebPackageManifest manifest = GetManifest();
  DCHECK(client_);
  client_->OnFoundManifest(manifest);
}

}  // namespace content
