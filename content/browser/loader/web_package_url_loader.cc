// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/loader/web_package_url_loader.h"

#include "base/feature_list.h"
#include "content/browser/loader/web_package_loader.h"
#include "content/public/common/content_features.h"

namespace content {

WebPackageURLLoader::WebPackageURLLoader(
    std::unique_ptr<WebPackageLoader> web_package_loader)
    : web_package_loader_(std::move(web_package_loader)) {
  DCHECK(base::FeatureList::IsEnabled(features::kSignedHTTPExchange));
}

WebPackageURLLoader::~WebPackageURLLoader() {}

void WebPackageURLLoader::FollowRedirect() {
  NOTREACHED();
}

void WebPackageURLLoader::ProceedWithResponse() {
  // TODO(803774): Implement this if we support non-NetworkService mode.
}

void WebPackageURLLoader::SetPriority(net::RequestPriority priority,
                                      int intra_priority_value) {
  // TODO(803774): Implement this.
}

void WebPackageURLLoader::PauseReadingBodyFromNet() {
  // TODO(803774): Implement this.
}

void WebPackageURLLoader::ResumeReadingBodyFromNet() {
  // TODO(803774): Implement this.
}

}  // namespace content
