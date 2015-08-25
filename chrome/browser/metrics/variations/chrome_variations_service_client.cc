// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/metrics/variations/chrome_variations_service_client.h"

#include "chrome/browser/browser_process.h"

ChromeVariationsServiceClient::ChromeVariationsServiceClient() {}

ChromeVariationsServiceClient::~ChromeVariationsServiceClient() {}

std::string ChromeVariationsServiceClient::GetApplicationLocale() {
  return g_browser_process->GetApplicationLocale();
}

net::URLRequestContextGetter*
ChromeVariationsServiceClient::GetURLRequestContext() {
  return g_browser_process->system_request_context();
}

network_time::NetworkTimeTracker*
ChromeVariationsServiceClient::GetNetworkTimeTracker() {
  return g_browser_process->network_time_tracker();
}

void ChromeVariationsServiceClient::OnInitialStartup() {
#if defined(OS_WIN)
  StartGoogleUpdateRegistrySync();
#endif
}

#if defined(OS_WIN)
void ChromeVariationsServiceClient::StartGoogleUpdateRegistrySync() {
  registry_syncer_.RequestRegistrySync();
}
#endif
