// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/origin_manifest/origin_manifest_fetcher.h"

#include "base/stl_util.h"
#include "base/strings/string_util.h"
#include "content/browser/origin_manifest/origin_manifest_parser.h"
#include "content/public/common/origin_util.h"
#include "net/base/url_util.h"
#include "net/url_request/url_fetcher.h"
#include "net/url_request/url_request_context.h"
#include "net/url_request/url_request_context_getter.h"
#include "url/gurl.h"
#include "url/url_util.h"

namespace content {

OriginManifestFetcher::OriginManifestFetcher(
    const url::Origin origin,
    const std::string version,
    net::URLRequestContextGetter* getter)
    : origin_(origin), version_(version), url_request_context_getter_(getter) {}

OriginManifestFetcher::~OriginManifestFetcher() {}

void OriginManifestFetcher::Fetch(
    base::OnceCallback<void(blink::mojom::OriginManifestPtr)> callback) {
  // Check if origin is Secure Context
  if (!IsOriginSecure(origin_.GetURL())) {
    std::move(callback).Run(nullptr);
    return;
  }

  // Remember things for processing in OnURLFetchComplete
  callback_ = std::move(callback);

  url_fetcher_ = net::URLFetcher::Create(
      GURL(origin_.GetURL().spec() + "originmanifest/" + version_ + ".json"),
      net::URLFetcher::GET, this);
  url_fetcher_->SetRequestContext(url_request_context_getter_);
  url_fetcher_->Start();
}

void OriginManifestFetcher::OnURLFetchComplete(const net::URLFetcher* source) {
  std::string response_str;
  if (source->GetResponseCode() != 200 ||
      !source->GetResponseAsString(&response_str)) {
    std::move(callback_).Run(nullptr);
    return;
  }
  std::move(callback_).Run(
      OriginManifestParser::Parse(origin_, version_, response_str));

  int l = (response_str.size() > 256) ? 256 : response_str.size();
  VLOG(1) << response_str.substr(0, l);

  delete this;
}

const char* OriginManifestFetcher::GetNameForLogging() {
  return "OriginManifestFetcher";
}

}  // namespace content
