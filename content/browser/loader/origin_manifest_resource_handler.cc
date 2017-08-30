// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/loader/origin_manifest_resource_handler.h"

#include "base/strings/string_util.h"
#include "content/browser/loader/resource_controller.h"
#include "content/browser/origin_manifest/origin_manifest.h"
#include "content/browser/origin_manifest/origin_manifest_store.h"
#include "content/public/common/resource_response.h"
#include "net/url_request/redirect_info.h"
#include "net/url_request/url_fetcher.h"
#include "net/url_request/url_request_context.h"
#include "net/url_request/url_request_context_getter.h"

namespace content {

OriginManifestResourceHandler::OriginManifestResourceHandler(
    net::URLRequest* request,
    std::unique_ptr<ResourceHandler> next_handler)
    : LayeredResourceHandler(request, std::move(next_handler)),
      store_(OriginManifestStore::Get()) {}

OriginManifestResourceHandler::~OriginManifestResourceHandler() {}

void OriginManifestResourceHandler::OnWillStart(
    const GURL& url,
    std::unique_ptr<ResourceController> controller) {
  // Tack an 'Sec-Origin-Manifest' header to outgoing requests, as described
  // in https://wicg.github.io/origin-policy/ with the difference to use the
  // (hopefully) future name Origin Manifest already
  std::string origin = url.GetOrigin().spec();
  std::string origin_manifest_version = store_.GetVersion(origin);
  request()->SetExtraRequestHeaderByName("Sec-Origin-Manifest",
                                         origin_manifest_version, true);

  const OriginManifest* manifest =
      store_.GetManifest(origin, origin_manifest_version);
  if (manifest) {
    net::HttpRequestHeaders extra_headers(request()->extra_request_headers());
    manifest->ApplyToRequest(&extra_headers);
    request()->SetExtraRequestHeaders(extra_headers);
  }

  DCHECK(next_handler_.get());
  next_handler_->OnWillStart(url, std::move(controller));
}

void OriginManifestResourceHandler::OnRequestRedirected(
    const net::RedirectInfo& redirect_info,
    ResourceResponse* response,
    std::unique_ptr<ResourceController> controller) {
  DCHECK(next_handler_.get());
  DCHECK(response);

  load_event = ON_REDIRECT;
  redirect_info_ = base::MakeUnique<net::RedirectInfo>(redirect_info);
  response_ = response;
  HoldController(std::move(controller));

  bool is_fetching_origin_manifest =
      MaybeFetchOriginManifest(request()->response_info().headers.get());

  if (!is_fetching_origin_manifest) {
    next_handler_->OnRequestRedirected(*redirect_info_, response_.get(),
                                       ReleaseController());
    response_ = nullptr;
    redirect_info_ = nullptr;
  }
}

void OriginManifestResourceHandler::OnResponseStarted(
    ResourceResponse* response,
    std::unique_ptr<ResourceController> controller) {
  DCHECK(next_handler_.get());
  DCHECK(response);

  load_event = ON_RESPONSE;
  response_ = response;
  HoldController(std::move(controller));

  bool is_fetching_origin_manifest =
      MaybeFetchOriginManifest(request()->response_info().headers.get());

  if (!is_fetching_origin_manifest) {
    next_handler_->OnResponseStarted(response_.get(), ReleaseController());
    response_ = nullptr;
  }
}

bool OriginManifestResourceHandler::MaybeFetchOriginManifest(
    net::HttpResponseHeaders* headers) {
  // TODO We ignore the response header if it is "Not Trustworthy"
  // if (!IsOriginSecure(request()->url()))
  //  return false;

  std::string header_value;
  if (!(headers && headers->EnumerateHeader(nullptr, "Sec-Origin-Manifest",
                                            &header_value)))
    return false;

  origin = request()->url().GetOrigin().spec();
  // revoke the current manifest for the origin if requested
  if (header_value == "0") {
    store_.Revoke(origin);
    return false;
  }

  // remove quotation marks when present
  if (base::StartsWith(header_value, "\"", base::CompareCase::SENSITIVE) &&
      base::EndsWith(header_value, "\"", base::CompareCase::SENSITIVE))
    manifest_version = header_value.substr(1, header_value.length() - 2);
  else
    manifest_version = header_value;

  // if we don't have the manifest yet, fetch it. Otherwise apply it.
  if (store_.HasManifest(origin, manifest_version)) {
    const OriginManifest* origin_manifest =
        store_.GetManifest(origin, manifest_version);
    ApplyManifest(origin_manifest, headers);
    return false;
  }

  url_fetcher_ = net::URLFetcher::Create(
      GURL(origin + "originmanifest/" + manifest_version + ".json"),
      net::URLFetcher::GET, this);
  net::URLRequestContext* newContext = new net::URLRequestContext();
  newContext->CopyFrom(request()->context());
  url_fetcher_->SetRequestContext(new net::TrivialURLRequestContextGetter(
      newContext, base::ThreadTaskRunnerHandle::Get()));
  url_fetcher_->Start();
  return true;
}

void OriginManifestResourceHandler::ApplyManifest(
    const OriginManifest* origin_manifest,
    net::HttpResponseHeaders* headers) {
  DCHECK(origin_manifest != nullptr);

  switch (load_event) {
    case UNKNOWN:
      break;
    case ON_REDIRECT:
      origin_manifest->ApplyToRedirect(headers);
      break;
    case ON_RESPONSE:
      origin_manifest->ApplyToResponse(headers);
      break;
  }
}

void OriginManifestResourceHandler::OnURLFetchComplete(
    const net::URLFetcher* source) {
  std::unique_ptr<OriginManifest> origin_manifest = nullptr;
  std::string response_str;
  if (source->GetResponseAsString(&response_str)) {
    // parse manifest file from JSON
    origin_manifest = OriginManifest::CreateOriginManifest(
        origin, manifest_version, response_str);
  }

  if (origin_manifest != nullptr) {
    // Apply the manifest and store it
    ApplyManifest(origin_manifest.get(),
                  request()->response_info().headers.get());
    store_.Add(origin, manifest_version, std::move(origin_manifest));
  }

  origin = "boobs hihihi";

  // continue with next handler
  switch (load_event) {
    case UNKNOWN:
      break;
    case ON_REDIRECT:
      next_handler_->OnRequestRedirected(*(redirect_info_), response_.get(),
                                         ReleaseController());
      response_ = nullptr;
      redirect_info_ = nullptr;
      break;
    case ON_RESPONSE:
      next_handler_->OnResponseStarted(response_.get(), ReleaseController());
      response_ = nullptr;
      break;
  }
}

}  // namespace content
