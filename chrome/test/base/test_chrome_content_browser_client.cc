// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/test/base/test_chrome_content_browser_client.h"

TestChromeContentBrowserClient::TestChromeContentBrowserClient() {}

TestChromeContentBrowserClient::~TestChromeContentBrowserClient() {}

content::mojom::NetworkContextPtr
TestChromeContentBrowserClient::CreateNetworkContext(
    content::BrowserContext* context,
    bool in_memory,
    const base::FilePath& relative_partition_path) {
  return content::ContentBrowserClient::CreateNetworkContext(
      context, in_memory, relative_partition_path);
}
