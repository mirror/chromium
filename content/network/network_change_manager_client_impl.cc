// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/network/network_change_manager_client_impl.h"

#include <utility>

#include "content/public/browser/browser_thread.h"
#include "content/public/browser/network_service_instance.h"
#include "content/public/common/network_service.mojom.h"

namespace content {

using mojom::ConnectionType;
using mojom::ConnectionSubtype;

namespace {

NetworkChangeManagerClientImpl* g_network_change_notifier = nullptr;

}  // namespace

NetworkChangeManagerClientImpl::NetworkChangeManagerClientImpl(
    mojom::NetworkService* network_service)
    : connection_type_(mojom::ConnectionType::CONNECTION_UNKNOWN),
      connection_subtype_(mojom::ConnectionSubtype::SUBTYPE_UNKNOWN),
      network_change_observer_list_(new base::ObserverListThreadSafe<
                                    NetworkChangeObserver>(
          base::ObserverListBase<NetworkChangeObserver>::NOTIFY_EXISTING_ONLY)),
      binding_(this) {
  DCHECK(!g_network_change_notifier);

  // Get NetworkChangeManagerPtr.
  mojom::NetworkChangeManagerRequest request(mojo::MakeRequest(&manager_ptr_));
  network_service->GetNetworkChangeManager(std::move(request));

  // Request notification from NetworkChangeManagerPtr.
  mojom::NetworkChangeManagerClientPtr client_ptr;
  mojom::NetworkChangeManagerClientRequest client_request(
      mojo::MakeRequest(&client_ptr));
  binding_.Bind(std::move(client_request));
  manager_ptr_->RequestNotification(std::move(client_ptr));
}

NetworkChangeManagerClientImpl::~NetworkChangeManagerClientImpl() {}

// static
void NetworkChangeManagerClientImpl::Initialize(
    mojom::NetworkService* network_service) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));

  g_network_change_notifier =
      new NetworkChangeManagerClientImpl(network_service);
}

// static
mojom::ConnectionType NetworkChangeManagerClientImpl::GetConnectionType() {
  DCHECK(g_network_change_notifier);
  return g_network_change_notifier->GetCurrentConnectionType();
}

// static
mojom::ConnectionSubtype
NetworkChangeManagerClientImpl::GetConnectionSubtype() {
  DCHECK(g_network_change_notifier);
  return g_network_change_notifier->GetCurrentConnectionSubtype();
}

// static
bool NetworkChangeManagerClientImpl::IsConnectionCellular(
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

void NetworkChangeManagerClientImpl::AddNetworkChangeObserver(
    NetworkChangeObserver* observer) {
  DCHECK(g_network_change_notifier);
  g_network_change_notifier->network_change_observer_list_->AddObserver(
      observer);
}

void NetworkChangeManagerClientImpl::RemoveNetworkChangeObserver(
    NetworkChangeObserver* observer) {
  DCHECK(g_network_change_notifier);
  g_network_change_notifier->network_change_observer_list_->RemoveObserver(
      observer);
}

mojom::ConnectionType NetworkChangeManagerClientImpl::GetCurrentConnectionType()
    const {
  return connection_type_;
}

mojom::ConnectionSubtype
NetworkChangeManagerClientImpl::GetCurrentConnectionSubtype() const {
  return connection_subtype_;
}

void NetworkChangeManagerClientImpl::OnNetworkChanged(
    mojom::ConnectionType type,
    mojom::ConnectionSubtype subtype) {
  connection_type_ = type;
  connection_subtype_ = subtype;
  network_change_observer_list_->Notify(
      FROM_HERE, &NetworkChangeObserver::OnNetworkChanged, type);
}

}  // namespace content
