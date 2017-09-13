// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform/testing/ScopedMockedURL.h"

#include "platform/testing/URLTestHelpers.h"
#include "public/platform/WebURLLoaderMockFactory.h"

namespace blink {
namespace testing {

ScopedMockedURL::ScopedMockedURL(const WebURL& url,
                                 WebURLLoaderMockFactory* loader_factory)
    : url_(url), loader_factory_(loader_factory) {}

ScopedMockedURL::~ScopedMockedURL() {
  loader_factory_->UnregisterURL(url_);
}

ScopedMockedURLLoad::ScopedMockedURLLoad(
    const WebURL& full_url,
    const WebString& file_path,
    WebURLLoaderMockFactory* loader_factory,
    const WebString& mime_type)
    : ScopedMockedURL(full_url, loader_factory) {
  URLTestHelpers::RegisterMockedURLLoad(full_url, file_path, loader_factory,
                                        mime_type);
}

}  // namespace testing
}  // namespace blink
