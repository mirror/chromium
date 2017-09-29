// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/public/browser/network_connection_tracker.h"

#include <utility>

#include "content/public/common/network_service.mojom.h"

namespace content {

using mojom::ConnectionType;
using mojom::ConnectionSubtype;

NetworkConnectionTracker::NetworkConnectionTracker(
    mojom::NetworkService* network_service)
    : connection_type_(mojom::ConnectionType::CONNECTION_UNKNOWN),
      connection_subtype_(mojom::ConnectionSubtype::SUBTYPE_UNKNOWN),
      network_change_observer_list_(
          new base::ObserverListThreadSafe<NetworkConnectionObserver>(
              base::ObserverListBase<
                  NetworkConnectionObserver>::NOTIFY_EXISTING_ONLY)),
      binding_(this) {
  // Get NetworkChangeManagerPtr.
  mojom::NetworkChangeManagerPtr manager_ptr;
  mojom::NetworkChangeManagerRequest request(mojo::MakeRequest(&manager_ptr));
  network_service->GetNetworkChangeManager(std::move(request));

  // Request notification from NetworkChangeManagerPtr.
  mojom::NetworkChangeManagerClientPtr client_ptr;
  mojom::NetworkChangeManagerClientRequest client_request(
      mojo::MakeRequest(&client_ptr));
  binding_.Bind(std::move(client_request));
  manager_ptr->RequestNotifications(std::move(client_ptr));
}

NetworkConnectionTracker::~NetworkConnectionTracker() {}

mojom::ConnectionType NetworkConnectionTracker::GetConnectionType() const {
  return connection_type_;
}

mojom::ConnectionSubtype NetworkConnectionTracker::GetConnectionSubtype()
    const {
  return connection_subtype_;
}

// static
bool NetworkConnectionTracker::IsConnectionCellular(
    mojom::ConnectionType type) {
  bool is_cellular = false;
  switch (type) {
    case mojom::ConnectionType::CONNECTION_2G:
    case mojom::ConnectionType::CONNECTION_3G:
    case mojom::ConnectionType::CONNECTION_4G:
      is_cellular = true;
      break;
    case mojom::ConnectionType::CONNECTION_UNKNOWN:
    case mojom::ConnectionType::CONNECTION_ETHERNET:
    case mojom::ConnectionType::CONNECTION_WIFI:
    case mojom::ConnectionType::CONNECTION_NONE:
    case mojom::ConnectionType::CONNECTION_BLUETOOTH:
      is_cellular = false;
      break;
  }
  return is_cellular;
}

void NetworkConnectionTracker::AddNetworkConnectionObserver(
    NetworkConnectionObserver* observer) {
  network_change_observer_list_->AddObserver(observer);
}

void NetworkConnectionTracker::RemoveNetworkConnectionObserver(
    NetworkConnectionObserver* observer) {
  network_change_observer_list_->RemoveObserver(observer);
}

void NetworkConnectionTracker::OnNetworkChanged(
    mojom::ConnectionType type,
    mojom::ConnectionSubtype subtype) {
  connection_type_ = type;
  connection_subtype_ = subtype;
  network_change_observer_list_->Notify(
      FROM_HERE, &NetworkConnectionObserver::OnConnectionChanged, type);
}

}  // namespace content
