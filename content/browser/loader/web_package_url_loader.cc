// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/loader/web_package_url_loader.h"

#include "content/browser/loader/web_package_loader.h"

namespace content {

WebPackageURLLoader::WebPackageURLLoader(
    std::unique_ptr<WebPackageLoader> web_package_loader)
    : web_package_loader_(std::move(web_package_loader)) {}

WebPackageURLLoader::~WebPackageURLLoader() {}

}  // namespace content
