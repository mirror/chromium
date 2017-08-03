// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_TEST_BASE_TEST_CHROME_CONTENT_BROWSER_CLIENT_H_
#define CHROME_TEST_BASE_TEST_CHROME_CONTENT_BROWSER_CLIENT_H_

#include "base/macros.h"
#include "chrome/browser/chrome_content_browser_client.h"
#include "content/public/browser/content_browser_client.h"
#include "content/public/common/network_service.mojom.h"

namespace base {
class FilePath;
}

class BrowserContext;

// Subclass of ChromeContentBrowserClient for unit tests, where a TestProfile is
// used. Unittests using the TestProfile violate some fundamental properties of
// the BrowserContextKeyedService / Profile / ProfileIOData relationships. In
// particular, CreateNetworkContext is normally called before
// Profile::CreateRequestContext, and there's normally one StoragePartition per
// BrowserContextKeyedService.
//
// TODO(mmenke):  Figure how to get rid of this.
class TestChromeContentBrowserClient : public ChromeContentBrowserClient {
 public:
  TestChromeContentBrowserClient();
  ~TestChromeContentBrowserClient() override;

  // ChromeContentBrowserClient::CreateNetworkContext does not work with a
  // TestProfile.
  content::mojom::NetworkContextPtr CreateNetworkContext(
      content::BrowserContext* context,
      bool in_memory,
      const base::FilePath& relative_partition_path) override;

 private:
  DISALLOW_COPY_AND_ASSIGN(TestChromeContentBrowserClient);
};

#endif  // CHROME_TEST_BASE_TEST_CHROME_CONTENT_BROWSER_CLIENT_H_
