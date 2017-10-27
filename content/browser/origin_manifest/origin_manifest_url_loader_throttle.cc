// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/origin_manifest/origin_manifest_url_loader_throttle.h"

#include "base/strings/string_util.h"
#include "content/public/common/content_features.h"
#include "content/public/common/resource_request.h"
#include "content/public/common/resource_response.h"
#include "content/public/common/simple_url_loader.h"
#include "content/public/network/network_service.h"
#include "net/base/mime_util.h"
#include "net/http/http_request_headers.h"
#include "url/origin.h"

namespace {

bool SanitizeOriginManifestVersion(std::string version, std::string* result) {
  if (!base::IsStringASCII(version))
    return false;

  base::TrimString(version, "\"", &version);

  // TODO(dhausknecht) is there a max length? At least not by the HTTP sepc
  if (version.length() == 0)
    return false;

  for (char& c : version) {
    if (!base::IsAsciiAlpha(c) && !base::IsAsciiDigit(c))
      return false;
  }

  *result = version;
  return true;
}

bool ExtractManifestVersion(const content::ResourceResponseHead& response_head,
                            std::string* result) {
  DCHECK(response_head.headers.get());

  std::string version;
  if (!response_head.headers->EnumerateHeader(nullptr, "Sec-Origin-Manifest",
                                              &version))
    return false;

  return SanitizeOriginManifestVersion(version, result);
}

}  // namespace

// static
std::unique_ptr<OriginManifestURLLoaderThrottle>
OriginManifestURLLoaderThrottle::Create() {
  if (base::FeatureList::IsEnabled(features::kOriginManifest)) {
    return std::unique_ptr<OriginManifestURLLoaderThrottle>(
        new OriginManifestURLLoaderThrottle());
  }
  return nullptr;
}

OriginManifestURLLoaderThrottle::OriginManifestURLLoaderThrottle() {}

OriginManifestURLLoaderThrottle::~OriginManifestURLLoaderThrottle() {}

void OriginManifestURLLoaderThrottle::WillProcessResponse(
    const GURL& response_url,
    const content::ResourceResponseHead& response_head,
    bool* defer) {
  DCHECK(response_head.headers.get());

  std::string version;
  if (!ExtractManifestVersion(response_head, &version)) {
    *defer = false;
    return;
  }

  // At this point we are guaranteed to have a non-empty string in |version|
  if (version == "0") {
    // TODO(dhausknecht): Clear the stored manifest once we're storing
    // manifests.
    // TODO(dhausknecht) we might want to defer in future to make sure there are
    // no race conditions between "time of deletion" and
    // "time of use still stored manifest"
    *defer = false;
    return;
  }

  // TODO(dhausknecht) Check if we already have the current version. If not
  // continue, otherwise we are done here

  // Build the ResourceRequest
  std::string path = std::string("/.well-known/originmanifest/")
                         .append(version)
                         .append(".json");
  url::Replacements<char> replacements;
  replacements.SetPath(path.c_str(), url::Component(0, path.length()));
  content::ResourceRequest res_request;
  res_request.url = url::Origin::Create(response_url)
                        .GetURL()
                        .ReplaceComponents(replacements);

  // Get the URLLoaderFactory
  network_service_ = content::NetworkService::Create();
  content::mojom::NetworkContextParamsPtr context_params =
      content::mojom::NetworkContextParams::New();
  context_params->http_cache_enabled = false;
  network_service_->CreateNetworkContext(mojo::MakeRequest(&network_context_),
                                         std::move(context_params));
  network_context_->CreateURLLoaderFactory(
      mojo::MakeRequest(&url_loader_factory_), 0);

  // Create the traffic annotation
  net::NetworkTrafficAnnotationTag traffic_annotation =
      net::DefineNetworkTrafficAnnotation("origin_manifest_loader", R"(
        semantics {
          sender: "Origin Manifest URL Loader Throttle"
          description:
            "Fetches the Origin Manifest with a given version from an origin."
          trigger:
            "In case the Origin Manifest with a given version does not "
            "exist in the Origin Manifest Store, it is fetched from the "
            "origin at a well-known location."
          data:
            "None, the URL itself contains the origin and Origin Manifest "
            "version"
          destination: OTHER
        }
        policy {
          cookies_allowed: NO
          setting: "This feature cannot be disabled by settings. Server "
            "opt-in or out of this mechanism."
          policy_exception_justification:
            "Not implemented, considered not useful."
        })");

  // make the actual manifest download
  url_loader_ = content::SimpleURLLoader::Create();
  url_loader_->SetAllowHttpErrorResults(false);
  // TODO(dhausknecht) Add the following line as soon as patch 741744 has landed
  // url_loader_->SetAllowRedirects(false);
  url_loader_->DownloadToString(
      res_request, url_loader_factory_.get(), traffic_annotation,
      base::BindOnce(&OriginManifestURLLoaderThrottle::OnDownloadFinished,
                     base::Unretained(this)),
      url_loader_->kMaxBoundedStringDownloadSize);

  *defer = true;
}

void OriginManifestURLLoaderThrottle::OnDownloadFinished(
    std::unique_ptr<std::string> response_body) {
  if (url_loader_->NetError()) {
    delegate_->Resume();
    return;
  }

  // MIME type check
  if (!net::MatchesMimeType("application/json",
                            url_loader_->ResponseInfo()->mime_type)) {
    delegate_->Resume();
    return;
  }

  // TODO(dhausknecht) parse the string to a Origin Manifest and store it

  // continues the deferred initial response
  delegate_->Resume();
}
