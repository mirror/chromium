// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_ORIGIN_MANIFEST_ORIGIN_MANIFEST_FETCHER_H_
#define CONTENT_BROWSER_ORIGIN_MANIFEST_ORIGIN_MANIFEST_FETCHER_H_

#include <string>

#include "mojo/public/cpp/bindings/binding.h"
#include "net/url_request/url_fetcher_delegate.h"
#include "third_party/WebKit/public/platform/origin_manifest.mojom.h"

namespace net {
class URLFetcher;
class URLRequestContextGetter;
}  // namespace net

namespace url {
class Origin;
}  // namespace url

namespace content {

class OriginManifestFetcher : public net::URLFetcherDelegate {
 public:
  OriginManifestFetcher(const url::Origin origin,
                        const std::string version,
                        net::URLRequestContextGetter* getter);

  void Fetch(
      base::OnceCallback<void(blink::mojom::OriginManifestPtr)> callback);

  const char* GetNameForLogging();

 private:
  ~OriginManifestFetcher() override;

  void OnURLFetchComplete(const net::URLFetcher* source) override;

  const url::Origin origin_;
  const std::string version_;
  net::URLRequestContextGetter* url_request_context_getter_;

  base::OnceCallback<void(blink::mojom::OriginManifestPtr)> callback_;
  std::unique_ptr<net::URLFetcher> url_fetcher_;

  DISALLOW_COPY_AND_ASSIGN(OriginManifestFetcher);
};

}  // namespace content

#endif  // CONTENT_BROWSER_ORIGIN_MANIFEST_ORIGIN_MANIFEST_FETCHER_H_
