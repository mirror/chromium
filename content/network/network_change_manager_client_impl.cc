// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/network/network_change_manager_client_impl.h"

#include "base/lazy_instance.h"

namespace content {

using mojom::ConnectionType;
using mojom::ConnectionSubtype;

namespace {
base::LazyInstance<NetworkChangeManagerClientImpl>::Leaky
    g_network_change_notifier = LAZY_INSTANCE_INITIALIZER;
}

NetworkChangeManagerClientImpl::NetworkChangeManagerClientImpl() {}

NetworkChangeManagerClientImpl::~NetworkChangeManagerClientImpl() {}

// static
ConnectionType NetworkChangeManagerClientImpl::GetConnectionType() {
  return g_network_change_notifier.Get().GetCurrentConnectionType();
}

// static
ConnectionSubtype NetworkChangeManagerClientImpl::GetConnectionSubtype() {
  return g_network_change_notifier.Get().GetCurrentConnectionSubtype();
}

// static
bool NetworkChangeManagerClientImpl::IsConnectionCellular(ConnectionType type) {
  bool is_cellular = false;
  switch (type) {
    case ConnectionType::CONNECTION_2G:
    case ConnectionType::CONNECTION_3G:
    case ConnectionType::CONNECTION_4G:
      is_cellular = true;
      break;
    case ConnectionType::CONNECTION_UNKNOWN:
    case ConnectionType::CONNECTION_ETHERNET:
    case ConnectionType::CONNECTION_WIFI:
    case ConnectionType::CONNECTION_NONE:
    case ConnectionType::CONNECTION_BLUETOOTH:
      is_cellular = false;
      break;
  }
  return is_cellular;
}

void NetworkChangeManagerClientImpl::AddNetworkChangeObserver(
    NetworkChangeObserver* observer) {
  g_network_change_notifier.Get().network_change_observer_list_->AddObserver(
      observer);
}

void NetworkChangeManagerClientImpl::RemoveNetworkChangeObserver(
    NetworkChangeObserver* observer) {
  g_network_change_notifier.Get().network_change_observer_list_->RemoveObserver(
      observer);
}

ConnectionType NetworkChangeManagerClientImpl::GetCurrentConnectionType()
    const {
  return ConnectionType::CONNECTION_UNKNOWN;
}

ConnectionSubtype NetworkChangeManagerClientImpl::GetCurrentConnectionSubtype()
    const {
  return ConnectionSubtype::SUBTYPE_UNKNOWN;
}

void NetworkChangeManagerClientImpl::OnNetworkChanged(
    ConnectionType type,
    ConnectionSubtype subtype) {}

}  // namespace content
