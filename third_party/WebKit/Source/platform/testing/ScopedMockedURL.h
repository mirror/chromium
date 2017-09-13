// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ScopedMockedURL_h
#define ScopedMockedURL_h

#include "platform/testing/TestingPlatformSupport.h"
#include "public/platform/WebString.h"
#include "public/platform/WebURL.h"
#include "public/platform/WebURLResponse.h"

namespace blink {

namespace testing {

// Convenience classes that register a mocked URL on construction, and
// unregister it on destruction. This prevent mocked URL from leaking to other
// tests.
class ScopedMockedURL {
 public:
  explicit ScopedMockedURL(const WebURL&,
                           WebURLLoaderMockFactory* loader_factory);
  virtual ~ScopedMockedURL();

 private:
  WebURL url_;
  WebURLLoaderMockFactory* loader_factory_;
};

class ScopedMockedURLLoad : ScopedMockedURL {
 public:
  ScopedMockedURLLoad(
      const WebURL& full_url,
      const WebString& file_path,
      WebURLLoaderMockFactory* loader_factory,
      const WebString& mime_type = WebString::FromUTF8("text/html"));
  ~ScopedMockedURLLoad() override = default;
};

}  // namespace testing

}  // namespace blink

#endif
