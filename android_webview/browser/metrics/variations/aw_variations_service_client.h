// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ANDROID_WEBVIEW_BROWSER_METRICS_VARIATIONS_AW_VARIATIONS_SERVICE_CLIENT_H_
#define ANDROID_WEBVIEW_BROWSER_METRICS_VARIATIONS_AW_VARIATIONS_SERVICE_CLIENT_H_

#include <string>

#include "base/macros.h"
#include "components/variations/service/variations_service_client.h"

// AwVariationsServiceClient provides an implementation of
// VariationsServiceClient, all members are currently stubs for WebView.
class AwVariationsServiceClient : public variations::VariationsServiceClient {
 public:
  AwVariationsServiceClient();
  ~AwVariationsServiceClient() override;

  std::string GetApplicationLocale() override;
  base::Callback<base::Version(void)> GetVersionForSimulationCallback()
      override;
  net::URLRequestContextGetter* GetURLRequestContext() override;
  network_time::NetworkTimeTracker* GetNetworkTimeTracker() override;
  version_info::Channel GetChannel() override;
  bool OverridesRestrictParameter(std::string* parameter) override;
  static std::unique_ptr<AwVariationsServiceClient> Create();

  DISALLOW_COPY_AND_ASSIGN(AwVariationsServiceClient);
};

#endif  // ANDROID_WEBVIEW_BROWSER_METRICS_VARIATIONS_AW_VARIATIONS_SERVICE_CLIENT_H_
