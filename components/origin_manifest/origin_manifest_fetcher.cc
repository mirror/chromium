// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/origin_manifest/origin_manifest_fetcher.h"

#include "base/stl_util.h"
#include "net/base/url_util.h"
#include "net/url_request/url_fetcher.h"
#include "net/url_request/url_request_context.h"
#include "net/url_request/url_request_context_getter.h"
#include "url/gurl.h"
#include "url/url_util.h"

namespace {

// shamelessly copy-pasted from '//src/content/common/origin_util.cc'. But I
// shouldn't depend on content. That's why.
bool IsOriginSecure(const GURL& url) {
  if (url.SchemeIsCryptographic() || url.SchemeIsFile())
    return true;

  if (url.SchemeIsFileSystem() && url.inner_url() &&
      IsOriginSecure(*url.inner_url())) {
    return true;
  }

  if (net::IsLocalhost(url.HostNoBracketsPiece()))
    return true;

  if (base::ContainsValue(url::GetSecureSchemes(), url.scheme()))
    return true;

  // if (base::ContainsValue(GetSecureOrigins(), url.GetOrigin()))
  //  return true;

  return false;
}

}  // namespace

namespace origin_manifest {

OriginManifestFetcher::OriginManifestFetcher(
    const std::string origin,
    const std::string version,
    net::URLRequestContextGetter* getter)
    : origin_(origin), version_(version), url_request_context_getter_(getter) {}

OriginManifestFetcher::~OriginManifestFetcher() {}

void OriginManifestFetcher::Fetch(
    base::OnceCallback<void(mojom::OriginManifestPtr)> callback) {
  // Check if origin is Secure Context
  VLOG(1) << "Ignoring security for now...";
  /*
    if (!IsOriginSecure(GURL(origin_))) {
      std::move(callback).Run(nullptr);
      return;
    }
  */

  // Remember things for processing in OnURLFetchComplete
  callback_ = std::move(callback);

  url_fetcher_ = net::URLFetcher::Create(
      GURL(origin_ + "originmanifest/" + version_ + ".json"),
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
  // TODO(dhausknecht): remove string shortener. Just to get not too much LOG
  if (response_str.length() > 256)
    response_str = response_str.substr(0, 256);
  std::move(callback_).Run(
      mojom::OriginManifest::New(origin_, version_, response_str));

  delete this;
}

const char* OriginManifestFetcher::GetNameForLogging() {
  return "OriginManifestFetcher";
}

}  // namespace origin_manifest
