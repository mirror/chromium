// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/network/network_change_manager_impl.h"

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
  client_ptr.set_connection_error_handler(
      base::Bind(&NetworkChangeManagerImpl::NotificationPipeBroken,
                 // base::Unretained is safe as destruction of the
                 // NetworkChangeManagerImpl will also destroy the
                 // |clients_| list (which this object will be
                 // inserted into, below), which will destroy the
                 // client_ptr, rendering this callback moot.
                 base::Unretained(this),
                 base::Unretained(client_ptr.get())));
  clients_.push_back(std::move(client_ptr));
}

void NetworkChangeManagerImpl::NotificationPipeBroken(
    mojom::NetworkChangeManagerClient* client) {
  for (auto it = clients_.begin(); it != clients_.end(); ++it) {
    if (it->get() == client) {
      // It isn't expected this will be a common enough operation for
      // the performance of std::vector::erase() to matter.
      clients_.erase(it);
      return;
    }
  }
  // A broken connection error should never be raised for an unknown pipe.
  NOTREACHED();
}

void NetworkChangeManagerImpl::OnNetworkChanged(
    net::NetworkChangeNotifier::ConnectionType type) {
  for (auto it = clients_.begin(); it != clients_.end(); ++it) {
    it->get()->OnNetworkChanged(
        mojom::ConnectionType(type),
        mojom::ConnectionSubtype(
            net::NetworkChangeNotifier::GetConnectionSubtype()));
  }
}

}  // namespace content
