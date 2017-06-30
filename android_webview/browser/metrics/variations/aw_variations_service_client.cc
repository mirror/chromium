// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "android_webview/browser/metrics/variations/aw_variations_service_client.h"

#include "base/bind.h"
#include "build/build_config.h"
#include "components/version_info/version_info.h"

namespace {

// TODO(kmilka):  actually implement this class

// Gets the version number to use for variations seed simulation. Must be called
// on a thread where IO is allowed.
base::Version GetVersionForSimulation() {
  return base::Version(version_info::GetVersionNumber());
}

}  // namespace

AwVariationsServiceClient::AwVariationsServiceClient() {}

AwVariationsServiceClient::~AwVariationsServiceClient() {}

std::string AwVariationsServiceClient::GetApplicationLocale() {
  return "";
}

base::Callback<base::Version(void)>
AwVariationsServiceClient::GetVersionForSimulationCallback() {
  return base::Bind(&GetVersionForSimulation);
}

net::URLRequestContextGetter*
AwVariationsServiceClient::GetURLRequestContext() {
  return nullptr;
}

network_time::NetworkTimeTracker*
AwVariationsServiceClient::GetNetworkTimeTracker() {
  return nullptr;
}

version_info::Channel AwVariationsServiceClient::GetChannel() {
  return version_info::Channel::DEV;
}

bool AwVariationsServiceClient::OverridesRestrictParameter(
    std::string* parameter) {
  return false;
}

std::unique_ptr<AwVariationsServiceClient> AwVariationsServiceClient::Create() {
  return std::unique_ptr<AwVariationsServiceClient>(
      new AwVariationsServiceClient());
}
