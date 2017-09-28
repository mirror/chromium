// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ssl/captive_portal_util.h"

#include <netlistmgr.h>

#include "base/logging.h"
#include "base/win/scoped_comptr.h"
#include "base/win/scoped_variant.h"
#include "net/base/network_interfaces.h"

namespace {

enum NetworkStatus {
  NETWORK_STATUS_DISCONNECTED = 0,
  NETWORK_STATUS_NOT_BEHIND_CAPTIVE_PORTAL = 1,
  NETWORK_STATUS_BEHIND_CAPTIVE_PORTAL = 2,
};

NetworkStatus GetNetworkStatus(INetwork* network) {
  NLM_CONNECTIVITY connectivity;
  if (FAILED(network->GetConnectivity(&connectivity))) {
    return NETWORK_STATUS_NOT_BEHIND_CAPTIVE_PORTAL;
  }

  if (connectivity == NLM_CONNECTIVITY_DISCONNECTED) {
    return NETWORK_STATUS_DISCONNECTED;
  }

  base::win::ScopedComPtr<IPropertyBag> property_bag;
  if (FAILED(network->QueryInterface(property_bag.GetAddressOf())) ||
      !property_bag)
    return NETWORK_STATUS_NOT_BEHIND_CAPTIVE_PORTAL;

  base::win::ScopedVariant connectivity_variant;

  // IPV4 connection:
  if ((connectivity & NLM_CONNECTIVITY_IPV4_INTERNET) ||
      (connectivity & NLM_CONNECTIVITY_IPV4_LOCALNETWORK)) {
    if (SUCCEEDED(property_bag->Read(NA_InternetConnectivityV4,
                                     connectivity_variant.Receive(),
                                     nullptr)) &&
        (V_UINT(connectivity_variant.ptr()) &
         NLM_INTERNET_CONNECTIVITY_WEBHIJACK) ==
            NLM_INTERNET_CONNECTIVITY_WEBHIJACK) {
      return NETWORK_STATUS_BEHIND_CAPTIVE_PORTAL;
    }
    return NETWORK_STATUS_NOT_BEHIND_CAPTIVE_PORTAL;
  }

  // IPV6 connection:
  if ((connectivity & NLM_CONNECTIVITY_IPV6_INTERNET) ==
      NLM_CONNECTIVITY_IPV6_INTERNET) {
    if (SUCCEEDED(property_bag->Read(NA_InternetConnectivityV6,
                                     connectivity_variant.Receive(),
                                     nullptr)) &&
        (V_UINT(connectivity_variant.ptr()) &
         NLM_INTERNET_CONNECTIVITY_WEBHIJACK) ==
            NLM_INTERNET_CONNECTIVITY_WEBHIJACK) {
      return NETWORK_STATUS_BEHIND_CAPTIVE_PORTAL;
    }
  }
  return NETWORK_STATUS_NOT_BEHIND_CAPTIVE_PORTAL;
}

}  // namespace

namespace chrome {

bool GetIsCaptivePortal() {
  // Assume the device is behind a captive portal if there is at least one
  // connected network and all connected networks are behind captive portals.
  INetworkListManager* network_list_manager;
  HRESULT hr = ::CoCreateInstance(CLSID_NetworkListManager, NULL, CLSCTX_ALL,
                                  IID_INetworkListManager,
                                  (LPVOID*)&network_list_manager);
  if (FAILED(hr))
    return false;

  base::win::ScopedComPtr<IEnumNetworks> enum_networks;
  hr = network_list_manager->GetNetworks(NLM_ENUM_NETWORK_ALL,
                                         enum_networks.GetAddressOf());
  if (FAILED(hr) || !enum_networks)
    return false;

  bool found_connected_networks = false;
  while (hr == S_OK) {
    base::win::ScopedComPtr<INetwork> network;
    ULONG items_returned = 0;
    hr = enum_networks->Next(1, network.GetAddressOf(), &items_returned);
    if (items_returned == 0)
      break;

    switch (GetNetworkStatus(network.Get())) {
      case NETWORK_STATUS_DISCONNECTED:
        // Ignore disconnected networks.
        continue;

      case NETWORK_STATUS_NOT_BEHIND_CAPTIVE_PORTAL:
        return false;
    }
    found_connected_networks = true;
  }
  return found_connected_networks;
}

}  // namespace chrome
