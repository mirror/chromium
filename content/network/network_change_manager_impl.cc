// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/network/network_change_manager_impl.h"

#include <algorithm>
#include <utility>

namespace content {

NetworkChangeManagerImpl::NetworkChangeManagerImpl() {
  net::NetworkChangeNotifier::AddNetworkChangeObserver(this);
}

NetworkChangeManagerImpl::~NetworkChangeManagerImpl() {
  net::NetworkChangeNotifier::RemoveNetworkChangeObserver(this);
}

void NetworkChangeManagerImpl::AddRequest(
    mojom::NetworkChangeManagerRequest request) {
  bindings_.AddBinding(this, std::move(request));
}

void NetworkChangeManagerImpl::RequestNotification(
    mojom::NetworkChangeManagerClientPtr client_ptr) {
  std::set<mojom::NetworkChangeManagerClientPtr>::iterator iter =
      clients_.insert(std::move(client_ptr));
  (*iter.first)
      .set_connection_error_handler(
          base::Bind(&NetworkChangeManagerImpl::NotificationPipeBroken,
                     // base::Unretained is safe as destruction of the
                     // NetworkChangeManagerImpl will also destroy the
                     // |clients_| list (which this object will be
                     // inserted into, below), which will destroy the
                     // client_ptr, rendering this callback moot.
                     base::Unretained(this)));
}

void NetworkChangeManagerImpl::NotificationPipeBroken() {
  //  const std::set<mojom::NetworkChangeManagerClientPtr>::iterator& iter) {
  //  clients_.erase(iter);
}

void NetworkChangeManagerImpl::OnNetworkChanged(
    net::NetworkChangeNotifier::ConnectionType type) {
  for (const auto& client : clients_) {
    client->OnNetworkChanged(
        mojom::ConnectionType(type),
        mojom::ConnectionSubtype(
            net::NetworkChangeNotifier::GetConnectionSubtype()));
  }
}

}  // namespace content
