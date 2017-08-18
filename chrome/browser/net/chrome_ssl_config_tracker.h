// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_NET_CHROME_SSL_CONFIG_TRACKER_H_
#define CHROME_BROWSER_NET_CHROME_SSL_CONFIG_TRACKER_H_

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "content/public/common/ssl_config.mojom.h"
#include "net/ssl/ssl_config.h"
#include "net/ssl/ssl_config_service.h"

namespace net {
struct SSLConfig;
}

namespace ssl_config {
class SSLConfigServiceManager;
}

// Class that tracks preferences as they apply to the SSLConfig, and sends
// updated configurations to a mojom::SSLConfigClient. Lives on the UI thread.
class ChromeSSLConfigTracker : public net::SSLConfigService::Observer {
 public:
  explicit ChromeSSLConfigTracker(
      content::mojom::SSLConfigClientPtr ssl_config_client);

  ~ChromeSSLConfigTracker() override;

  // net::SSLConfigService::Observer implementation:
  void OnSSLConfigChanged() override;

  net::SSLConfig GetSSLConfig() const;

 private:
  // This is an instance of the default SSLConfigServiceManager for the current
  // platform and it gets SSL preferences from local_state object.
  std::unique_ptr<ssl_config::SSLConfigServiceManager>
      ssl_config_service_manager_;

  const content::mojom::SSLConfigClientPtr ssl_config_client_;

  DISALLOW_COPY_AND_ASSIGN(ChromeSSLConfigTracker);
};

#endif  // CHROME_BROWSER_NET_CHROME_SSL_CONFIG_TRACKER_H_
