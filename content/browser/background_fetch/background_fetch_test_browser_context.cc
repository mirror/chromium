// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/background_fetch/background_fetch_test_browser_context.h"

namespace content {

BackgroundFetchTestBrowserContext::BackgroundFetchTestBrowserContext() {}

BackgroundFetchTestBrowserContext::~BackgroundFetchTestBrowserContext() {}

MockBackgroundFetchDelegate*
BackgroundFetchTestBrowserContext::GetBackgroundFetchDelegate() {
  if (!delegate_)
    delegate_.reset(new MockBackgroundFetchDelegate());

  return delegate_.get();
}

}  // namespace content
