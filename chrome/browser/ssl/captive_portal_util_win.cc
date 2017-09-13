// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ssl/captive_portal_util.h"

#include <netlistmgr.h>

#include "base/logging.h"
#include "base/win/scoped_bstr.h"
#include "base/win/scoped_comptr.h"
#include "base/win/scoped_variant.h"

namespace {

bool GetIsCaptivePortalForNetwork(INetwork* network) {
  base::win::ScopedComPtr<IPropertyBag> property_bag;
  if (FAILED(network->QueryInterface(property_bag.GetAddressOf())))
    return false;

  if (!property_bag)
    return false;

  base::win::ScopedBstr name;
  network->GetName(name.Receive());
  LOG(ERROR) << ">> Network Name: " << name;

  NLM_CONNECTIVITY connectivity;
  base::win::ScopedVariant connectivity_variant;

  if (FAILED(network->GetConnectivity(&connectivity)))
    return false;

  // IPV4 connection:
  if ((connectivity & NLM_CONNECTIVITY_IPV4_INTERNET) ==
      NLM_CONNECTIVITY_IPV4_INTERNET) {
    LOG(ERROR) << ">> IPV4 Connection";
    return SUCCEEDED(property_bag->Read(NA_InternetConnectivityV4,
                                        connectivity_variant.Receive(),
                                        nullptr)) &&
           (V_UINT(connectivity_variant.ptr()) &
            NLM_INTERNET_CONNECTIVITY_WEBHIJACK) ==
               NLM_INTERNET_CONNECTIVITY_WEBHIJACK;
  }
  // IPV6 connection:
  if ((connectivity & NLM_CONNECTIVITY_IPV6_INTERNET) ==
      NLM_CONNECTIVITY_IPV6_INTERNET) {
    LOG(ERROR) << ">> IPV6 Connection";
    return SUCCEEDED(property_bag->Read(NA_InternetConnectivityV6,
                                        connectivity_variant.Receive(),
                                        nullptr)) &&
           (V_UINT(connectivity_variant.ptr()) &
            NLM_INTERNET_CONNECTIVITY_WEBHIJACK) ==
               NLM_INTERNET_CONNECTIVITY_WEBHIJACK;
  }
  return false;
}

}  // namespace

namespace chrome {
bool GetIsCaptivePortal() {
  LOG(ERROR) << ">> GetIsCaptivePortal 0";
  INetworkListManager* network_list_manager;
  HRESULT hr = ::CoCreateInstance(CLSID_NetworkListManager, NULL, CLSCTX_ALL,
                                  IID_INetworkListManager,
                                  (LPVOID*)&network_list_manager);

  if (FAILED(hr))
    return false;

  LOG(ERROR) << ">> GetIsCaptivePortal 1";

  base::win::ScopedComPtr<IEnumNetworks> enum_networks;
  // hr = network_list_manager->GetNetworks(NLM_ENUM_NETWORK_CONNECTED,
  // enum_networks.GetAddressOf());
  hr = network_list_manager->GetNetworks(NLM_ENUM_NETWORK_ALL,
                                         enum_networks.GetAddressOf());
  if (FAILED(hr) || !enum_networks)
    return false;

  LOG(ERROR) << ">> GetIsCaptivePortal 2";

  while (hr == S_OK) {
    base::win::ScopedComPtr<INetwork> network;
    ULONG items_returned = 0;
    hr = enum_networks->Next(1, network.GetAddressOf(), &items_returned);
    if (items_returned == 0)
      break;

    LOG(ERROR) << ">> Enumerated network";
    if (GetIsCaptivePortalForNetwork(network.Get())) {
      LOG(ERROR) << ">> Found captive portal, returning true";
      return true;
    }
  }
  return true;
}

}  // namespace chrome
