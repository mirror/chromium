// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/origin_manifest/origin_manifest_url_loader_throttle.h"

#include "base/logging.h"
#include "base/strings/string_util.h"
#include "content/public/common/resource_request.h"
#include "content/public/common/resource_response.h"
#include "content/public/common/simple_url_loader.h"
#include "content/public/network/network_service.h"
#include "net/http/http_request_headers.h"
#include "net/url_request/url_request_context_getter.h"
#include "url/origin.h"

namespace {

bool SanitizeOriginManifestVersion(std::string version, std::string* result) {
  if (!base::IsStringASCII(version))
    return false;

  base::TrimString(version, "\"", &version);

  // TODO(dhausknecht) is there a max length?
  if (version.length() == 0)
    return false;

  *result = version;
  return true;
}

}  // namespace

// static
std::unique_ptr<OriginManifestURLLoaderThrottle>
OriginManifestURLLoaderThrottle::Create() {
  return std::unique_ptr<OriginManifestURLLoaderThrottle>(
      new OriginManifestURLLoaderThrottle());
}

OriginManifestURLLoaderThrottle::OriginManifestURLLoaderThrottle() {}

OriginManifestURLLoaderThrottle::~OriginManifestURLLoaderThrottle() {}

void OriginManifestURLLoaderThrottle::WillStartRequest(
    const content::ResourceRequest& request,
    bool* defer) {
  *defer = false;
}

void OriginManifestURLLoaderThrottle::WillRedirectRequest(
    const net::RedirectInfo& redirect_info,
    bool* defer) {
  *defer = false;
}

void OriginManifestURLLoaderThrottle::WillProcessResponse(
    const GURL& response_url,
    const content::ResourceResponseHead& response_head,
    bool* defer) {
  std::string version;
  if (!response_head.headers.get() &&
      !response_head.headers->EnumerateHeader(nullptr, "Sec-Origin-Manifest",
                                              &version) &&
      !SanitizeOriginManifestVersion(version, &version)) {
    *defer = false;
    return;
  }

  // TODO(dhausknecht) this way of fetching works for now but seems
  // non-optimal. I am sure there is a better way.

  // Build the ResourceRequest
  std::string path = std::string(".well-known/originmanifest/")
                         .append(version)
                         .append(".json");
  url::Replacements<char> replacements;
  replacements.SetPath(path.c_str(), url::Component(0, path.length()));
  content::ResourceRequest res_request;
  res_request.url =
      url::Origin(response_url).GetURL().ReplaceComponents(replacements);

  // Get the URLLoaderFactory
  network_service_ = content::NetworkService::Create();
  content::mojom::NetworkContextParamsPtr context_params =
      content::mojom::NetworkContextParams::New();
  context_params->http_cache_enabled = false;
  network_service_->CreateNetworkContext(mojo::MakeRequest(&network_context_),
                                         std::move(context_params));
  // TODO(dhausknecht) 0 for the process ID is probably not what we want
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

  url_loader_ = content::SimpleURLLoader::Create();
  url_loader_->DownloadToString(
      res_request, url_loader_factory_.get(), traffic_annotation,
      base::BindOnce(&OriginManifestURLLoaderThrottle::OnBodyAsString,
                     base::Unretained(this)),
      url_loader_->kMaxBoundedStringDownloadSize);

  *defer = true;
}

void OriginManifestURLLoaderThrottle::OnBodyAsString(
    std::unique_ptr<std::string> response_body) {
  VLOG(1) << "Origin Manifest: " << response_body.get()->c_str();
}
