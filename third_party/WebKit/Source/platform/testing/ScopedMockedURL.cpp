// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform/testing/ScopedMockedURL.h"

#include "platform/testing/URLTestHelpers.h"
#include "public/platform/WebURLLoaderMockFactory.h"

namespace blink {
namespace testing {

ScopedMockedURL::ScopedMockedURL(const WebURL& url,
                                 WebURLLoaderMockFactory* platform)
    : url_(url), platform_(platform) {}

ScopedMockedURL::~ScopedMockedURL() {
  platform_->UnregisterURL(url_);
}

ScopedMockedURLLoad::ScopedMockedURLLoad(const WebURL& full_url,
                                         const WebString& file_path,
                                         WebURLLoaderMockFactory* platform,
                                         const WebString& mime_type)
    : ScopedMockedURL(full_url, platform) {
  URLTestHelpers::RegisterMockedURLLoad(full_url, file_path, platform,
                                        mime_type);
}

}  // namespace testing
}  // namespace blink
