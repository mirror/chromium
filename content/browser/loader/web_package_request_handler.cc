// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/loader/web_package_request_handler.h"

#include "base/bind.h"
#include "content/browser/loader/web_package_loader.h"
#include "content/browser/loader/web_package_url_loader.h"
#include "content/common/throttling_url_loader.h"
#include "mojo/public/cpp/bindings/strong_binding.h"
#include "net/http/http_response_headers.h"
#include "services/network/public/cpp/resource_response.h"
#include "services/network/public/interfaces/url_loader.mojom.h"

namespace content {

WebPackageRequestHandler::WebPackageRequestHandler() : weak_factory_(this) {}

WebPackageRequestHandler::~WebPackageRequestHandler() {}

void WebPackageRequestHandler::MaybeCreateLoader(
    const network::ResourceRequest& resource_request,
    ResourceContext* resource_context,
    LoaderCallback callback) {
  // TODO(horo): Ask WebPackageFetchManager to return the ongoing matching
  // SignedExchangeHandler.

  if (!web_package_loader_) {
    std::move(callback).Run(StartLoaderCallback());
    return;
  }

  std::move(callback).Run(base::BindOnce(
      &WebPackageRequestHandler::StartResponse, weak_factory_.GetWeakPtr()));
}

bool WebPackageRequestHandler::MaybeCreateLoaderForResponse(
    const network::ResourceResponseHead& response,
    network::mojom::URLLoaderPtr* loader,
    network::mojom::URLLoaderClientRequest* client_request,
    ThrottlingURLLoader* url_loader) {
  if (response.was_fetched_via_service_worker)
    return false;
  if (!response.headers)
    return false;
  std::string mime_type;
  std::string new_url_string;
  if (!response.headers->GetMimeType(&mime_type))
    return false;
  if (mime_type != "application/http-exchange+cbor")
    return false;

  network::mojom::URLLoaderClientPtr client;
  *client_request = mojo::MakeRequest(&client);

  web_package_loader_ = base::MakeUnique<WebPackageLoader>(
      response, std::move(client), url_loader->Unbind());

  return true;
}

void WebPackageRequestHandler::StartResponse(
    network::mojom::URLLoaderRequest request,
    network::mojom::URLLoaderClientPtr client) {
  web_package_loader_->ConnectToClient(std::move(client));
  mojo::MakeStrongBinding(
      base::MakeUnique<WebPackageURLLoader>(std::move(web_package_loader_)),
      std::move(request));
}

}  // namespace content
