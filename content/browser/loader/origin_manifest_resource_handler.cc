// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/loader/origin_manifest_resource_handler.h"

#include "content/browser/loader/resource_controller.h"
#include "content/browser/origin_manifest/origin_manifest.h"
#include "content/browser/origin_manifest/origin_manifest_storage.h"
#include "content/public/common/resource_response.h"
#include "net/url_request/redirect_info.h"
#include "net/url_request/url_fetcher.h"
#include "net/url_request/url_request_context.h"
#include "net/url_request/url_request_context_getter.h"

namespace content {

class OriginManifestResourceHandler::OriginManifestFetcherDelegate
    : public net::URLFetcherDelegate {
 public:
  OriginManifestFetcherDelegate(OriginManifestResourceHandler* omrh)
      : parent(omrh) {}

  ~OriginManifestFetcherDelegate() override {}
  bool MaybeFetchOriginManifest(ResourceResponse* response) {
    std::string header_value;
    if (!(response->head.headers &&
          response->head.headers->EnumerateHeader(
              nullptr, "Sec-Origin-Manifest", &header_value)))
      return false;

    origin = parent->request()->url().GetOrigin().spec();

    // revoke the current manifest for the origin if requested
    if (header_value == "0") {
      OriginManifestStorage::Revoke(origin);
      return false;
    }

    // if we don't have the manifest yet, fetch it. Otherwise apply it.
    manifest_version = header_value.substr(1, header_value.length() - 2);
    if (OriginManifestStorage::HasManifest(origin, manifest_version)) {
      ApplyManifest(response);
      return false;
    }

    url_fetcher_ = net::URLFetcher::Create(
        GURL(origin + "originmanifest/" + manifest_version + ".json"),
        net::URLFetcher::GET, this);
    net::URLRequestContext* newContext = new net::URLRequestContext();
    newContext->CopyFrom(parent->request()->context());
    url_fetcher_->SetRequestContext(new net::TrivialURLRequestContextGetter(
        newContext, base::ThreadTaskRunnerHandle::Get()));
    url_fetcher_->Start();
    return true;
  }

 protected:
  virtual void ApplyManifest(ResourceResponse* response) = 0;

  std::string manifest_version;
  std::string origin;
  OriginManifestResourceHandler* parent;

  std::unique_ptr<net::URLFetcher> url_fetcher_;
};

class OriginManifestResourceHandler::OriginManifestFetcherDelegateOnRedirect
    : public OriginManifestResourceHandler::OriginManifestFetcherDelegate {
 public:
  OriginManifestFetcherDelegateOnRedirect(OriginManifestResourceHandler* omrh)
      : OriginManifestFetcherDelegate(omrh) {}

 private:
  void ApplyManifest(ResourceResponse* response) override {
    OriginManifestStorage::GetManifest(origin, manifest_version)
        ->ApplyToRedirect(response->head.headers.get());
  }

  void OnURLFetchComplete(const net::URLFetcher* source) override {
    std::unique_ptr<OriginManifest> origin_manifest = nullptr;
    std::string response_str;
    if (source->GetResponseAsString(&response_str)) {
      // parse manifest file from JSON
      origin_manifest = OriginManifest::CreateOriginManifest(response_str);
    }

    if (origin_manifest != nullptr) {
      // Apply the manifest and store it
      origin_manifest->ApplyToRedirect(parent->response_->head.headers.get());
      OriginManifestStorage::Add(origin, manifest_version,
                                 std::move(origin_manifest));
    }

    // continue with next handler
    parent->next_handler_->OnRequestRedirected(*(parent->redirect_info_),
                                               parent->response_.get(),
                                               parent->ReleaseController());
    parent->response_ = nullptr;
    parent->redirect_info_ = nullptr;
  }
};

class OriginManifestResourceHandler::OriginManifestFetcherDelegateOnResponse
    : public OriginManifestResourceHandler::OriginManifestFetcherDelegate {
 public:
  OriginManifestFetcherDelegateOnResponse(OriginManifestResourceHandler* omrh)
      : OriginManifestFetcherDelegate(omrh) {}

 private:
  void ApplyManifest(ResourceResponse* response) override {
    OriginManifestStorage::GetManifest(origin, manifest_version)
        ->ApplyToResponse(response->head.headers.get());
  }

  void OnURLFetchComplete(const net::URLFetcher* source) override {
    std::unique_ptr<OriginManifest> origin_manifest = nullptr;
    std::string response_str;
    if (source->GetResponseAsString(&response_str)) {
      // parse manifest file from JSON
      origin_manifest = OriginManifest::CreateOriginManifest(response_str);
    }

    if (origin_manifest != nullptr) {
      // Apply the manifest and store it
      origin_manifest->ApplyToResponse(parent->response_->head.headers.get());
      OriginManifestStorage::Add(origin, manifest_version,
                                 std::move(origin_manifest));
    }

    // continue with next handler
    parent->next_handler_->OnResponseStarted(parent->response_.get(),
                                             parent->ReleaseController());
    parent->response_ = nullptr;
  }
};

OriginManifestResourceHandler::OriginManifestResourceHandler(
    net::URLRequest* request,
    std::unique_ptr<ResourceHandler> next_handler)
    : LayeredResourceHandler(request, std::move(next_handler)) {}

OriginManifestResourceHandler::~OriginManifestResourceHandler() {}

void OriginManifestResourceHandler::OnWillStart(
    const GURL& url,
    std::unique_ptr<ResourceController> controller) {
  // Tack an 'Sec-Origin-Manifest' header to outgoing requests, as described
  // in https://wicg.github.io/origin-policy/ with the difference to use the
  // (hopefully) future name Origin Manifest already
  std::string origin = url.GetOrigin().spec();
  std::string origin_manifest_version =
      OriginManifestStorage::GetVersion(origin);
  request()->SetExtraRequestHeaderByName("Sec-Origin-Manifest",
                                         origin_manifest_version, true);

  const OriginManifest* manifest =
      OriginManifestStorage::GetManifest(origin, origin_manifest_version);
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

  redirect_info_ = base::MakeUnique<net::RedirectInfo>(redirect_info);
  response_ = response;
  HoldController(std::move(controller));

  origin_manifest_delegate_.reset(
      new OriginManifestFetcherDelegateOnRedirect(this));
  bool is_fetching_origin_manifest =
      origin_manifest_delegate_->MaybeFetchOriginManifest(response);

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

  response_ = response;
  HoldController(std::move(controller));

  origin_manifest_delegate_.reset(
      new OriginManifestFetcherDelegateOnResponse(this));
  bool is_fetching_origin_manifest =
      origin_manifest_delegate_->MaybeFetchOriginManifest(response);

  if (!is_fetching_origin_manifest) {
    next_handler_->OnResponseStarted(response_.get(), ReleaseController());
    response_ = nullptr;
  }
}

}  // namespace content
