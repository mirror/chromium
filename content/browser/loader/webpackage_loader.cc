// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/loader/webpackage_loader.h"

#include "content/browser/url_loader_factory_getter.h"

namespace content {

WebPackageRequestHandler::WebPackageRequestHandler(
    const ResourceRequest& resource_request,
    const scoped_refptr<URLLoaderFactoryGetter>&
    default_url_loader_factory_getter)
    : resource_request_(resource_request),
      default_url_loader_factory_getter_(default_url_loader_factory_getter) {
}

WebPackageRequestHandler::~WebPackageRequestHandler() = default;

void WebPackageRequestHandler::MaybeCreateLoader(
    const ResourceRequest& resource_request,
    ResourceContext* resource_context,
    LoaderCallback callback) {
  // XXX dirty handling.
  base::FilePath::StringType extension =
    base::FilePath(resource_request.url.PathForRequest()).Extension();
  if (extension != FILE_PATH_LITERAL(".wpkg")) {
    // Shouldn't be a wpkg, let it go.
    std::move(callback).Run(StartLoaderCallback());
  }
}

bool WebPackageRequestHandler::MaybeCreateLoaderForResponse(
    const ResourceResponseHead& response,
    mojom::URLLoaderPtr* loader,
    mojom::URLLoaderClientRequest* client_request) {
  return false;
}

}  // namespace content
