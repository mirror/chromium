// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ssl/captive_portal_helper.h"

#include <netlistmgr.h>

#include "base/win/scoped_comptr.h"
#include "base/win/scoped_variant.h"

namespace {

bool IsNetworkBehindCaptivePortal(INetwork* network) {
  NLM_CONNECTIVITY connectivity;
  if (FAILED(network->GetConnectivity(&connectivity)))
    return false;

  if (connectivity == NLM_CONNECTIVITY_DISCONNECTED)
    return false;

  base::win::ScopedComPtr<IPropertyBag> property_bag;
  if (FAILED(network->QueryInterface(property_bag.GetAddressOf())) ||
      !property_bag) {
    return false;
  }

  base::win::ScopedVariant connectivity_variant;

  if ((connectivity & NLM_CONNECTIVITY_IPV4_INTERNET) ||
      (connectivity & NLM_CONNECTIVITY_IPV4_LOCALNETWORK)) {
    // IPV4 connection:
    if (SUCCEEDED(property_bag->Read(NA_InternetConnectivityV4,
                                     connectivity_variant.Receive(),
                                     nullptr)) &&
        (V_UINT(connectivity_variant.ptr()) &
         NLM_INTERNET_CONNECTIVITY_WEBHIJACK) ==
            NLM_INTERNET_CONNECTIVITY_WEBHIJACK) {
      return true;
    }
  } else if ((connectivity & NLM_CONNECTIVITY_IPV6_INTERNET) ||
             (connectivity & NLM_CONNECTIVITY_IPV6_LOCALNETWORK)) {
    // IPV6 connection:
    if (SUCCEEDED(property_bag->Read(NA_InternetConnectivityV6,
                                     connectivity_variant.Receive(),
                                     nullptr)) &&
        (V_UINT(connectivity_variant.ptr()) &
         NLM_INTERNET_CONNECTIVITY_WEBHIJACK) ==
            NLM_INTERNET_CONNECTIVITY_WEBHIJACK) {
      return true;
    }
  }
  return false;
}

}  // namespace

namespace chrome {

bool IsBehindCaptivePortal() {
  // Assume the device is behind a captive portal if there is at least one
  // connected network and all connected networks are behind captive portals.
  base::win::ScopedComPtr<INetworkListManager> network_list_manager;
  if (FAILED(CoCreateInstance(CLSID_NetworkListManager, nullptr,
                              CLSCTX_INPROC_SERVER,
                              IID_PPV_ARGS(&network_list_manager)))) {
    return false;
  }

  base::win::ScopedComPtr<IEnumNetworks> enum_networks;
  if (FAILED(network_list_manager->GetNetworks(NLM_ENUM_NETWORK_CONNECTED,
                                               enum_networks.GetAddressOf()))) {
    return false;
  }

  if (!enum_networks)
    return false;

  bool found = false;
  while (true) {
    base::win::ScopedComPtr<INetwork> network;
    ULONG items_returned = 0;
    if (FAILED(enum_networks->Next(1, network.GetAddressOf(), &items_returned)))
      return false;

    if (!IsNetworkBehindCaptivePortal(network.Get()))
      return false;

    found = true;
  }
  return found;
}

}  // namespace chrome
